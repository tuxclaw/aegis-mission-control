#include "services/i_provider_fetcher.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>

IProviderFetcher::IProviderFetcher(QNetworkAccessManager* nam, QObject* parent)
    : QObject(parent), m_nam(nam) {}

IProviderFetcher::~IProviderFetcher() = default;

QNetworkReply* IProviderFetcher::httpGet(
    const QUrl& url,
    const QList<QPair<QByteArray, QByteArray>>& headers) {
    QNetworkRequest request(url);
    request.setTransferTimeout(10000);
    for (const auto& header : headers)
        request.setRawHeader(header.first, header.second);

    QNetworkReply* reply = m_nam->get(request);
    setTimeout(reply);
    return reply;
}

QNetworkReply* IProviderFetcher::httpPost(
    const QUrl& url,
    const QByteArray& body,
    const QList<QPair<QByteArray, QByteArray>>& headers) {
    QNetworkRequest request(url);
    request.setTransferTimeout(10000);
    for (const auto& header : headers)
        request.setRawHeader(header.first, header.second);

    QNetworkReply* reply = m_nam->post(request, body);
    setTimeout(reply);
    return reply;
}

void IProviderFetcher::setTimeout(QNetworkReply* reply, int ms) {
    if (!reply) return;

    QTimer::singleShot(ms, reply, [reply]() {
        if (reply->isRunning())
            reply->abort();
    });
}
