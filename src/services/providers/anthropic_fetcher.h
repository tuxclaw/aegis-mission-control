#pragma once

#include "services/i_provider_fetcher.h"

class MonitorConfig;

class AnthropicFetcher : public IProviderFetcher {
    Q_OBJECT
public:
    explicit AnthropicFetcher(const MonitorConfig* config, QNetworkAccessManager* nam,
                              QObject* parent = nullptr);

    QString id()          const override { return QStringLiteral("anthropic"); }
    QString displayName() const override { return QStringLiteral("Anthropic (Claude)"); }
    void    fetch()             override;

private:
    void fetchOAuthUsage(const QString& token, const QString& planName);

    const MonitorConfig* m_config;
};
