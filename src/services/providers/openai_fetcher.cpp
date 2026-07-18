#include "services/providers/openai_fetcher.h"

#include "services/monitor_config.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>

OpenAIFetcher::OpenAIFetcher(const MonitorConfig* config, QNetworkAccessManager* nam,
                             QObject* parent)
    : IProviderFetcher(nam, parent), m_config(config) {}

void OpenAIFetcher::fetch() {
    const QString apiKey = m_config->providerCredential(
        QStringLiteral("openai"), QStringLiteral("apiKey"));
    if (apiKey.isEmpty()) {
        emit failed(id(), QStringLiteral("Missing API key"));
        return;
    }

    // Request 1 — Credit grants
    const QUrl url(QStringLiteral(
        "https://api.openai.com/v1/dashboard/billing/credit_grants"));
    const QList<QPair<QByteArray, QByteArray>> headers = {
        { QByteArrayLiteral("Authorization"),
          QByteArray("Bearer ") + apiKey.toUtf8() },
        { QByteArrayLiteral("Accept"),
          QByteArrayLiteral("application/json") },
    };

    QNetworkReply* reply = httpGet(url, headers);
    connect(reply, &QNetworkReply::finished, this, [this, reply, apiKey]() {
        reply->deleteLater();

        double balance = 0.0;
        bool balanceAvailable = false;

        const int status = reply->attribute(
            QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (reply->error() == QNetworkReply::NoError && status == 200) {
            const QByteArray raw = reply->readAll();
            const QJsonDocument doc = QJsonDocument::fromJson(raw);
            const QJsonObject root = doc.object();
            const QJsonArray data = root.value(
                QStringLiteral("data")).toArray();
            for (const QJsonValue& v : data) {
                const QJsonObject obj = v.toObject();
                balance += obj.value(
                    QStringLiteral("balance")).toDouble();
            }
            balanceAvailable = true;
        } else if (status == 403) {
            // Org keys don't support this endpoint — skip gracefully
            balanceAvailable = false;
        } else {
            qWarning() << "OpenAIFetcher: credit grants HTTP" << status
                       << reply->errorString();
        }

        // Request 2 — Monthly usage
        const QDate today = QDate::currentDate();
        const QDate monthStart(today.year(), today.month(), 1);
        const QString startDate = monthStart.toString(
            QStringLiteral("yyyy-MM-dd"));
        const QString endDate = today.toString(
            QStringLiteral("yyyy-MM-dd"));

        const QUrl usageUrl(QStringLiteral(
            "https://api.openai.com/v1/dashboard/billing/usage"
            "?start_date=%1&end_date=%2").arg(startDate, endDate));
        const QList<QPair<QByteArray, QByteArray>> usageHeaders = {
            { QByteArrayLiteral("Authorization"),
              QByteArray("Bearer ") + apiKey.toUtf8() },
            { QByteArrayLiteral("Accept"),
              QByteArrayLiteral("application/json") },
        };

        QNetworkReply* usageReply = httpGet(usageUrl, usageHeaders);
        connect(usageReply, &QNetworkReply::finished, this,
                [this, usageReply, balance, balanceAvailable]() {
            usageReply->deleteLater();

            const int usageStatus = usageReply->attribute(
                QNetworkRequest::HttpStatusCodeAttribute).toInt();

            if (usageReply->error() != QNetworkReply::NoError || usageStatus != 200) {
                if (!balanceAvailable) {
                    emit failed(id(),
                        QStringLiteral("HTTP %1: %2")
                            .arg(usageStatus)
                            .arg(usageReply->errorString()));
                    return;
                }
                // Balance available but usage failed — emit partial quota
                ProviderQuota quota;
                quota.id = id();
                quota.displayName = displayName();
                quota.balance.total = balance;
                quota.balance.currency = QStringLiteral("USD");
                emit done(quota);
                return;
            }

            const QByteArray raw = usageReply->readAll();
            const QJsonDocument doc = QJsonDocument::fromJson(raw);
            const QJsonObject root = doc.object();

            // total_usage is in cents
            const double totalUsageCents = root.value(
                QStringLiteral("total_usage")).toDouble();
            const double creditsUsed = totalUsageCents / 100.0;

            ProviderQuota quota;
            quota.id = id();
            quota.displayName = displayName();

            if (balanceAvailable) {
                quota.balance.total = balance;
                quota.balance.currency = QStringLiteral("USD");
            }

            ProviderWindow w;
            w.label = QStringLiteral("monthly");
            w.creditsUsed = creditsUsed;

            if (balanceAvailable && (creditsUsed + balance) > 0.0) {
                w.usedFraction = creditsUsed / (creditsUsed + balance);
            }

            // Parse daily_costs for tokensUsed if available
            const QJsonArray dailyCosts = root.value(
                QStringLiteral("daily_costs")).toArray();
            qint64 totalTokens = 0;
            for (const QJsonValue& day : dailyCosts) {
                const QJsonArray lineItems = day.toObject().value(
                    QStringLiteral("line_items")).toArray();
                for (const QJsonValue& item : lineItems) {
                    totalTokens += static_cast<qint64>(
                        item.toObject().value(
                            QStringLiteral("n_context_tokens_total"))
                            .toDouble());
                }
            }
            if (totalTokens > 0)
                w.tokensUsed = totalTokens;

            quota.windows.append(w);
            emit done(quota);
        });
    });
}
