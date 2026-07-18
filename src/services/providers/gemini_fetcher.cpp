#include "services/providers/gemini_fetcher.h"

#include "services/monitor_config.h"

GeminiFetcher::GeminiFetcher(const MonitorConfig* config,
                             QNetworkAccessManager* nam,
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

    auto* process = new QProcess(this);
    process->setProcessChannelMode(QProcess::ForwardedChannels);

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("GEMINI_API_KEY"), apiKey);
    process->setProcessEnvironment(env);

    connect(process, &QProcess::finished, this,
            [this, process](int exitCode, QProcess::ExitStatus exitStatus) {
        process->deleteLater();
        if (exitStatus == QProcess::NormalExit && exitCode == 0) {
            ProviderQuota quota;
            quota.id = id();
            quota.displayName = displayName();
            quota.planName = QStringLiteral("API Key");
            ProviderWindow window;
            window.label = QStringLiteral("active");
            window.usedFraction = -1.0;
            quota.windows.append(window);
            emit done(quota);
        } else {
            emit failed(id(), QStringLiteral(
                "Gemini CLI exited with code %1").arg(exitCode));
        }
    });

    connect(process, &QProcess::errorOccurred, this,
            [this, process](QProcess::ProcessError error) {
        if (error != QProcess::FailedToStart)
            return;
        const QString message = process->errorString();
        process->deleteLater();
        emit failed(id(), QStringLiteral(
            "Cannot start Gemini CLI: %1").arg(message));
    });

    process->start(QStringLiteral("gemini"), {
        QStringLiteral("--skip-trust"),
        QStringLiteral("-p"),
        QStringLiteral("respond with exactly: OK"),
    });
}
