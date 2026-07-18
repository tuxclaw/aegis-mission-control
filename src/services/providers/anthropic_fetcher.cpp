#include "services/providers/anthropic_fetcher.h"

#include "services/monitor_config.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>

#include <cmath>

namespace {

constexpr auto kUsageUrl = "https://api.anthropic.com/api/oauth/usage";

QString oauthPlanName(const QJsonObject& oauth) {
    const QString subscription = oauth.value(
        QStringLiteral("subscriptionType")).toString().trimmed();
    if (!subscription.isEmpty())
        return subscription;

    const QString tier = oauth.value(
        QStringLiteral("rateLimitTier")).toString().trimmed();
    return tier.isEmpty() ? QStringLiteral("Claude OAuth") : tier;
}

void appendUsageWindow(ProviderQuota& quota, const QJsonObject& root,
                       const QString& key, const QString& label) {
    const QJsonObject value = root.value(key).toObject();
    if (value.isEmpty())
        return;

    ProviderWindow window;
    window.label = label;
    window.usedFraction = qBound(
        0.0, value.value(QStringLiteral("utilization")).toDouble() / 100.0,
        1.0);
    const QString reset = value.value(
        QStringLiteral("resets_at")).toString();
    if (!reset.isEmpty())
        window.resetsAt = QDateTime::fromString(reset, Qt::ISODate);
    quota.windows.append(window);
}

}  // namespace

AnthropicFetcher::AnthropicFetcher(const MonitorConfig* config,
                                   QNetworkAccessManager* nam,
                                   QObject* parent)
    : IProviderFetcher(nam, parent), m_config(config) {}

void AnthropicFetcher::fetch() {
    QFile credentials(QDir::homePath()
        + QStringLiteral("/.claude/.credentials.json"));
    if (credentials.open(QIODevice::ReadOnly)) {
        QJsonParseError error;
        const QJsonDocument document = QJsonDocument::fromJson(
            credentials.readAll(), &error);
        const QJsonObject oauth = document.object().value(
            QStringLiteral("claudeAiOauth")).toObject();
        QString accessToken = oauth.value(
            QStringLiteral("accessToken")).toString().trimmed();
        if (accessToken.isEmpty()) {
            accessToken = document.object().value(
                QStringLiteral("claudeAiOauthToken")).toString().trimmed();
        }
        if (error.error == QJsonParseError::NoError && !accessToken.isEmpty()) {
            fetchOAuthUsage(accessToken, oauthPlanName(oauth));
            return;
        }
    }

    const QString adminKey = m_config->providerCredential(
        QStringLiteral("anthropic"), QStringLiteral("adminKey"));
    if (!adminKey.isEmpty()) {
        emit failed(id(), QStringLiteral(
            "Usage data not available for Anthropic Admin API keys; "
            "Claude subscription quota requires Claude OAuth credentials"));
        return;
    }

    emit failed(id(), QStringLiteral(
        "Claude OAuth credentials not found; run Claude Code to authenticate"));
}

void AnthropicFetcher::fetchOAuthUsage(const QString& token,
                                       const QString& planName) {
    const QList<QPair<QByteArray, QByteArray>> headers = {
        {QByteArrayLiteral("Authorization"),
         QByteArray("Bearer ") + token.toUtf8()},
        {QByteArrayLiteral("Accept"), QByteArrayLiteral("application/json")},
        {QByteArrayLiteral("Content-Type"),
         QByteArrayLiteral("application/json")},
        {QByteArrayLiteral("anthropic-beta"),
         QByteArrayLiteral("oauth-2025-04-20")},
        {QByteArrayLiteral("User-Agent"),
         QByteArrayLiteral("claude-code/2.1.0")},
    };

    QNetworkReply* reply = httpGet(QUrl(QString::fromLatin1(kUsageUrl)), headers);
    connect(reply, &QNetworkReply::finished, this, [this, reply, planName]() {
        const int status = reply->attribute(
            QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray raw = reply->readAll();
        reply->deleteLater();

        if (status == 401 || status == 403) {
            emit failed(id(), QStringLiteral(
                "Claude OAuth token expired or invalid; re-authenticate Claude Code"));
            return;
        }
        if (status == 429) {
            emit failed(id(), QStringLiteral(
                "Claude usage endpoint is rate limited; try again in a few minutes"));
            return;
        }
        if (reply->error() != QNetworkReply::NoError || status != 200) {
            emit failed(id(), QStringLiteral("Claude usage request failed (HTTP %1): %2")
                .arg(status).arg(reply->errorString()));
            return;
        }

        QJsonParseError error;
        const QJsonDocument document = QJsonDocument::fromJson(raw, &error);
        if (error.error != QJsonParseError::NoError || !document.isObject()) {
            emit failed(id(), QStringLiteral("Invalid response from Claude usage API"));
            return;
        }

        const QJsonObject root = document.object();
        ProviderQuota quota;
        quota.id = id();
        quota.displayName = displayName();
        quota.planName = planName;

        appendUsageWindow(quota, root, QStringLiteral("five_hour"),
                          QStringLiteral("5h"));
        appendUsageWindow(quota, root, QStringLiteral("seven_day"),
                          QStringLiteral("weekly"));

        if (quota.windows.size() < 2) {
            appendUsageWindow(quota, root, QStringLiteral("seven_day_sonnet"),
                              QStringLiteral("weekly Sonnet"));
            appendUsageWindow(quota, root, QStringLiteral("seven_day_opus"),
                              QStringLiteral("weekly Opus"));
        }

        const QJsonObject extra = root.value(
            QStringLiteral("extra_usage")).toObject();
        if (extra.value(QStringLiteral("is_enabled")).toBool()) {
            const int decimalPlaces = qMax(
                0, extra.value(QStringLiteral("decimal_places")).toInt(0));
            const double scale = std::pow(10.0, decimalPlaces);
            const double used = extra.value(
                QStringLiteral("used_credits")).toDouble() / scale;
            const double limit = extra.value(
                QStringLiteral("monthly_limit")).toDouble() / scale;
            if (limit > 0.0) {
                ProviderWindow window;
                window.label = QStringLiteral("monthly extra usage");
                window.creditsUsed = used;
                window.creditsLimit = limit;
                window.usedFraction = qBound(
                    0.0,
                    extra.value(QStringLiteral("utilization"))
                        .toDouble((used / limit) * 100.0) / 100.0,
                    1.0);
                quota.windows.append(window);
            }
        }

        if (quota.windows.isEmpty()) {
            emit failed(id(), QStringLiteral(
                "Claude usage API returned no quota windows for this account"));
            return;
        }
        emit done(quota);
    });
}
