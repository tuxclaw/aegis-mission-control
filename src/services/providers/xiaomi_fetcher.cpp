#include "services/providers/xiaomi_fetcher.h"

#include "services/monitor_config.h"

#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTimeZone>
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

}  // namespace

XiaomiFetcher::XiaomiFetcher(const MonitorConfig* config, QNetworkAccessManager* nam,
                             QObject* parent)
    : IProviderFetcher(nam, parent), m_config(config) {}

void XiaomiFetcher::fetch() {
    resolveCookieAndFetch();
}

void XiaomiFetcher::resolveCookieAndFetch() {
    QString cookie = m_config->providerCredential(
        QStringLiteral("xiaomi"), QStringLiteral("cookie"));

    if (cookie.isEmpty())
        cookie = readCookieFromFirefox();

    // Normalize: trim and validate required cookies
    cookie = cookie.trimmed();
    if (cookie.isEmpty()) {
        emit failed(id(), QStringLiteral(
            "Missing api-platform_serviceToken or userId cookie"));
        return;
    }
    if (!cookie.contains(QLatin1String("api-platform_serviceToken")) ||
        !cookie.contains(QLatin1String("userId"))) {
        emit failed(id(), QStringLiteral(
            "Missing api-platform_serviceToken or userId cookie"));
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
    QDir dir(firefoxDir);
    const QStringList profiles = dir.entryList(
        {QStringLiteral("*.default-release")}, QDir::Dirs | QDir::NoDotAndDotDot);

    for (const QString& profile : profiles) {
        const QString dbPath = firefoxDir + QLatin1Char('/') + profile
            + QStringLiteral("/cookies.sqlite");
        if (!QFile::exists(dbPath))
            continue;

        QString serviceToken, userId;
        {
            QSqlDatabase db = QSqlDatabase::addDatabase(
                QStringLiteral("QSQLITE"), QStringLiteral("xiaomi-cookie"));
            db.setDatabaseName(dbPath);
            db.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY"));
            if (!db.open()) {
                qWarning() << "XiaomiFetcher: cannot open" << dbPath
                           << db.lastError().text();
                QSqlDatabase::removeDatabase(QStringLiteral("xiaomi-cookie"));
                continue;
            }

            QSqlQuery query(db);
            query.prepare(
                QStringLiteral(
                    "SELECT name, value FROM moz_cookies "
                    "WHERE host LIKE '%xiaomimimo.com%'"));
            if (!query.exec()) {
                qWarning() << "XiaomiFetcher: cookie query failed"
                           << query.lastError().text();
                db.close();
                QSqlDatabase::removeDatabase(QStringLiteral("xiaomi-cookie"));
                continue;
            }

            while (query.next()) {
                const QString name = query.value(0).toString();
                const QString value = query.value(1).toString();
                if (name == QLatin1String("api-platform_serviceToken"))
                    serviceToken = value;
                else if (name == QLatin1String("userId"))
                    userId = value;
            }

            db.close();
        }
        QSqlDatabase::removeDatabase(QStringLiteral("xiaomi-cookie"));

        if (!serviceToken.isEmpty() && !userId.isEmpty()) {
            return QStringLiteral("api-platform_serviceToken=%1; userId=%2")
                .arg(serviceToken, userId);
        }
    }

    return {};
}

void XiaomiFetcher::onAllRepliesFinished() {
    // Check for session expiry (3xx, 401, 403) on balance request
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
    if (m_balanceStatus == 200 && !m_balanceData.isEmpty()) {
        const QJsonDocument doc = QJsonDocument::fromJson(m_balanceData);
        const QJsonObject root = doc.object();
        if (root.value(QStringLiteral("code")).toInt() == 0) {
            const QJsonObject data =
                root.value(QStringLiteral("data")).toObject();
            quota.balance.total =
                data.value(QStringLiteral("balance")).toString().toDouble();
            quota.balance.cash =
                data.value(QStringLiteral("cashBalance")).toString().toDouble();
            quota.balance.gift =
                data.value(QStringLiteral("giftBalance")).toString().toDouble();
            quota.balance.currency =
                data.value(QStringLiteral("currency")).toString();
            if (quota.balance.currency.isEmpty())
                quota.balance.currency = QStringLiteral("CNY");
            balanceOk = true;
        }
    }

    // Parse token plan detail (optional)
    if (m_tokenDetailStatus == 200 && !m_tokenDetailData.isEmpty()) {
        const QJsonDocument doc = QJsonDocument::fromJson(m_tokenDetailData);
        const QJsonObject root = doc.object();
        if (root.value(QStringLiteral("code")).toInt() == 0) {
            const QJsonObject data =
                root.value(QStringLiteral("data")).toObject();
            const QString planCode =
                data.value(QStringLiteral("planCode")).toString();
            if (!planCode.isEmpty())
                quota.planName = planCode;

            const QString periodEnd =
                data.value(QStringLiteral("currentPeriodEnd")).toString();
            if (!periodEnd.isEmpty()) {
                ProviderWindow w;
                w.label = QStringLiteral("monthly");
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
    if (m_tokenUsageStatus == 200 && !m_tokenUsageData.isEmpty()) {
        const QJsonDocument doc = QJsonDocument::fromJson(m_tokenUsageData);
        const QJsonObject root = doc.object();
        if (root.value(QStringLiteral("code")).toInt() == 0) {
            const QJsonObject data =
                root.value(QStringLiteral("data")).toObject();
            const QJsonObject monthUsage =
                data.value(QStringLiteral("monthUsage")).toObject();
            const QJsonArray items =
                monthUsage.value(QStringLiteral("items")).toArray();
            if (!items.isEmpty()) {
                const QJsonObject item = items.first().toObject();
                ProviderWindow w;
                w.label = QStringLiteral("monthly");
                w.tokensUsed = static_cast<qint64>(
                    item.value(QStringLiteral("used")).toDouble());
                w.tokensLimit = static_cast<qint64>(
                    item.value(QStringLiteral("limit")).toDouble());
                w.usedFraction =
                    item.value(QStringLiteral("percent")).toDouble() / 100.0;
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

    // If all three failed, emit failed
    const bool allFailed =
        (m_balanceStatus != 200 || m_balanceData.isEmpty()) &&
        (m_tokenDetailStatus != 200 || m_tokenDetailData.isEmpty()) &&
        (m_tokenUsageStatus != 200 || m_tokenUsageData.isEmpty());

    if (allFailed && !balanceOk) {
        emit failed(id(), QStringLiteral("All requests failed"));
        return;
    }

    // If balance failed but tokens succeeded, still emit with balance=0
    if (!balanceOk)
        quota.balance = ProviderBalance{};

    emit done(quota);
}
