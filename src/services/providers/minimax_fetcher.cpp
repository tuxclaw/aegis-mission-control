#include "services/providers/minimax_fetcher.h"

#include "services/monitor_config.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QRegularExpression>

MiniMaxFetcher::MiniMaxFetcher(const MonitorConfig* config,
                               QNetworkAccessManager* nam,
                               QObject* parent)
    : IProviderFetcher(nam, parent), m_config(config) {}

void MiniMaxFetcher::fetch() {
    const QString authToken = m_config->providerCredential(
        QStringLiteral("minimax"), QStringLiteral("authToken"));
    const QString cookie = m_config->providerCredential(
        QStringLiteral("minimax"), QStringLiteral("cookie"));

    if (!authToken.isEmpty()) {
        // Try API with auth token first
        const QUrl url(QStringLiteral(
            "https://api.minimax.io/v1/token_plan/remains"));
        const QList<QPair<QByteArray, QByteArray>> headers = {
            { QByteArrayLiteral("Authorization"),
              QByteArray("Bearer ") + authToken.toUtf8() },
            { QByteArrayLiteral("Accept"),
              QByteArrayLiteral("application/json") },
        };

        QNetworkReply* reply = httpGet(url, headers);
        connect(reply, &QNetworkReply::finished, this,
                [this, reply, cookie]() {
            reply->deleteLater();

            const int status = reply->attribute(
                QNetworkRequest::HttpStatusCodeAttribute).toInt();

            if (reply->error() == QNetworkReply::NoError && status == 200) {
                const QByteArray raw = reply->readAll();
                const QJsonDocument doc = QJsonDocument::fromJson(raw);
                const QJsonObject root = doc.object();
                const QJsonObject data = root.value(
                    QStringLiteral("data")).toObject();
                const QJsonArray models = data.value(
                    QStringLiteral("model_remains")).toArray();

                if (!models.isEmpty()) {
                    const QJsonObject model = models.first().toObject();
                    const qint64 totalCount = static_cast<qint64>(
                        model.value(
                            QStringLiteral("current_interval_total_count"))
                            .toDouble());
                    const qint64 usageCount = static_cast<qint64>(
                        model.value(
                            QStringLiteral("current_interval_usage_count"))
                            .toDouble());
                    const double remainingPct = model.value(
                        QStringLiteral("current_interval_remaining_percent"))
                        .toDouble();
                    const qint64 endTime = static_cast<qint64>(
                        model.value(QStringLiteral("end_time")).toDouble());

                    ProviderQuota quota;
                    quota.id = id();
                    quota.displayName = displayName();

                    ProviderWindow w;
                    w.label = QStringLiteral("plan");
                    w.tokensUsed = usageCount;
                    w.tokensLimit = totalCount;
                    w.usedFraction = (100.0 - remainingPct) / 100.0;
                    if (endTime > 0)
                        w.resetsAt = QDateTime::fromSecsSinceEpoch(endTime);

                    quota.windows.append(w);
                    emit done(quota);
                    return;
                }
            }

            // Auth token failed or no data — fall back to cookie
            if (!cookie.isEmpty()) {
                fetchWithCookie(cookie);
            } else {
                emit failed(id(),
                    QStringLiteral("HTTP %1: %2 (no cookie fallback)")
                        .arg(status).arg(reply->errorString()));
            }
        });
        return;
    }

    // No auth token — try cookie directly
    if (!cookie.isEmpty()) {
        fetchWithCookie(cookie);
        return;
    }

    emit failed(id(), QStringLiteral(
        "No auth token or cookie configured"));
}

void MiniMaxFetcher::fetchWithCookie(const QString& cookie) {
    const QUrl url(QStringLiteral(
        "https://api.minimax.io/user-center/payment/coding-plan"
        "?cycle_type=3"));
    const QList<QPair<QByteArray, QByteArray>> headers = {
        { QByteArrayLiteral("Cookie"), cookie.toUtf8() },
        { QByteArrayLiteral("Accept"),
          QByteArrayLiteral("text/html,application/json") },
        { QByteArrayLiteral("User-Agent"),
          QByteArrayLiteral(
              "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36") },
    };

    QNetworkReply* reply = httpGet(url, headers);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            emit failed(id(),
                QStringLiteral("Cookie request failed: %1")
                    .arg(reply->errorString()));
            return;
        }

        const QByteArray raw = reply->readAll();
        const QString html = QString::fromUtf8(raw);

        // Try to extract usage patterns from HTML
        // Look for common patterns like "320 / 1000 tokens" or percentages
        static const QRegularExpression usageRe(
            QStringLiteral(R"((\d[\d,.]*)\s*/\s*(\d[\d,.]*))"));
        static const QRegularExpression pctRe(
            QStringLiteral(R"((\d+(?:\.\d+)?)\s*%)"));

        ProviderQuota quota;
        quota.id = id();
        quota.displayName = displayName();

        QRegularExpressionMatch usageMatch = usageRe.match(html);
        if (usageMatch.hasMatch()) {
            const QString usedStr = usageMatch.captured(1)
                .remove(QLatin1Char(','));
            const QString limitStr = usageMatch.captured(2)
                .remove(QLatin1Char(','));
            const double used = usedStr.toDouble();
            const double limit = limitStr.toDouble();

            ProviderWindow w;
            w.label = QStringLiteral("plan");
            w.tokensUsed = static_cast<qint64>(used);
            w.tokensLimit = static_cast<qint64>(limit);
            if (limit > 0.0)
                w.usedFraction = used / limit;

            quota.windows.append(w);
            emit done(quota);
            return;
        }

        QRegularExpressionMatch pctMatch = pctRe.match(html);
        if (pctMatch.hasMatch()) {
            const double pct = pctMatch.captured(1).toDouble();

            ProviderWindow w;
            w.label = QStringLiteral("plan");
            w.usedFraction = pct / 100.0;

            quota.windows.append(w);
            emit done(quota);
            return;
        }

        emit failed(id(),
            QStringLiteral("Could not parse usage data from HTML"));
    });
}
