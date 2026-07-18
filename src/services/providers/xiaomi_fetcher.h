#pragma once

#include "services/i_provider_fetcher.h"

#include <QByteArray>

class MonitorConfig;

class XiaomiFetcher : public IProviderFetcher {
    Q_OBJECT
public:
    explicit XiaomiFetcher(const MonitorConfig* config, QNetworkAccessManager* nam,
                           QObject* parent = nullptr);

    QString id()          const override { return QStringLiteral("xiaomi"); }
    QString displayName() const override { return QStringLiteral("Xiaomi MiMo"); }
    void    fetch()             override;

private:
    void resolveCookieAndFetch();
    QString readCookieFromFirefox() const;
    void onAllRepliesFinished();

    const MonitorConfig* m_config;
    int           m_pending = 0;
    QByteArray    m_balanceData;
    QByteArray    m_tokenDetailData;
    QByteArray    m_tokenUsageData;
    int           m_balanceStatus   = 0;
    int           m_tokenDetailStatus = 0;
    int           m_tokenUsageStatus  = 0;
};
