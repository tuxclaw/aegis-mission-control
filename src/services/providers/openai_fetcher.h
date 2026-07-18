#pragma once

#include "services/i_provider_fetcher.h"

class MonitorConfig;

class OpenAIFetcher : public IProviderFetcher {
    Q_OBJECT
public:
    explicit OpenAIFetcher(const MonitorConfig* config, QNetworkAccessManager* nam,
                           QObject* parent = nullptr);

    QString id()          const override { return QStringLiteral("openai"); }
    QString displayName() const override { return QStringLiteral("OpenAI (Codex)"); }
    void    fetch()             override;

private:
    void fetchCreditGrants();

    const MonitorConfig* m_config;
};
