#pragma once

#include "services/i_provider_fetcher.h"

#include <memory>
#include <QString>

class MonitorConfig;
class QProcess;
class QTemporaryDir;

class CodexBarFetcher : public IProviderFetcher {
    Q_OBJECT
public:
    explicit CodexBarFetcher(const MonitorConfig* config,
                             const QString& providerId,
                             const QString& displayName,
                             QNetworkAccessManager* nam,
                             QObject* parent = nullptr);

    QString id()          const override { return m_providerId; }
    QString displayName() const override { return m_displayName; }
    void    fetch()             override;

private:
    void onProcessFinished(int exitCode, QProcess* process);
    void parseJsonArray(const QByteArray& raw);
    QString codexbarProviderName() const;
    QString codexAuthHomePath() const;
    std::shared_ptr<QTemporaryDir> oauthOnlyCodexHome(const QString& authHome) const;
    static QString windowLabel(int windowMinutes, const QString& fallback);

    const MonitorConfig* m_config;
    QString       m_providerId;
    QString       m_displayName;
};
