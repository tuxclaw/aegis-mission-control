#include "services/providers/anthropic_fetcher.h"

#include "services/monitor_config.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>

AnthropicFetcher::AnthropicFetcher(const MonitorConfig* config,
                                   QNetworkAccessManager* nam,
                                   QObject* parent)
    : IProviderFetcher(nam, parent), m_config(config) {}

void AnthropicFetcher::fetch() {
    // Priority 1: Admin API key
    const QString adminKey = m_config->providerCredential(
        QStringLiteral("anthropic"), QStringLiteral("adminKey"));
    if (!adminKey.isEmpty()) {
        fetchWithToken(adminKey, /*isAdmin=*/true);
        return;
    }

    // Priority 2: OAuth token from ~/.claude/.credentials.json
    QFile credFile(QDir::homePath()
        + QStringLiteral("/.claude/.credentials.json"));
    if (credFile.open(QIODevice::ReadOnly)) {
        const QJsonDocument doc =
            QJsonDocument::fromJson(credFile.readAll());
        const QJsonObject root = doc.object();
        // Try nested format: {"claudeAiOauth": {"accessToken": "..."}}
        const QJsonObject oauth = root.value(
            QStringLiteral("claudeAiOauth")).toObject();
        QString oauthToken = oauth.value(
            QStringLiteral("accessToken")).toString();
        // Fallback: flat format {"claudeAiOauthToken": "..."}
        if (oauthToken.isEmpty())
            oauthToken = root.value(
                QStringLiteral("claudeAiOauthToken")).toString();
        if (!oauthToken.isEmpty()) {
            fetchWithToken(oauthToken, /*isAdmin=*/false);
            return;
        }
    }

    emit failed(id(), QStringLiteral(
        "No credentials found (set adminKey or configure Claude OAuth)"));
}

void AnthropicFetcher::fetchWithToken(const QString& token, bool isAdmin) {
    const QUrl url(QStringLiteral("https://api.anthropic.com/v1/usage"));

    QList<QPair<QByteArray, QByteArray>> headers;
    if (isAdmin) {
        headers.append({
            QByteArrayLiteral("x-api-key"), token.toUtf8() });
        headers.append({
            QByteArrayLiteral("anthropic-version"),
            QByteArrayLiteral("2023-06-01") });
    } else {
        headers.append({
            QByteArrayLiteral("Authorization"),
            QByteArray("Bearer ") + token.toUtf8() });
    }
    headers.append({
        QByteArrayLiteral("Accept"),
        QByteArrayLiteral("application/json") });

    QNetworkReply* reply = httpGet(url, headers);
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, token, isAdmin]() {
        reply->deleteLater();

        const int status = reply->attribute(
            QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // If admin key got 401/403, try OAuth fallback
        if (isAdmin && (status == 401 || status == 403)) {
            QFile credFile(QDir::homePath()
                + QStringLiteral("/.claude/.credentials.json"));
            if (credFile.open(QIODevice::ReadOnly)) {
                const QJsonDocument doc =
                    QJsonDocument::fromJson(credFile.readAll());
                const QString oauthToken = doc.object().value(
                    QStringLiteral("claudeAiOauthToken")).toString();
                if (!oauthToken.isEmpty()) {
                    fetchWithToken(oauthToken, /*isAdmin=*/false);
                    return;
                }
            }
            emit failed(id(),
                QStringLiteral("Admin key rejected and no OAuth fallback"));
            return;
        }

        if (reply->error() != QNetworkReply::NoError || status != 200) {
            emit failed(id(),
                QStringLiteral("HTTP %1: %2")
                    .arg(status).arg(reply->errorString()));
            return;
        }

        const QByteArray raw = reply->readAll();
        const QJsonDocument doc = QJsonDocument::fromJson(raw);
        const QJsonObject root = doc.object();

        const qint64 totalTokens = static_cast<qint64>(
            root.value(QStringLiteral("total_tokens")).toDouble());
        const qint64 tokenLimit = static_cast<qint64>(
            root.value(QStringLiteral("token_limit")).toDouble());
        const QString planName = root.value(
            QStringLiteral("plan_name")).toString();

        ProviderQuota quota;
        quota.id = id();
        quota.displayName = displayName();
        quota.planName = planName;
        quota.balance.currency = QStringLiteral("USD");

        ProviderWindow w;
        w.label = QStringLiteral("monthly");
        w.tokensUsed = totalTokens;
        w.tokensLimit = tokenLimit;
        if (tokenLimit > 0)
            w.usedFraction = static_cast<double>(totalTokens) /
                             static_cast<double>(tokenLimit);

        quota.windows.append(w);
        emit done(quota);
    });
}
