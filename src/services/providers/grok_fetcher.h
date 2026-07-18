#pragma once

#include "services/i_provider_fetcher.h"

class MonitorConfig;

class GrokFetcher : public IProviderFetcher {
    Q_OBJECT
public:
    explicit GrokFetcher(const MonitorConfig* config, QNetworkAccessManager* nam,
                         QObject* parent = nullptr);

    QString id()          const override { return QStringLiteral("xai"); }
    QString displayName() const override { return QStringLiteral("xAI (Grok)"); }
    void    fetch()             override;

};
