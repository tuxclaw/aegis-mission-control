#include "services/providers/xiaomi_fetcher.h"

#include "services/monitor_config.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTimeZone>
#include <QUuid>
#include <QVariant>

namespace {

constexpr const char* kBaseURL =
    "https://platform.xiaomimimo.com/api/v1";
constexpr const char* kOrigin =
    "https://platform.xiaomimimo.com";
constexpr const char* kReferer =
    "https://platform.xiaomimimo.com/#/console/balance";
constexpr const char* kUserAgent =
    "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 "
    "(KHTML, like Gecko) Chrome/143.0.0.0 Safari/537.36";

const QStringList kKnownCookieNames = {
    QStringLiteral("api-platform_serviceToken"),
    QStringLiteral("userId"),
    QStringLiteral("api-platform_ph"),
    QStringLiteral("api-platform_slh"),
};

QString normalizeCookieHeader(const QString& raw) {
    QString value = raw.trimmed();
    if (value.isEmpty())
        return {};

    const QStringList patterns = {
        QStringLiteral(R"((?i)-H\s*'Cookie:\s*([^']+)')"),
        QStringLiteral(R"cookie((?i)-H\s*"Cookie:\s*([^"]+)")cookie"),
        QStringLiteral(R"((?i)\bcookie:\s*'([^']+)')"),
        QStringLiteral(R"cookie((?i)\bcookie:\s*"([^"]+)")cookie"),
        QStringLiteral(R"((?i)\bcookie:\s*([^\r\n]+))"),
        QStringLiteral(R"((?i)(?:^|\s)(?:--cookie|-b)\s*'([^']+)')"),
        QStringLiteral(R"cookie((?i)(?:^|\s)(?:--cookie|-b)\s*"([^"]+)")cookie"),
        QStringLiteral(R"((?i)(?:^|\s)-b([^\s=]+=[^\s]+))"),
        QStringLiteral(R"((?i)(?:^|\s)(?:--cookie|-b)\s+([^\s]+))"),
    };
    for (const QString& pattern : patterns) {
        const QRegularExpressionMatch match = QRegularExpression(pattern).match(value);
        if (match.hasMatch()) {
            value = match.captured(1).trimmed();
            break;
        }
    }

    if (value.startsWith(QStringLiteral("cookie:"), Qt::CaseInsensitive))
        value = value.mid(QStringLiteral("cookie:").size()).trimmed();
    if (value.size() >= 2 &&
        ((value.startsWith(QLatin1Char('\'')) && value.endsWith(QLatin1Char('\''))) ||
         (value.startsWith(QLatin1Char('"')) && value.endsWith(QLatin1Char('"'))))) {
        value = value.mid(1, value.size() - 2);
    }

    QMap<QString, QString> cookies;
    const QStringList parts = value.split(QLatin1Char(';'), Qt::SkipEmptyParts);
    for (const QString& part : parts) {
        const qsizetype equals = part.indexOf(QLatin1Char('='));
        if (equals <= 0)
            continue;
        const QString name = part.left(equals).trimmed();
        const QString cookieValue = part.mid(equals + 1).trimmed();
        if (kKnownCookieNames.contains(name) && !cookieValue.isEmpty())
            cookies[name] = cookieValue;
    }

    if (!cookies.contains(QStringLiteral("api-platform_serviceToken")) ||
        !cookies.contains(QStringLiteral("userId"))) {
        return {};
    }

    QStringList normalized;
    for (auto it = cookies.constBegin(); it != cookies.constEnd(); ++it)
        normalized.append(QStringLiteral("%1=%2").arg(it.key(), it.value()));
    return normalized.join(QStringLiteral("; "));
}

double flexibleDouble(const QJsonValue& value) {
    if (value.isDouble())
        return value.toDouble();
    return value.toString().trimmed().toDouble();
}

int flexibleInt(const QJsonValue& value) {
    if (value.isDouble())
        return value.toInt();
    return value.toString().trimmed().toInt();
}

QString responseMessage(const QByteArray& raw) {
    return QJsonDocument::fromJson(raw).object()
        .value(QStringLiteral("message")).toString().trimmed();
}

}  // namespace

XiaomiFetcher::XiaomiFetcher(const MonitorConfig* config, QNetworkAccessManager* nam,
                             QObject* parent)
    : IProviderFetcher(nam, parent), m_config(config) {}

void XiaomiFetcher::fetch() {
    resolveCookieAndFetch();
}

void XiaomiFetcher::resolveCookieAndFetch() {
    QString rawCookie = m_config->providerCredential(
        QStringLiteral("xiaomi"), QStringLiteral("cookie"));
    const bool hasConfiguredCookie = !rawCookie.trimmed().isEmpty();

    if (rawCookie.isEmpty()) {
        QString serviceToken = m_config->providerCredential(
            QStringLiteral("xiaomi"), QStringLiteral("serviceToken"));
        if (serviceToken.isEmpty()) {
            serviceToken = m_config->providerCredential(
                QStringLiteral("xiaomi"),
                QStringLiteral("api-platform_serviceToken"));
        }
        const QString userId = m_config->providerCredential(
            QStringLiteral("xiaomi"), QStringLiteral("userId"));
        if (!serviceToken.isEmpty() && !userId.isEmpty()) {
            rawCookie = QStringLiteral(
                "api-platform_serviceToken=%1; userId=%2")
                .arg(serviceToken, userId);
        }
    }

    // Fallback: try passToken from Xiaomi account cookies.
    if (rawCookie.isEmpty()) {
        const QString passToken = m_config->providerCredential(
            QStringLiteral("xiaomi"), QStringLiteral("passToken"));
        const QString userId = m_config->providerCredential(
            QStringLiteral("xiaomi"), QStringLiteral("userId"));
        if (!passToken.isEmpty() && !userId.isEmpty()) {
            rawCookie = QStringLiteral(
                "api-platform_serviceToken=%1; userId=%2")
                .arg(passToken, userId);
        }
    }

    if (rawCookie.isEmpty())
        rawCookie = readCookieFromFirefox();

    const QString cookie = normalizeCookieHeader(rawCookie);
    if (cookie.isEmpty()) {
        emit failed(id(), hasConfiguredCookie
            ? QStringLiteral(
                "Invalid Xiaomi MiMo cookie: expected api-platform_serviceToken "
                "and userId (a Cookie header or copied curl command is accepted)")
            : QStringLiteral(
                "No Xiaomi MiMo browser session found; log in at "
                "platform.xiaomimimo.com or configure its Cookie header"));
        return;
    }

    // Build shared headers
    const QList<QPair<QByteArray, QByteArray>> headers = {
        { QByteArrayLiteral("Cookie"),           cookie.toUtf8() },
        { QByteArrayLiteral("Accept"),
          QByteArrayLiteral("application/json, text/plain, */*") },
        { QByteArrayLiteral("Origin"),           QByteArrayLiteral(kOrigin) },
        { QByteArrayLiteral("Referer"),          QByteArrayLiteral(kReferer) },
        { QByteArrayLiteral("User-Agent"),       QByteArrayLiteral(kUserAgent) },
        { QByteArrayLiteral("Accept-Language"),
          QByteArrayLiteral("en-US,en;q=0.9") },
        { QByteArrayLiteral("x-timeZone"), QByteArrayLiteral("UTC+00:00") },
    };

    // Reset state
    m_pending = 3;
    m_balanceData.clear();
    m_tokenDetailData.clear();
    m_tokenUsageData.clear();
    m_balanceStatus = 0;
    m_tokenDetailStatus = 0;
    m_tokenUsageStatus = 0;

    // Fire 3 parallel requests
    QNetworkReply* r1 = httpGet(
        QUrl(QLatin1String(kBaseURL) + QStringLiteral("/balance")), headers);
    QNetworkReply* r2 = httpGet(
        QUrl(QLatin1String(kBaseURL) + QStringLiteral("/tokenPlan/detail")), headers);
    QNetworkReply* r3 = httpGet(
        QUrl(QLatin1String(kBaseURL) + QStringLiteral("/tokenPlan/usage")), headers);

    connect(r1, &QNetworkReply::finished, this, [this, r1]() {
        r1->deleteLater();
        m_balanceStatus = r1->attribute(
            QNetworkRequest::HttpStatusCodeAttribute).toInt();
        m_balanceData = r1->readAll();
        m_pending--;
        if (m_pending <= 0) onAllRepliesFinished();
    });
    connect(r2, &QNetworkReply::finished, this, [this, r2]() {
        r2->deleteLater();
        m_tokenDetailStatus = r2->attribute(
            QNetworkRequest::HttpStatusCodeAttribute).toInt();
        m_tokenDetailData = r2->readAll();
        m_pending--;
        if (m_pending <= 0) onAllRepliesFinished();
    });
    connect(r3, &QNetworkReply::finished, this, [this, r3]() {
        r3->deleteLater();
        m_tokenUsageStatus = r3->attribute(
            QNetworkRequest::HttpStatusCodeAttribute).toInt();
        m_tokenUsageData = r3->readAll();
        m_pending--;
        if (m_pending <= 0) onAllRepliesFinished();
    });
}

QString XiaomiFetcher::readCookieFromFirefox() const {
    const QString firefoxDir = QDir::homePath()
        + QStringLiteral("/.mozilla/firefox");
    QDirIterator databases(firefoxDir, {QStringLiteral("cookies.sqlite")},
                           QDir::Files, QDirIterator::Subdirectories);
    while (databases.hasNext()) {
        const QString dbPath = databases.next();
        const QString connectionName = QStringLiteral("xiaomi-cookie-%1")
            .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
        QString cookieHeader;
        {
            QSqlDatabase db = QSqlDatabase::addDatabase(
                QStringLiteral("QSQLITE"), connectionName);
            db.setDatabaseName(dbPath);
            db.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY"));
            if (!db.open()) {
                qWarning() << "XiaomiFetcher: cannot open" << dbPath
                           << db.lastError().text();
            } else {
                QMap<QString, QString> values;
                QSqlQuery query(db);
                query.prepare(QStringLiteral(
                    "SELECT name, value FROM moz_cookies "
                    "WHERE host LIKE '%xiaomimimo.com%'"));
                if (!query.exec()) {
                    qWarning() << "XiaomiFetcher: cookie query failed"
                               << query.lastError().text();
                } else {
                    while (query.next()) {
                        const QString name = query.value(0).toString();
                        const QString value = query.value(1).toString();
                        if (kKnownCookieNames.contains(name) && !value.isEmpty())
                            values[name] = value;
                    }
                    QStringList pairs;
                    for (auto it = values.constBegin(); it != values.constEnd(); ++it)
                        pairs.append(QStringLiteral("%1=%2").arg(it.key(), it.value()));
                    cookieHeader = normalizeCookieHeader(
                        pairs.join(QStringLiteral("; ")));
                }
                db.close();
            }
        }
        QSqlDatabase::removeDatabase(connectionName);

        if (!cookieHeader.isEmpty())
            return cookieHeader;
    }

    return {};
}

void XiaomiFetcher::onAllRepliesFinished() {
    // Check for session expiry (3xx, 401, 403) on the required balance request.
    if (m_balanceStatus >= 300 && m_balanceStatus < 400) {
        emit failed(id(), QStringLiteral(
            "Session expired \u2014 log in again"));
        return;
    }
    if (m_balanceStatus == 401 || m_balanceStatus == 403) {
        emit failed(id(), QStringLiteral(
            "Session expired \u2014 log in again"));
        return;
    }

    ProviderQuota quota;
    quota.id = id();
    quota.displayName = displayName();

    // Parse balance (required for success)
    bool balanceOk = false;
    QString apiError;
    if (m_balanceStatus == 200 && !m_balanceData.isEmpty()) {
        const QJsonDocument doc = QJsonDocument::fromJson(m_balanceData);
        const QJsonObject root = doc.object();
        const int code = flexibleInt(root.value(QStringLiteral("code")));
        if (code == 401 || code == 403) {
            emit failed(id(), QStringLiteral("Xiaomi MiMo session expired; log in again"));
            return;
        }
        if (code == 0) {
            const QJsonObject data =
                root.value(QStringLiteral("data")).toObject();
            quota.balance.total = flexibleDouble(
                data.value(QStringLiteral("balance")));
            quota.balance.cash = flexibleDouble(
                data.value(QStringLiteral("cashBalance")));
            quota.balance.gift = flexibleDouble(
                data.value(QStringLiteral("giftBalance")));
            quota.balance.currency =
                data.value(QStringLiteral("currency")).toString();
            if (quota.balance.currency.isEmpty())
                quota.balance.currency = QStringLiteral("CNY");
            balanceOk = true;
        } else {
            apiError = root.value(QStringLiteral("message")).toString().trimmed();
        }
    } else {
        apiError = responseMessage(m_balanceData);
    }

    // Parse token plan detail (optional)
    bool detailOk = false;
    if (m_tokenDetailStatus == 200 && !m_tokenDetailData.isEmpty()) {
        const QJsonDocument doc = QJsonDocument::fromJson(m_tokenDetailData);
        const QJsonObject root = doc.object();
        if (flexibleInt(root.value(QStringLiteral("code"))) == 0) {
            const QJsonObject data =
                root.value(QStringLiteral("data")).toObject();
            const QString planCode =
                data.value(QStringLiteral("planCode")).toString();
            if (!planCode.isEmpty())
                quota.planName = planCode;

            const QString periodEnd =
                data.value(QStringLiteral("currentPeriodEnd")).toString();
            detailOk = !planCode.isEmpty() || !periodEnd.isEmpty();
            if (!periodEnd.isEmpty()) {
                ProviderWindow w;
                w.label = QStringLiteral("monthly");
                w.usedFraction = -1.0;
                w.resetsAt = QDateTime::fromString(
                    periodEnd, QStringLiteral("yyyy-MM-dd HH:mm:ss"));
                if (w.resetsAt.isValid())
                    w.resetsAt.setTimeZone(QTimeZone::utc());
                // Will be filled with token usage below; append as placeholder
                quota.windows.append(w);
            }
        }
    }

    // Parse token plan usage (optional)
    bool usageOk = false;
    if (m_tokenUsageStatus == 200 && !m_tokenUsageData.isEmpty()) {
        const QJsonDocument doc = QJsonDocument::fromJson(m_tokenUsageData);
        const QJsonObject root = doc.object();
        if (flexibleInt(root.value(QStringLiteral("code"))) == 0) {
            const QJsonObject data =
                root.value(QStringLiteral("data")).toObject();
            const QJsonObject monthUsage =
                data.value(QStringLiteral("monthUsage")).toObject();
            const QJsonArray items =
                monthUsage.value(QStringLiteral("items")).toArray();
            if (!items.isEmpty()) {
                usageOk = true;
                const QJsonObject item = items.first().toObject();
                ProviderWindow w;
                w.label = QStringLiteral("monthly");
                w.tokensUsed = static_cast<qint64>(
                    item.value(QStringLiteral("used")).toDouble());
                w.tokensLimit = static_cast<qint64>(
                    item.value(QStringLiteral("limit")).toDouble());
                w.usedFraction = qBound(
                    0.0, item.value(QStringLiteral("percent")).toDouble() / 100.0,
                    1.0);
                // Copy resetsAt from the detail window if present
                if (!quota.windows.isEmpty())
                    w.resetsAt = quota.windows.first().resetsAt;
                if (quota.windows.isEmpty())
                    quota.windows.append(w);
                else
                    quota.windows.first() = w;
            }
        }
    }

    if (!balanceOk && !detailOk && !usageOk) {
        if (apiError.isEmpty()) {
            apiError = QStringLiteral("HTTP %1").arg(m_balanceStatus);
        }
        emit failed(id(), QStringLiteral("Xiaomi MiMo usage request failed: %1")
            .arg(apiError));
        return;
    }

    // If balance failed but tokens succeeded, still emit with balance=0
    if (!balanceOk)
        quota.balance = ProviderBalance{};

    emit done(quota);
}
