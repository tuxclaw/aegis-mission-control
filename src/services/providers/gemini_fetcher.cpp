#include "services/providers/gemini_fetcher.h"

#include "services/monitor_config.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>

GeminiFetcher::GeminiFetcher(const MonitorConfig* config, QNetworkAccessManager* nam,
                             QObject* parent)
    : IProviderFetcher(nam, parent), m_config(config) {}

void GeminiFetcher::fetch() {
    QString apiKey = m_config->providerCredential(
        QStringLiteral("gemini"), QStringLiteral("apiKey"));
    if (apiKey.isEmpty())
        apiKey = qEnvironmentVariable("GEMINI_API_KEY");
    if (apiKey.isEmpty()) {
        emit failed(id(), QStringLiteral("Missing API key"));
        return;
    }

    const QUrl url(QStringLiteral(
        "https://generativelanguage.googleapis.com/v1beta/models/"
        "gemini-2.0-flash:generateContent?key=%1").arg(apiKey));

    QJsonObject part;
    part[QStringLiteral("text")] = QStringLiteral("hi");

    QJsonObject content;
    content[QStringLiteral("parts")] = QJsonArray{part};

    QJsonObject genConfig;
    genConfig[QStringLiteral("maxOutputTokens")] = 1;

    QJsonObject body;
    body[QStringLiteral("contents")] = QJsonArray{content};
    body[QStringLiteral("generationConfig")] = genConfig;

    const QByteArray payload = QJsonDocument(body).toJson(QJsonDocument::Compact);

    const QList<QPair<QByteArray, QByteArray>> headers = {
        { QByteArrayLiteral("Content-Type"),
          QByteArrayLiteral("application/json") },
        { QByteArrayLiteral("Accept"),
          QByteArrayLiteral("application/json") },
    };

    QNetworkReply* reply = httpPost(url, payload, headers);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onReplyFinished(reply);
    });
}

void GeminiFetcher::onReplyFinished(QNetworkReply* reply) {
    reply->deleteLater();

    const int status = reply->attribute(
        QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (reply->error() != QNetworkReply::NoError) {
        const QByteArray raw = reply->readAll();
        const QJsonDocument doc = QJsonDocument::fromJson(raw);
        const QJsonObject root = doc.object();
        const QJsonObject err = root.value(QStringLiteral("error")).toObject();
        const QString msg = err.value(QStringLiteral("message")).toString();
        const QString code = err.value(QStringLiteral("code")).toString();

        if (status == 400 && code.contains(QStringLiteral("API_KEY_INVALID"), Qt::CaseInsensitive))
            emit failed(id(), QStringLiteral("Invalid API key"));
        else if (status == 429)
            emit failed(id(), QStringLiteral("Rate limited — quota exceeded"));
        else
            emit failed(id(),
                QStringLiteral("HTTP %1: %2").arg(status)
                    .arg(msg.isEmpty() ? reply->errorString() : msg));
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
    const QJsonObject usageMeta = root.value(QStringLiteral("usageMetadata")).toObject();
    const qint64 totalTokens = static_cast<qint64>(
        usageMeta.value(QStringLiteral("totalTokenCount")).toDouble());

    ProviderQuota quota;
    quota.id = id();
    quota.displayName = displayName();
    quota.planName = QStringLiteral("API Key");

    ProviderWindow w;
    w.label = QStringLiteral("probe");
    w.tokensUsed = totalTokens;
    w.usedFraction = -1.0;  // unknown

    quota.windows.append(w);
    emit done(quota);
}
