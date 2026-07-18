#pragma once

#include "dto/provider_quota.h"

#include <QList>
#include <QObject>
#include <QPair>
#include <QUrl>

class QNetworkAccessManager;
class QNetworkReply;

class IProviderFetcher : public QObject {
    Q_OBJECT
public:
    explicit IProviderFetcher(QNetworkAccessManager* nam, QObject* parent = nullptr);
    ~IProviderFetcher() override;

    virtual QString id()          const = 0;
    virtual QString displayName() const = 0;
    virtual void    fetch()             = 0;

signals:
    void done(ProviderQuota quota);
    void failed(QString providerId, QString reason);

protected:
    QNetworkReply* httpGet(const QUrl& url,
                           const QList<QPair<QByteArray, QByteArray>>& headers = {});
    QNetworkReply* httpPost(const QUrl& url, const QByteArray& body,
                            const QList<QPair<QByteArray, QByteArray>>& headers = {});
    void           setTimeout(QNetworkReply* reply, int ms = 10000);

    QNetworkAccessManager* m_nam;
};
