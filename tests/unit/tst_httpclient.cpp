#include <QFutureWatcher>
#include <QHostAddress>
#include <QNetworkRequest>
#include <QTcpServer>
#include <QTcpSocket>
#include <QtTest>

#include "core/http_client.h"

class HttpClientTest : public QObject {
  Q_OBJECT

 private slots:
  void timeoutFires();
  void sizeCapAborts();
  void nonSuccessStatusMapsToHttpStatus();

 private:
  static QUrl serverUrl(const QTcpServer& server);
  static aegis::Result<QByteArray> waitFor(
      QFuture<aegis::Result<QByteArray>> future);
};

QUrl HttpClientTest::serverUrl(const QTcpServer& server) {
  QUrl url;
  url.setScheme(QStringLiteral("http"));
  url.setHost(QStringLiteral("127.0.0.1"));
  url.setPort(static_cast<int>(server.serverPort()));
  url.setPath(QStringLiteral("/test"));
  return url;
}

aegis::Result<QByteArray> HttpClientTest::waitFor(
    QFuture<aegis::Result<QByteArray>> future) {
  QFutureWatcher<aegis::Result<QByteArray>> watcher;
  watcher.setFuture(std::move(future));
  QSignalSpy spy(&watcher, &QFutureWatcherBase::finished);
  if (!watcher.isFinished()) spy.wait(2000);
  return watcher.result();
}

void HttpClientTest::timeoutFires() {
  QTcpServer server;
  QVERIFY(server.listen(QHostAddress::LocalHost));
  connect(&server, &QTcpServer::newConnection, &server, [&server]() {
    auto* socket = server.nextPendingConnection();
    QObject::connect(socket, &QTcpSocket::readyRead, socket,
                     [socket]() { socket->readAll(); });
  });
  aegis::HttpClient client;
  aegis::HttpRequestOptions options;
  options.connectTimeoutMs = 100;
  options.totalTimeoutMs = 150;
  options.maxRetries = 0;
  const auto result = waitFor(client.get(QNetworkRequest(serverUrl(server)), options));
  QVERIFY(!result.has_value());
  QCOMPARE(result.error().code, aegis::ErrorCode::NetworkTimeout);
}

void HttpClientTest::sizeCapAborts() {
  QTcpServer server;
  QVERIFY(server.listen(QHostAddress::LocalHost));
  connect(&server, &QTcpServer::newConnection, &server, [&server]() {
    auto* socket = server.nextPendingConnection();
    QObject::connect(socket, &QTcpSocket::readyRead, socket, [socket]() {
      socket->readAll();
      socket->write("HTTP/1.1 200 OK\r\nContent-Length: 100\r\nConnection: close\r\n\r\n");
      socket->write(QByteArray(100, 'x'));
      socket->disconnectFromHost();
    });
  });
  aegis::HttpClient client;
  aegis::HttpRequestOptions options;
  options.maxResponseBytes = 16;
  options.maxRetries = 0;
  const auto result = waitFor(client.get(QNetworkRequest(serverUrl(server)), options));
  QVERIFY(!result.has_value());
  QCOMPARE(result.error().code, aegis::ErrorCode::ResponseTooLarge);
}

void HttpClientTest::nonSuccessStatusMapsToHttpStatus() {
  QTcpServer server;
  QVERIFY(server.listen(QHostAddress::LocalHost));
  connect(&server, &QTcpServer::newConnection, &server, [&server]() {
    auto* socket = server.nextPendingConnection();
    QObject::connect(socket, &QTcpSocket::readyRead, socket, [socket]() {
      socket->readAll();
      socket->write("HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
      socket->disconnectFromHost();
    });
  });
  aegis::HttpClient client;
  aegis::HttpRequestOptions options;
  options.maxRetries = 0;
  const auto result = waitFor(client.get(QNetworkRequest(serverUrl(server)), options));
  QVERIFY(!result.has_value());
  QCOMPARE(result.error().code, aegis::ErrorCode::HttpStatus);
}

QTEST_MAIN(HttpClientTest)
#include "tst_httpclient.moc"
