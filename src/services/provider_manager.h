#pragma once

#include "dto/provider_quota.h"

#include <QList>
#include <QMap>
#include <QObject>
#include <QSet>

class MonitorConfig;
class IProviderFetcher;
class QNetworkAccessManager;

class ProviderManager : public QObject {
    Q_OBJECT
public:
    explicit ProviderManager(const MonitorConfig* config, QObject* parent = nullptr);
    ~ProviderManager() override;

    void refresh();
    QList<ProviderQuota> currentQuotas() const;

signals:
    void quotasReady(QList<ProviderQuota> quotas);

private slots:
    void onFetcherDone(ProviderQuota quota);
    void onFetcherFailed(QString providerId, QString reason);

private:
    const MonitorConfig*             m_config;
    QNetworkAccessManager*    m_nam;
    QList<IProviderFetcher*>  m_fetchers;
    QMap<QString, ProviderQuota> m_cache;
    QMap<QString, QDateTime>     m_fetchedAt;
    QSet<QString>               m_pending;
};
