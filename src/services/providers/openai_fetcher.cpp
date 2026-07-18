#include "services/providers/openai_fetcher.h"

#include "services/monitor_config.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QStringList>
#include <QTimeZone>

namespace {

constexpr auto kCodexUsageUrl = "https://chatgpt.com/backend-api/wham/usage";

QString tokenValue(const QJsonObject& tokens, const QString& snakeCase,
                   const QString& camelCase) {
    QString value = tokens.value(snakeCase).toString().trimmed();
    if (value.isEmpty())
        value = tokens.value(camelCase).toString().trimmed();
    return value;
}

QString windowLabel(int seconds, const QString& fallback) {
    constexpr int kWeekSeconds = 7 * 24 * 60 * 60;
    constexpr int kMonthSeconds = 30 * 24 * 60 * 60;
    if (seconds >= kMonthSeconds)
        return QStringLiteral("monthly");
    if (seconds >= kWeekSeconds)
        return QStringLiteral("weekly");
    if (seconds > 0 && seconds % 3600 == 0)
        return QStringLiteral("%1h").arg(seconds / 3600);
    return fallback;
}

void appendRateWindow(ProviderQuota& quota, const QJsonObject& value,
                      const QString& fallbackLabel) {
    if (value.isEmpty())
        return;

    ProviderWindow window;
    window.usedFraction = qBound(
        0.0, value.value(QStringLiteral("used_percent")).toDouble() / 100.0,
        1.0);
    const int seconds = value.value(
        QStringLiteral("limit_window_seconds")).toInt();
    window.label = windowLabel(seconds, fallbackLabel);
    const qint64 resetAt = static_cast<qint64>(value.value(
        QStringLiteral("reset_at")).toDouble());
    if (resetAt > 0)
        window.resetsAt = QDateTime::fromSecsSinceEpoch(resetAt, QTimeZone::UTC);
    quota.windows.append(window);
}

double flexibleDouble(const QJsonValue& value) {
    if (value.isDouble())
        return value.toDouble();
    return value.toString().trimmed().toDouble();
}

}  // namespace

OpenAIFetcher::OpenAIFetcher(const MonitorConfig* config,
                             QNetworkAccessManager* nam,
                             QObject* parent)
    : IProviderFetcher(nam, parent), m_config(config) {}

void OpenAIFetcher::fetch() {
    QStringList authPaths;
    const QString codexHome = qEnvironmentVariable("CODEX_HOME").trimmed();
    if (!codexHome.isEmpty())
        authPaths.append(QDir(codexHome).filePath(QStringLiteral("auth.json")));
    authPaths.append(QDir::homePath() + QStringLiteral("/.codex/auth.json"));
    authPaths.append(QDir::homePath()
        + QStringLiteral("/.openclaw/acpx/codex-home/auth.json"));

    for (const QString& path : authPaths) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly))
            continue;

        QJsonParseError error;
        const QJsonDocument document = QJsonDocument::fromJson(
            file.readAll(), &error);
        if (error.error != QJsonParseError::NoError || !document.isObject())
            continue;

        const QJsonObject tokens = document.object().value(
            QStringLiteral("tokens")).toObject();
        const QString accessToken = tokenValue(
            tokens, QStringLiteral("access_token"),
            QStringLiteral("accessToken"));
        if (accessToken.isEmpty())
            continue;

        const QString accountId = tokenValue(
            tokens, QStringLiteral("account_id"),
            QStringLiteral("accountId"));
        fetchOAuthUsage(accessToken, accountId);
        return;
    }

    if (!m_config->providerCredential(
            QStringLiteral("openai"), QStringLiteral("apiKey")).isEmpty()) {
        emit failed(id(), QStringLiteral(
            "Usage data not available for OpenAI API-key auth; "
            "Codex subscription quota requires Codex OAuth credentials"));
        return;
    }

    emit failed(id(), QStringLiteral(
        "Codex OAuth credentials not found; sign in with the Codex CLI"));
}

void OpenAIFetcher::fetchOAuthUsage(const QString& accessToken,
                                    const QString& accountId) {
    QList<QPair<QByteArray, QByteArray>> headers = {
        {QByteArrayLiteral("Authorization"),
         QByteArray("Bearer ") + accessToken.toUtf8()},
        {QByteArrayLiteral("Accept"), QByteArrayLiteral("application/json")},
        {QByteArrayLiteral("User-Agent"), QByteArrayLiteral("AEGIS")},
    };
    if (!accountId.isEmpty()) {
        headers.append({QByteArrayLiteral("ChatGPT-Account-Id"),
                        accountId.toUtf8()});
    }

    QNetworkReply* reply = httpGet(
        QUrl(QString::fromLatin1(kCodexUsageUrl)), headers);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        const int status = reply->attribute(
            QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray raw = reply->readAll();
        reply->deleteLater();

        if (status == 401 || status == 403) {
            emit failed(id(), QStringLiteral(
                "Codex OAuth token expired or invalid; re-authenticate Codex"));
            return;
        }
        if (status == 429) {
            emit failed(id(), QStringLiteral(
                "Codex usage endpoint is rate limited; try again later"));
            return;
        }
        if (reply->error() != QNetworkReply::NoError || status < 200 ||
            status >= 300) {
            emit failed(id(), QStringLiteral("Codex usage request failed (HTTP %1): %2")
                .arg(status).arg(reply->errorString()));
            return;
        }

        QJsonParseError error;
        const QJsonDocument document = QJsonDocument::fromJson(raw, &error);
        if (error.error != QJsonParseError::NoError || !document.isObject()) {
            emit failed(id(), QStringLiteral("Invalid response from Codex usage API"));
            return;
        }

        const QJsonObject root = document.object();
        ProviderQuota quota;
        quota.id = id();
        quota.displayName = displayName();
        quota.planName = root.value(QStringLiteral("plan_type"))
            .toString(QStringLiteral("Codex OAuth"));

        const QJsonObject rateLimit = root.value(
            QStringLiteral("rate_limit")).toObject();
        appendRateWindow(quota, rateLimit.value(
            QStringLiteral("primary_window")).toObject(),
            QStringLiteral("primary"));
        appendRateWindow(quota, rateLimit.value(
            QStringLiteral("secondary_window")).toObject(),
            QStringLiteral("weekly"));

        const QJsonObject credits = root.value(
            QStringLiteral("credits")).toObject();
        const bool hasCredits = credits.value(
            QStringLiteral("has_credits")).toBool();
        if (hasCredits) {
            quota.balance.total = flexibleDouble(
                credits.value(QStringLiteral("balance")));
            quota.balance.currency = QStringLiteral("USD");
        }

        if (quota.windows.isEmpty() && !hasCredits) {
            emit failed(id(), QStringLiteral(
                "Codex usage API returned no quota windows"));
            return;
        }
        emit done(quota);
    });
}
