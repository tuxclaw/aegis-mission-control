#pragma once

#include "services/i_provider_fetcher.h"

class MonitorConfig;

class MiniMaxFetcher : public IProviderFetcher {
    Q_OBJECT
public:
    explicit MiniMaxFetcher(const MonitorConfig* config, QNetworkAccessManager* nam,
                            QObject* parent = nullptr);

    QString id()          const override { return QStringLiteral("minimax"); }
    QString displayName() const override { return QStringLiteral("MiniMax"); }
    void    fetch()             override;

private:
    void fetchWithCookie(const QString& cookie);

    const MonitorConfig* m_config;
};
