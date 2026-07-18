#include "services/providers/codexbar_fetcher.h"

#include "services/monitor_config.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTemporaryDir>
#include <QTimer>

CodexBarFetcher::CodexBarFetcher(const MonitorConfig* config,
                                 const QString& providerId,
                                 const QString& displayName,
                                 QNetworkAccessManager* nam,
                                 QObject* parent)
    : IProviderFetcher(nam, parent)
    , m_config(config)
    , m_providerId(providerId)
    , m_displayName(displayName)
{}

// Maps OpenClaw provider IDs to the names codexbar --provider accepts
QString CodexBarFetcher::codexbarProviderName() const {
    if (m_providerId == QStringLiteral("openai"))    return QStringLiteral("codex");
    if (m_providerId == QStringLiteral("xai"))       return QStringLiteral("grok");
    if (m_providerId == QStringLiteral("anthropic")) return QStringLiteral("claude");
    return m_providerId;
}

QString CodexBarFetcher::codexAuthHomePath() const {
    const QStringList candidates = {
        QDir::homePath() + QStringLiteral("/.codex"),
        qEnvironmentVariable("CODEX_HOME"),
        QDir::homePath() + QStringLiteral("/.openclaw/acpx/codex-home")
    };

    for (const QString& home : candidates) {
        if (home.trimmed().isEmpty())
            continue;
        QFile authFile(QDir(home).filePath(QStringLiteral("auth.json")));
        if (!authFile.open(QIODevice::ReadOnly))
            continue;
        const QJsonObject root = QJsonDocument::fromJson(authFile.readAll()).object();
        const QJsonObject tokens = root.value(QStringLiteral("tokens")).toObject();
        if (tokens.contains(QStringLiteral("access_token")) &&
            tokens.contains(QStringLiteral("refresh_token")))
            return home;
    }

    return QString();
}

std::shared_ptr<QTemporaryDir> CodexBarFetcher::oauthOnlyCodexHome(const QString& authHome) const {
    QFile authFile(QDir(authHome).filePath(QStringLiteral("auth.json")));
    if (!authFile.open(QIODevice::ReadOnly))
        return {};

    QJsonObject root = QJsonDocument::fromJson(authFile.readAll()).object();
    if (!root.contains(QStringLiteral("OPENAI_API_KEY")) ||
        root.value(QStringLiteral("tokens")).toObject().isEmpty())
        return {};

    root.remove(QStringLiteral("OPENAI_API_KEY"));

    auto tempHome = std::make_shared<QTemporaryDir>();
    if (!tempHome->isValid())
        return {};
    QFile::setPermissions(tempHome->path(), QFileDevice::ReadOwner |
                                                QFileDevice::WriteOwner |
                                                QFileDevice::ExeOwner);

    QFile tempAuth(QDir(tempHome->path()).filePath(QStringLiteral("auth.json")));
    if (!tempAuth.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return {};
    tempAuth.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
    tempAuth.close();
    QFile::setPermissions(tempAuth.fileName(), QFileDevice::ReadOwner |
                                               QFileDevice::WriteOwner);
    return tempHome;
}

// Converts windowMinutes to a human-readable label, e.g. 300→"5h", 10080→"weekly"
QString CodexBarFetcher::windowLabel(int windowMinutes, const QString& fallback) {
    if (windowMinutes <= 0)
        return fallback;

    constexpr int kWeekMinutes  = 7 * 24 * 60;
    constexpr int kMonthMinutes = 30 * 24 * 60;

    if (windowMinutes >= kMonthMinutes)
        return QStringLiteral("monthly");
    if (windowMinutes >= kWeekMinutes)
        return QStringLiteral("weekly");
    if (windowMinutes % 60 == 0)
        return QStringLiteral("%1h").arg(windowMinutes / 60);
    return QStringLiteral("%1m").arg(windowMinutes);
}

void CodexBarFetcher::fetch() {
    QString cliPath = m_config->providerCredential(
        QStringLiteral("codexbar"), QStringLiteral("cliPath"));
    if (cliPath.isEmpty())
        cliPath = QStringLiteral("codexbar");

    const QString provider = codexbarProviderName();
    std::shared_ptr<QTemporaryDir> tempCodexHome;
    auto* process = new QProcess(this);
    process->setProcessChannelMode(QProcess::SeparateChannels);
    if (provider == QStringLiteral("codex")) {
        const QString authHome = codexAuthHomePath();
        if (!authHome.isEmpty()) {
            tempCodexHome = oauthOnlyCodexHome(authHome);
            QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
            env.insert(QStringLiteral("CODEX_HOME"),
                       tempCodexHome ? tempCodexHome->path() : authHome);
            process->setProcessEnvironment(env);
        }
    }

    connect(process, &QProcess::finished, this,
        [this, process, tempCodexHome](int exitCode, QProcess::ExitStatus) {
            onProcessFinished(exitCode, process);
        });
    connect(process, &QProcess::errorOccurred, this,
        [this, process, tempCodexHome](QProcess::ProcessError err) {
            if (err == QProcess::FailedToStart) {
                emit failed(id(),
                    QStringLiteral("Cannot start codexbar: %1").arg(process->errorString()));
                process->deleteLater();
            }
        });

    // 30-second timeout — disconnect before kill to prevent double-emit
    QTimer::singleShot(30000, process, [this, process, tempCodexHome]() {
        if (process->state() == QProcess::NotRunning)
            return;
        process->disconnect();
        process->kill();
        process->deleteLater();
        emit failed(id(), QStringLiteral("codexbar timed out"));
    });

    const QStringList args = {
        QStringLiteral("usage"),
        QStringLiteral("--format"), QStringLiteral("json"),
        QStringLiteral("--provider"), provider,
        QStringLiteral("--no-color")
    };
    process->start(cliPath, args);
}

void CodexBarFetcher::onProcessFinished(int exitCode, QProcess* process) {
    process->deleteLater();
    const QByteArray raw = process->readAllStandardOutput().trimmed();

    if (!raw.isEmpty()) {
        parseJsonArray(raw);
        return;
    }

    const QString errOut = QString::fromUtf8(process->readAllStandardError()).trimmed();
    const QString msg = errOut.isEmpty()
        ? QStringLiteral("codexbar exited %1 with no output").arg(exitCode)
        : errOut;
    emit failed(id(), msg);
}

void CodexBarFetcher::parseJsonArray(const QByteArray& raw) {
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(raw, &parseError);

    if (parseError.error != QJsonParseError::NoError || !doc.isArray()) {
        emit failed(id(),
            QStringLiteral("Invalid JSON from codexbar: %1").arg(parseError.errorString()));
        return;
    }

    const QJsonArray arr = doc.array();
    if (arr.isEmpty()) {
        emit failed(id(), QStringLiteral("Empty response from codexbar"));
        return;
    }

    const QJsonObject entry = arr.first().toObject();

    const QJsonValue errVal = entry.value(QStringLiteral("error"));
    if (errVal.isObject()) {
        const QString msg = errVal.toObject()
            .value(QStringLiteral("message"))
            .toString(QStringLiteral("Unknown error from codexbar"));
        emit failed(id(), msg);
        return;
    }

    const QJsonObject usageObj = entry.value(QStringLiteral("usage")).toObject();
    if (usageObj.isEmpty()) {
        emit failed(id(), QStringLiteral("No usage data in codexbar response"));
        return;
    }

    ProviderQuota quota;
    quota.id          = m_providerId;
    quota.displayName = m_displayName;

    const QJsonObject identity = usageObj.value(QStringLiteral("identity")).toObject();
    if (!identity.isEmpty()) {
        const QString method = identity.value(QStringLiteral("loginMethod")).toString();
        if (!method.isEmpty())
            quota.planName = method;
    }

    auto makeWindow = [](const QJsonObject& tier, const QString& defaultLabel) {
        ProviderWindow w;
        w.usedFraction = qBound(0.0,
            tier.value(QStringLiteral("usedPercent")).toDouble(0.0) / 100.0, 1.0);

        const QString resetsAtStr = tier.value(QStringLiteral("resetsAt")).toString();
        if (!resetsAtStr.isEmpty())
            w.resetsAt = QDateTime::fromString(resetsAtStr, Qt::ISODate);

        const int wmin = static_cast<int>(
            tier.value(QStringLiteral("windowMinutes")).toDouble(0));
        w.label = windowLabel(wmin, defaultLabel);
        return w;
    };

    const QJsonValue primaryVal = usageObj.value(QStringLiteral("primary"));
    if (primaryVal.isObject())
        quota.windows.append(makeWindow(primaryVal.toObject(), QStringLiteral("primary")));

    const QJsonValue secondaryVal = usageObj.value(QStringLiteral("secondary"));
    if (secondaryVal.isObject())
        quota.windows.append(makeWindow(secondaryVal.toObject(), QStringLiteral("weekly")));

    const QJsonValue tertiaryVal = usageObj.value(QStringLiteral("tertiary"));
    if (tertiaryVal.isObject())
        quota.windows.append(makeWindow(tertiaryVal.toObject(), QStringLiteral("monthly")));

    if (quota.windows.isEmpty()) {
        emit failed(id(), QStringLiteral("No usage windows in codexbar response"));
        return;
    }

    emit done(quota);
}
