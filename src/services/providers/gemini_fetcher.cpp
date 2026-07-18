#include "services/providers/gemini_fetcher.h"

#include "services/monitor_config.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>

#include <utility>

namespace {

constexpr auto kLoadCodeAssistUrl =
    "https://cloudcode-pa.googleapis.com/v1internal:loadCodeAssist";
constexpr auto kQuotaUrl =
    "https://cloudcode-pa.googleapis.com/v1internal:retrieveUserQuota";

double flexibleDouble(const QJsonValue& value) {
    if (value.isDouble())
        return value.toDouble();
    return value.toString().trimmed().toDouble();
}

QString projectIdFromResponse(const QJsonObject& root) {
    const QJsonValue project = root.value(
        QStringLiteral("cloudaicompanionProject"));
    if (project.isString())
        return project.toString().trimmed();
    const QJsonObject object = project.toObject();
    QString id = object.value(QStringLiteral("id")).toString().trimmed();
    if (id.isEmpty())
        id = object.value(QStringLiteral("projectId")).toString().trimmed();
    return id;
}

QString planNameFromResponse(const QJsonObject& root) {
    const QString paidName = root.value(QStringLiteral("paidTier"))
        .toObject().value(QStringLiteral("name")).toString().trimmed();
    if (!paidName.isEmpty())
        return paidName;

    const QJsonObject tier = root.value(
        QStringLiteral("currentTier")).toObject();
    QString value = tier.value(QStringLiteral("name")).toString().trimmed();
    if (value.isEmpty())
        value = tier.value(QStringLiteral("id")).toString().trimmed();
    return value.isEmpty() ? QStringLiteral("Gemini CLI OAuth") : value;
}

}  // namespace

GeminiFetcher::GeminiFetcher(const MonitorConfig* config,
                             QNetworkAccessManager* nam,
                             QObject* parent)
    : IProviderFetcher(nam, parent), m_config(config) {}

void GeminiFetcher::fetch() {
    const QString settingsPath = QDir::homePath()
        + QStringLiteral("/.gemini/settings.json");
    QFile settingsFile(settingsPath);
    if (settingsFile.open(QIODevice::ReadOnly)) {
        const QJsonObject auth = QJsonDocument::fromJson(
            settingsFile.readAll()).object()
            .value(QStringLiteral("security")).toObject()
            .value(QStringLiteral("auth")).toObject();
        const QString selectedType = auth.value(
            QStringLiteral("selectedType")).toString();
        if (selectedType == QStringLiteral("api-key") ||
            selectedType == QStringLiteral("gemini-api-key")) {
            emit failed(id(), QStringLiteral(
                "Usage data not available for Gemini API-key auth; "
                "Google exposes quota through Gemini CLI OAuth or Cloud Console"));
            return;
        }
        if (selectedType == QStringLiteral("vertex-ai")) {
            emit failed(id(), QStringLiteral(
                "Usage data not available for Gemini Vertex AI auth in AEGIS"));
            return;
        }
    }

    QFile credentials(QDir::homePath()
        + QStringLiteral("/.gemini/oauth_creds.json"));
    if (!credentials.open(QIODevice::ReadOnly)) {
        const bool hasApiKey = !m_config->providerCredential(
            QStringLiteral("gemini"), QStringLiteral("apiKey")).isEmpty()
            || !qEnvironmentVariable("GEMINI_API_KEY").isEmpty();
        emit failed(id(), hasApiKey
            ? QStringLiteral(
                "Usage data not available for Gemini API-key auth; "
                "Google exposes quota through Gemini CLI OAuth or Cloud Console")
            : QStringLiteral(
                "Gemini CLI OAuth credentials not found; authenticate the Gemini CLI"));
        return;
    }

    QJsonParseError error;
    const QJsonObject root = QJsonDocument::fromJson(
        credentials.readAll(), &error).object();
    const QString accessToken = root.value(
        QStringLiteral("access_token")).toString().trimmed();
    const qint64 expiryMs = static_cast<qint64>(flexibleDouble(
        root.value(QStringLiteral("expiry_date"))));
    if (error.error != QJsonParseError::NoError || accessToken.isEmpty()) {
        emit failed(id(), QStringLiteral(
            "Gemini CLI OAuth credentials are invalid; re-authenticate the Gemini CLI"));
        return;
    }
    if (expiryMs > 0 && expiryMs <= QDateTime::currentMSecsSinceEpoch()) {
        emit failed(id(), QStringLiteral(
            "Gemini CLI OAuth token expired; re-authenticate the Gemini CLI"));
        return;
    }

    fetchCodeAssist(accessToken);
}

void GeminiFetcher::fetchCodeAssist(const QString& accessToken) {
    QJsonObject metadata;
    metadata[QStringLiteral("ideType")] = QStringLiteral("GEMINI_CLI");
    metadata[QStringLiteral("pluginType")] = QStringLiteral("GEMINI");
    QJsonObject body;
    body[QStringLiteral("metadata")] = metadata;

    const QList<QPair<QByteArray, QByteArray>> headers = {
        {QByteArrayLiteral("Authorization"),
         QByteArray("Bearer ") + accessToken.toUtf8()},
        {QByteArrayLiteral("Content-Type"),
         QByteArrayLiteral("application/json")},
        {QByteArrayLiteral("Accept"), QByteArrayLiteral("application/json")},
    };
    QNetworkReply* reply = httpPost(
        QUrl(QString::fromLatin1(kLoadCodeAssistUrl)),
        QJsonDocument(body).toJson(QJsonDocument::Compact), headers);
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, accessToken]() {
        const int status = reply->attribute(
            QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray raw = reply->readAll();
        reply->deleteLater();

        if (status == 401 || status == 403) {
            emit failed(id(), QStringLiteral(
                "Gemini CLI OAuth token expired or invalid; re-authenticate Gemini"));
            return;
        }

        QString projectId;
        QString planName = QStringLiteral("Gemini CLI OAuth");
        if (status == 200) {
            const QJsonObject root = QJsonDocument::fromJson(raw).object();
            projectId = projectIdFromResponse(root);
            planName = planNameFromResponse(root);
        }
        fetchQuota(accessToken, projectId, planName);
    });
}

void GeminiFetcher::fetchQuota(const QString& accessToken,
                               const QString& projectId,
                               const QString& planName) {
    QJsonObject body;
    if (!projectId.isEmpty())
        body[QStringLiteral("project")] = projectId;

    const QList<QPair<QByteArray, QByteArray>> headers = {
        {QByteArrayLiteral("Authorization"),
         QByteArray("Bearer ") + accessToken.toUtf8()},
        {QByteArrayLiteral("Content-Type"),
         QByteArrayLiteral("application/json")},
        {QByteArrayLiteral("Accept"), QByteArrayLiteral("application/json")},
    };
    QNetworkReply* reply = httpPost(
        QUrl(QString::fromLatin1(kQuotaUrl)),
        QJsonDocument(body).toJson(QJsonDocument::Compact), headers);
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, planName]() {
        const int status = reply->attribute(
            QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray raw = reply->readAll();
        reply->deleteLater();

        if (status == 401 || status == 403) {
            emit failed(id(), QStringLiteral(
                "Gemini CLI OAuth token expired or invalid; re-authenticate Gemini"));
            return;
        }
        if (status == 429) {
            emit failed(id(), QStringLiteral(
                "Gemini quota endpoint is rate limited; try again later"));
            return;
        }
        if (reply->error() != QNetworkReply::NoError || status != 200) {
            emit failed(id(), QStringLiteral("Gemini quota request failed (HTTP %1): %2")
                .arg(status).arg(reply->errorString()));
            return;
        }

        QJsonParseError error;
        const QJsonDocument document = QJsonDocument::fromJson(raw, &error);
        const QJsonArray buckets = document.object().value(
            QStringLiteral("buckets")).toArray();
        if (error.error != QJsonParseError::NoError || buckets.isEmpty()) {
            emit failed(id(), QStringLiteral(
                "Gemini quota API returned no quota buckets"));
            return;
        }

        struct ModelQuota {
            double remaining = 1.0;
            QString reset;
        };
        QHash<QString, ModelQuota> byModel;
        for (const QJsonValue& value : buckets) {
            const QJsonObject bucket = value.toObject();
            const QString model = bucket.value(
                QStringLiteral("modelId")).toString().trimmed();
            if (model.isEmpty() || !bucket.contains(
                    QStringLiteral("remainingFraction"))) {
                continue;
            }
            const double remaining = qBound(
                0.0, bucket.value(QStringLiteral("remainingFraction"))
                    .toDouble(1.0), 1.0);
            if (!byModel.contains(model) ||
                remaining < byModel.value(model).remaining) {
                byModel[model] = {
                    remaining,
                    bucket.value(QStringLiteral("resetTime")).toString(),
                };
            }
        }

        auto lowestFor = [&byModel](const QString& contains,
                                    const QString& excludes = QString()) {
            ModelQuota result;
            bool found = false;
            for (auto it = byModel.constBegin(); it != byModel.constEnd(); ++it) {
                const QString model = it.key().toLower();
                if (!model.contains(contains) ||
                    (!excludes.isEmpty() && model.contains(excludes))) {
                    continue;
                }
                if (!found || it.value().remaining < result.remaining) {
                    result = it.value();
                    found = true;
                }
            }
            return qMakePair(found, result);
        };

        ProviderQuota quota;
        quota.id = id();
        quota.displayName = displayName();
        quota.planName = planName;
        auto append = [&quota](const QPair<bool, ModelQuota>& match,
                               const QString& label) {
            if (!match.first)
                return;
            ProviderWindow window;
            window.label = label;
            window.usedFraction = 1.0 - match.second.remaining;
            if (!match.second.reset.isEmpty()) {
                window.resetsAt = QDateTime::fromString(
                    match.second.reset, Qt::ISODate);
            }
            quota.windows.append(window);
        };
        append(lowestFor(QStringLiteral("pro")), QStringLiteral("Pro"));
        append(lowestFor(QStringLiteral("flash"), QStringLiteral("flash-lite")),
               QStringLiteral("Flash"));
        append(lowestFor(QStringLiteral("flash-lite")),
               QStringLiteral("Flash Lite"));

        if (quota.windows.isEmpty() && !byModel.isEmpty()) {
            ModelQuota lowest;
            bool found = false;
            for (const ModelQuota& value : std::as_const(byModel)) {
                if (!found || value.remaining < lowest.remaining) {
                    lowest = value;
                    found = true;
                }
            }
            append(qMakePair(found, lowest), QStringLiteral("daily"));
        }
        if (quota.windows.isEmpty()) {
            emit failed(id(), QStringLiteral(
                "Gemini quota API returned no recognized model quotas"));
            return;
        }
        emit done(quota);
    });
}
