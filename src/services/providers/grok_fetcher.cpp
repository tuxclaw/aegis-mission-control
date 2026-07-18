#include "services/providers/grok_fetcher.h"

#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace {

constexpr int kLookbackDays = 30;

void countModel(QHash<QString, int>& counts, const QString& raw) {
    const QString model = raw.trimmed();
    if (!model.isEmpty())
        counts[model] += 1;
}

}  // namespace

GrokFetcher::GrokFetcher(const MonitorConfig* config,
                         QNetworkAccessManager* nam,
                         QObject* parent)
    : IProviderFetcher(nam, parent) {
    Q_UNUSED(config)
}

void GrokFetcher::fetch() {
    QString grokHome = qEnvironmentVariable("GROK_HOME").trimmed();
    if (grokHome.isEmpty())
        grokHome = QDir::homePath() + QStringLiteral("/.grok");

    const QString sessionsRoot = QDir(grokHome).filePath(
        QStringLiteral("sessions"));
    const QDateTime cutoff = QDateTime::currentDateTimeUtc().addDays(
        -kLookbackDays);

    int sessionCount = 0;
    qint64 totalTokens = 0;
    QDateTime lastSessionAt;
    QHash<QString, int> modelCounts;

    QDirIterator iterator(sessionsRoot, {QStringLiteral("signals.json")},
                          QDir::Files, QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        const QString path = iterator.next();
        const QFileInfo info = iterator.fileInfo();
        const QDateTime modified = info.lastModified().toUTC();
        if (!modified.isValid() || modified < cutoff)
            continue;

        QFile file(path);
        if (!file.open(QIODevice::ReadOnly))
            continue;
        QJsonParseError error;
        const QJsonDocument document = QJsonDocument::fromJson(
            file.readAll(), &error);
        if (error.error != QJsonParseError::NoError || !document.isObject())
            continue;

        const QJsonObject root = document.object();
        ++sessionCount;
        totalTokens += static_cast<qint64>(root.value(
            QStringLiteral("totalTokensBeforeCompaction")).toDouble());
        totalTokens += static_cast<qint64>(root.value(
            QStringLiteral("contextTokensUsed")).toDouble());

        if (!lastSessionAt.isValid() || modified > lastSessionAt)
            lastSessionAt = modified;

        countModel(modelCounts, root.value(
            QStringLiteral("primaryModelId")).toString());
        const QJsonArray models = root.value(
            QStringLiteral("modelsUsed")).toArray();
        for (const QJsonValue& value : models)
            countModel(modelCounts, value.toString());
    }

    if (sessionCount == 0) {
        emit failed(id(), QStringLiteral(
            "Usage data not available for xAI (Grok): the CLI exposes no "
            "billing API and no local session data was found"));
        return;
    }

    QString primaryModel;
    int primaryModelCount = -1;
    for (auto it = modelCounts.constBegin(); it != modelCounts.constEnd(); ++it) {
        if (it.value() > primaryModelCount) {
            primaryModel = it.key();
            primaryModelCount = it.value();
        }
    }

    ProviderQuota quota;
    quota.id = id();
    quota.displayName = displayName();
    quota.planName = primaryModel.isEmpty()
        ? QStringLiteral("Local activity (not quota)")
        : QStringLiteral("Local activity · %1").arg(primaryModel);

    ProviderWindow window;
    window.label = QStringLiteral("30d local");
    window.tokensUsed = totalTokens;
    window.usedFraction = -1.0;
    quota.windows.append(window);
    emit done(quota);
}
