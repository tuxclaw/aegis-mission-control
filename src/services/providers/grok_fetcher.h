#pragma once

#include "services/i_provider_fetcher.h"

#include <QString>

class MonitorConfig;
class QProcess;

class GrokFetcher : public IProviderFetcher {
    Q_OBJECT
public:
    explicit GrokFetcher(const MonitorConfig* config, QNetworkAccessManager* nam,
                         QObject* parent = nullptr);

    QString id()          const override { return QStringLiteral("xai"); }
    QString displayName() const override { return QStringLiteral("xAI (Grok)"); }
    void    fetch()             override;

private:
    void onProcessFinished(int exitCode, QProcess* process);
    void parseOutput(const QString& output);
    void parseJsonOutput(const QByteArray& raw);
    void parseTextOutput(const QString& text);

    const MonitorConfig* m_config;
    QString       m_cliPath;
};
