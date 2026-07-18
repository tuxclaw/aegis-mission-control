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
    void onReplyFinished(QNetworkReply* reply);

    const MonitorConfig* m_config;
};
