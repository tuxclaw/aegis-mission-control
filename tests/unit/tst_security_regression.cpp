#include <QMetaMethod>
#include <QMetaProperty>
#include <QHostAddress>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest>

#include <array>

#include "controllers/agent_controller.h"
#include "controllers/app_controller.h"
#include "controllers/calendar_controller.h"
#include "controllers/creative_controller.h"
#include "controllers/cron_controller.h"
#include "controllers/git_controller.h"
#include "controllers/memory_controller.h"
#include "controllers/model_controller.h"
#include "controllers/package_controller.h"
#include "controllers/settings_controller.h"
#include "controllers/vitals_controller.h"
#include "config/config_service.h"
#include "config/secret_store.h"
#include "core/http_client.h"
#include "dto/creative_dto.h"
#include "services/creative_service.h"
#include "services/gateway_service.h"

class SecurityRegressionTest : public QObject {
  Q_OBJECT

 private slots:
  void controllersDoNotExposeSecretNamedMembers();
  void creativeRequestRejectsOutOfRangeValues();
  void ollamaFramesStreamBeforeCompletion();
};

void SecurityRegressionTest::controllersDoNotExposeSecretNamedMembers() {
  const std::array<const QMetaObject*, 11> controllers = {
      &aegis::AppController::staticMetaObject,
      &aegis::AgentController::staticMetaObject,
      &aegis::VitalsController::staticMetaObject,
      &aegis::CalendarController::staticMetaObject,
      &aegis::CronController::staticMetaObject,
      &aegis::MemoryController::staticMetaObject,
      &aegis::ModelController::staticMetaObject,
      &aegis::PackageController::staticMetaObject,
      &aegis::GitController::staticMetaObject,
      &aegis::CreativeController::staticMetaObject,
      &aegis::SettingsController::staticMetaObject,
  };
  const QStringList denylist = {
      QStringLiteral("token"), QStringLiteral("secret"),
      QStringLiteral("password"), QStringLiteral("bearer"),
      QStringLiteral("credential")};

  for (const auto* metaObject : controllers) {
    for (int index = metaObject->propertyOffset();
         index < metaObject->propertyCount(); ++index) {
      const auto name = QString::fromLatin1(metaObject->property(index).name());
      for (const auto& denied : denylist) {
        QVERIFY2(!name.contains(denied, Qt::CaseInsensitive),
                 qPrintable(QStringLiteral("%1 property %2 matched %3")
                                .arg(metaObject->className(), name, denied)));
      }
    }
    for (int index = metaObject->methodOffset();
         index < metaObject->methodCount(); ++index) {
      const auto name = QString::fromLatin1(metaObject->method(index).name());
      for (const auto& denied : denylist) {
        QVERIFY2(!name.contains(denied, Qt::CaseInsensitive),
                 qPrintable(QStringLiteral("%1 method %2 matched %3")
                                .arg(metaObject->className(), name, denied)));
      }
    }
  }
}

void SecurityRegressionTest::creativeRequestRejectsOutOfRangeValues() {
  const QJsonObject valid{{QStringLiteral("backend"), QStringLiteral("ollama")},
                          {QStringLiteral("model"), QStringLiteral("test")},
                          {QStringLiteral("prompt"), QStringLiteral("hello")},
                          {QStringLiteral("temperature"), 0.7},
                          {QStringLiteral("maxTokens"), 1024}};
  QVERIFY(aegis::dto::CreativeRequest::fromJson(valid));

  for (const auto temperature : {-0.1, 2.1}) {
    auto invalid = valid;
    invalid.insert(QStringLiteral("temperature"), temperature);
    QVERIFY(!aegis::dto::CreativeRequest::fromJson(invalid));
  }
  for (const auto maxTokens : {0, 8193}) {
    auto invalid = valid;
    invalid.insert(QStringLiteral("maxTokens"), maxTokens);
    QVERIFY(!aegis::dto::CreativeRequest::fromJson(invalid));
  }
}

void SecurityRegressionTest::ollamaFramesStreamBeforeCompletion() {
  QTcpServer server;
  QVERIFY(server.listen(QHostAddress::LocalHost));
  const QByteArray first = "{\"response\":\"hello\",\"done\":false}\n";
  const QByteArray second =
      "{\"response\":\" world\",\"done\":true,\"done_reason\":\"stop\"}";
  connect(&server, &QTcpServer::newConnection, &server,
          [&server, first, second]() {
            auto* socket = server.nextPendingConnection();
            connect(socket, &QTcpSocket::readyRead, socket,
                    [socket, first, second]() {
                      socket->readAll();
                      if (socket->property("responded").toBool()) return;
                      socket->setProperty("responded", true);
                      const auto header =
                          QByteArrayLiteral("HTTP/1.1 200 OK\r\nContent-Type: application/x-ndjson\r\nContent-Length: ") +
                          QByteArray::number(first.size() + second.size()) +
                          QByteArrayLiteral("\r\nConnection: close\r\n\r\n");
                      socket->write(header + first);
                      socket->flush();
                      QTimer::singleShot(100, socket, [socket, second]() {
                        socket->write(second);
                        socket->disconnectFromHost();
                      });
                    });
          });

  QTemporaryDir temporary;
  QVERIFY(temporary.isValid());
  aegis::ConfigService config(temporary.filePath(QStringLiteral("aegis.ini")));
  const auto baseUrl = QStringLiteral("http://127.0.0.1:%1")
                           .arg(server.serverPort());
  QVERIFY(config.setValue(QStringLiteral("ollama.baseUrl"), baseUrl));
  aegis::SecretStore secrets;
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

  aegis::dto::CreativeRequest request;
  request.model = QStringLiteral("test");
  request.prompt = QStringLiteral("hello");
  const auto requestId = creative.generate(request);
  QVERIFY(!requestId.isEmpty());

  QTRY_COMPARE_WITH_TIMEOUT(chunks.size(), 1, 1000);
  QVERIFY(!finished);
  QCOMPARE(chunks.first(), QStringLiteral("hello"));
  QTRY_VERIFY_WITH_TIMEOUT(finished, 1000);
  QCOMPARE(chunks, QStringList({QStringLiteral("hello"),
                                QStringLiteral(" world")}));
  QCOMPARE(finalText, QStringLiteral("hello world"));
}

QTEST_MAIN(SecurityRegressionTest)
#include "tst_security_regression.moc"
