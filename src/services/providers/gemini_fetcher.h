#pragma once

#include "services/i_provider_fetcher.h"

class MonitorConfig;

class GeminiFetcher : public IProviderFetcher {
    Q_OBJECT
public:
    explicit GeminiFetcher(const MonitorConfig* config, QNetworkAccessManager* nam,
                           QObject* parent = nullptr);

    QString id()          const override { return QStringLiteral("gemini"); }
    QString displayName() const override { return QStringLiteral("Gemini"); }
    void    fetch()             override;

private:
    void fetchCodeAssist(const QString& accessToken);
    void fetchQuota(const QString& accessToken, const QString& projectId,
                    const QString& planName);

    const MonitorConfig* m_config;
};
