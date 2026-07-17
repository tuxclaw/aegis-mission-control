#include <QHostAddress>
#include <QPointer>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest>

#include "config/config_service.h"
#include "config/secret_store.h"
#include "core/http_client.h"
#include "services/creative_service.h"
#include "services/gateway_service.h"

class CreativeStreamingTest : public QObject {
  Q_OBJECT

 private slots:
  void emitsChunkBeforeFinished();
  void cancelAbortsNetworkReply();

 private:
  static QUrl serverUrl(const QTcpServer& server);
};

QUrl CreativeStreamingTest::serverUrl(const QTcpServer& server) {
  QUrl url;
  url.setScheme(QStringLiteral("http"));
  url.setHost(QStringLiteral("127.0.0.1"));
  url.setPort(static_cast<int>(server.serverPort()));
  return url;
}

void CreativeStreamingTest::emitsChunkBeforeFinished() {
  QTcpServer server;
  QVERIFY(server.listen(QHostAddress::LocalHost));
  connect(&server, &QTcpServer::newConnection, &server, [&server] {
    auto* socket = server.nextPendingConnection();
    connect(socket, &QTcpSocket::readyRead, socket, [socket] {
      socket->readAll();
      if (socket->property("responded").toBool()) return;
      socket->setProperty("responded", true);
      socket->write("HTTP/1.1 200 OK\r\n"
                    "Content-Type: application/x-ndjson\r\n"
                    "Connection: close\r\n\r\n"
                    "{\"response\":\"first\",\"done\":false}\n");
      socket->flush();
      QTimer::singleShot(100, socket, [socket] {
        socket->write(
            "{\"response\":\" second\",\"done\":true,"
            "\"done_reason\":\"stop\"}\n");
        socket->disconnectFromHost();
      });
    });
  });

  QTemporaryDir directory;
  QVERIFY(directory.isValid());
  aegis::ConfigService config(
      directory.filePath(QStringLiteral("aegis.ini")));
  QVERIFY(config.setValue(QStringLiteral("ollama.baseUrl"),
                          serverUrl(server).toString()));
  aegis::SecretStore secrets(QStringLiteral("dev.tux.aegis.stream-test"));
  aegis::HttpClient http;
  aegis::GatewayService gateway(&secrets, &config, &http);
  aegis::CreativeService creative(&config, &http, &gateway);

  QStringList chunks;
  bool finished = false;
  QString finalText;
  connect(&creative, &aegis::CreativeService::chunk, &creative,
          [&chunks](const QString&, const QString& delta) {
            chunks.append(delta);
          });
  connect(&creative, &aegis::CreativeService::finished, &creative,
          [&finished, &finalText](const QString&,
                                  const aegis::dto::CreativeResult& result) {
            finished = true;
            finalText = result.text;
          });

  const auto requestId = creative.generate(
      {aegis::dto::CreativeBackend::Ollama, QStringLiteral("test-model"),
       QStringLiteral("test prompt"), 0.7, 128});
  QVERIFY(!requestId.isEmpty());
  QTRY_COMPARE_WITH_TIMEOUT(chunks.size(), 1, 1000);
  QVERIFY(!finished);
  QTRY_VERIFY_WITH_TIMEOUT(finished, 2000);
  QCOMPARE(chunks, QStringList({QStringLiteral("first"),
                                QStringLiteral(" second")}));
  QCOMPARE(finalText, QStringLiteral("first second"));
}

void CreativeStreamingTest::cancelAbortsNetworkReply() {
  QTcpServer server;
  QVERIFY(server.listen(QHostAddress::LocalHost));
  QPointer<QTcpSocket> activeSocket;
  bool disconnected = false;
  connect(&server, &QTcpServer::newConnection, &server,
          [&server, &activeSocket, &disconnected] {
            activeSocket = server.nextPendingConnection();
            connect(activeSocket, &QTcpSocket::readyRead, activeSocket,
                    [activeSocket] {
                      if (activeSocket) activeSocket->readAll();
                    });
            connect(activeSocket, &QTcpSocket::disconnected, &server,
                    [&disconnected] { disconnected = true; });
          });

  QTemporaryDir directory;
  QVERIFY(directory.isValid());
  aegis::ConfigService config(
      directory.filePath(QStringLiteral("aegis.ini")));
  QVERIFY(config.setValue(QStringLiteral("ollama.baseUrl"),
                          serverUrl(server).toString()));
  aegis::SecretStore secrets(QStringLiteral("dev.tux.aegis.cancel-test"));
  aegis::HttpClient http;
  aegis::GatewayService gateway(&secrets, &config, &http);
  aegis::CreativeService creative(&config, &http, &gateway);

  bool cancelled = false;
  connect(&creative, &aegis::CreativeService::finished, &creative,
          [&cancelled](const QString&,
                       const aegis::dto::CreativeResult& result) {
            cancelled = result.finishReason == QStringLiteral("cancelled");
          });
  const auto requestId = creative.generate(
      {aegis::dto::CreativeBackend::Ollama, QStringLiteral("test-model"),
       QStringLiteral("test prompt"), 0.7, 128});
  QTRY_VERIFY_WITH_TIMEOUT(!activeSocket.isNull(), 1000);
  creative.cancel(requestId);
  QTRY_VERIFY_WITH_TIMEOUT(cancelled, 1000);
  QTRY_VERIFY_WITH_TIMEOUT(disconnected, 1000);
}

QTEST_MAIN(CreativeStreamingTest)
#include "tst_creative_streaming.moc"
