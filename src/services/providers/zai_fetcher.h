#pragma once

#include "services/i_provider_fetcher.h"

class MonitorConfig;

class ZAIFetcher : public IProviderFetcher {
    Q_OBJECT
public:
    explicit ZAIFetcher(const MonitorConfig* config, QNetworkAccessManager* nam,
                        QObject* parent = nullptr);

    QString id()          const override { return QStringLiteral("zai"); }
    QString displayName() const override { return QStringLiteral("ZAI / GLM"); }
    void    fetch()             override;

private:
    void onReplyFinished(QNetworkReply* reply);

    const MonitorConfig* m_config;
};
