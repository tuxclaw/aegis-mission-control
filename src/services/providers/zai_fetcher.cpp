#include "services/providers/zai_fetcher.h"

#include "services/monitor_config.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>

ZAIFetcher::ZAIFetcher(const MonitorConfig* config, QNetworkAccessManager* nam,
                       QObject* parent)
    : IProviderFetcher(nam, parent), m_config(config) {}

void ZAIFetcher::fetch() {
    const QString apiKey = m_config->providerCredential(
        QStringLiteral("zai"), QStringLiteral("apiKey"));
    if (apiKey.isEmpty()) {
        emit failed(id(), QStringLiteral("Missing API key"));
        return;
    }

    const QUrl url(QStringLiteral("https://api.z.ai/api/monitor/usage/quota/limit"));
    const QList<QPair<QByteArray, QByteArray>> headers = {
        { QByteArrayLiteral("Authorization"),
          QByteArray("Bearer ") + apiKey.toUtf8() },
        { QByteArrayLiteral("Accept"),
          QByteArrayLiteral("application/json") },
    };

    QNetworkReply* reply = httpGet(url, headers);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onReplyFinished(reply);
    });
}

void ZAIFetcher::onReplyFinished(QNetworkReply* reply) {
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        const int status = reply->attribute(
            QNetworkRequest::HttpStatusCodeAttribute).toInt();
        emit failed(id(),
            QStringLiteral("HTTP %1: %2").arg(status).arg(reply->errorString()));
        return;
    }

    const QByteArray raw = reply->readAll();
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(raw, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        emit failed(id(),
            QStringLiteral("JSON parse error: %1").arg(parseError.errorString()));
        return;
    }

    const QJsonObject root = doc.object();
    const int code = root.value(QStringLiteral("code")).toInt();
    const bool success = root.value(QStringLiteral("success")).toBool();
    if (!success || code != 200) {
        const QString msg = root.value(QStringLiteral("msg")).toString();
        emit failed(id(),
            QStringLiteral("API error (code %1): %2").arg(code).arg(
                msg.isEmpty() ? QStringLiteral("unknown") : msg));
        return;
    }

    const QJsonObject data = root.value(QStringLiteral("data")).toObject();
    const QString planName = data.value(QStringLiteral("planName")).toString();
    const QJsonArray limits = data.value(QStringLiteral("limits")).toArray();

    ProviderQuota quota;
    quota.id = id();
    quota.displayName = displayName();
    if (!planName.isEmpty())
        quota.planName = planName;

    // Partition limits into token limits and time limits
    QList<QJsonObject> tokenLimits;
    QJsonObject timeLimitObj;
    bool hasTimeLimit = false;

    for (const QJsonValue& v : limits) {
        const QJsonObject obj = v.toObject();
        const QString type = obj.value(QStringLiteral("type")).toString();
        if (type == QLatin1String("TOKENS_LIMIT")) {
            tokenLimits.append(obj);
        } else if (type == QLatin1String("TIME_LIMIT")) {
            timeLimitObj = obj;
            hasTimeLimit = true;
        }
    }

    // Helper: map unit int to window label
    auto windowLabel = [](const QJsonObject& obj) -> QString {
        const int unit = obj.value(QStringLiteral("unit")).toInt();
        const int number = obj.value(QStringLiteral("number")).toInt();
        switch (unit) {
        case 1: return QStringLiteral("daily");
        case 3: return QStringLiteral("%1h").arg(number);
        case 5: return QStringLiteral("%1m").arg(number);
        case 6: return QStringLiteral("weekly");
        default: return QStringLiteral("%1/%2").arg(number).arg(unit);
        }
    };

    // Helper: compute used fraction
    auto usedFraction = [](const QJsonObject& obj, const QString& limitType) -> double {
        const double pct = obj.value(QStringLiteral("percentage")).toDouble();
        if (limitType == QLatin1String("TOKENS_LIMIT"))
            return pct / 100.0;

        // TIME_LIMIT: prefer usage/(usage+remaining) if available
        const double usage = obj.value(QStringLiteral("usage")).toDouble();
        const double remaining = obj.value(QStringLiteral("remaining")).toDouble();
        if (usage > 0.0 || remaining > 0.0) {
            const double total = usage + remaining;
            if (total > 0.0)
                return qBound(0.0, usage / total, 1.0);
        }
        return pct / 100.0;
    };

    // Sort TOKENS_LIMIT entries by window size (number * unit multiplier), longest first
    std::sort(tokenLimits.begin(), tokenLimits.end(),
        [](const QJsonObject& a, const QJsonObject& b) {
            auto sizeKey = [](const QJsonObject& obj) -> int {
                const int unit = obj.value(QStringLiteral("unit")).toInt();
                const int number = obj.value(QStringLiteral("number")).toInt();
                switch (unit) {
                case 1:  return number * 1440;      // days
                case 3:  return number * 60;         // hours
                case 5:  return number;              // minutes
                case 6:  return number * 10080;      // weeks
                default: return number * unit;
                }
            };
            return sizeKey(a) > sizeKey(b);
        });

    // Convert token limits to windows (longest = primary first)
    for (const QJsonObject& obj : std::as_const(tokenLimits)) {
        ProviderWindow w;
        w.label = windowLabel(obj);
        w.usedFraction = usedFraction(obj, QStringLiteral("TOKENS_LIMIT"));
        w.tokensUsed = static_cast<qint64>(
            obj.value(QStringLiteral("currentValue")).toDouble());
        w.tokensLimit = static_cast<qint64>(
            obj.value(QStringLiteral("usage")).toDouble());
        const qint64 nextReset = static_cast<qint64>(
            obj.value(QStringLiteral("nextResetTime")).toDouble());
        if (nextReset > 0)
            w.resetsAt = QDateTime::fromSecsSinceEpoch(nextReset);
        quota.windows.append(w);
    }

    // Convert time limit to window
    if (hasTimeLimit) {
        ProviderWindow w;
        w.label = windowLabel(timeLimitObj);
        w.usedFraction = usedFraction(timeLimitObj, QStringLiteral("TIME_LIMIT"));
        w.tokensUsed = static_cast<qint64>(
            timeLimitObj.value(QStringLiteral("currentValue")).toDouble());
        w.tokensLimit = static_cast<qint64>(
            timeLimitObj.value(QStringLiteral("usage")).toDouble());
        const qint64 nextReset = static_cast<qint64>(
            timeLimitObj.value(QStringLiteral("nextResetTime")).toDouble());
        if (nextReset > 0)
            w.resetsAt = QDateTime::fromSecsSinceEpoch(nextReset);
        quota.windows.append(w);
    }

    emit done(quota);
}
