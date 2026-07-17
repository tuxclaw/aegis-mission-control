#include <array>
#include <algorithm>

#include <QFutureWatcher>
#include <QMetaMethod>
#include <QMetaProperty>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QUuid>
#include <QtTest>

#include "config/config_service.h"
#include "config/secret_store.h"
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
#include "core/http_client.h"
#include "services/gateway_service.h"

class SecurityRegressionTest : public QObject {
  Q_OBJECT

 private slots:
  void controllerReflectionRejectsSensitiveNames();
  void gatewayFailsClosedWithoutAccessMaterial();
  void controllersExposeNoCredentialReturningMethod();

 private:
  static const std::array<const QMetaObject*, 11>& controllerMetaObjects();
  static bool containsSensitiveTerm(QByteArrayView name);
};

const std::array<const QMetaObject*, 11>&
SecurityRegressionTest::controllerMetaObjects() {
  static const std::array<const QMetaObject*, 11> kControllers = {
      &aegis::AgentController::staticMetaObject,
      &aegis::AppController::staticMetaObject,
      &aegis::CalendarController::staticMetaObject,
      &aegis::CreativeController::staticMetaObject,
      &aegis::CronController::staticMetaObject,
      &aegis::GitController::staticMetaObject,
      &aegis::MemoryController::staticMetaObject,
      &aegis::ModelController::staticMetaObject,
      &aegis::PackageController::staticMetaObject,
      &aegis::SettingsController::staticMetaObject,
      &aegis::VitalsController::staticMetaObject};
  return kControllers;
}

bool SecurityRegressionTest::containsSensitiveTerm(QByteArrayView name) {
  const auto normalized = name.toByteArray().toLower();
  static const std::array<QByteArray, 5> kDenied = {
      QByteArrayLiteral("token"), QByteArrayLiteral("secret"),
      QByteArrayLiteral("password"), QByteArrayLiteral("bearer"),
      QByteArrayLiteral("credential")};
  return std::any_of(kDenied.cbegin(), kDenied.cend(),
                     [&normalized](const QByteArray& denied) {
                       return normalized.contains(denied);
                     });
}

void SecurityRegressionTest::controllerReflectionRejectsSensitiveNames() {
  for (const auto* metaObject : controllerMetaObjects()) {
    for (int index = 0; index < metaObject->propertyCount(); ++index) {
      const auto property = metaObject->property(index);
      QVERIFY2(!containsSensitiveTerm(property.name()),
               qPrintable(QStringLiteral("Sensitive controller property: %1::%2")
                              .arg(metaObject->className(), property.name())));
    }
    for (int index = 0; index < metaObject->methodCount(); ++index) {
      const auto method = metaObject->method(index);
      QVERIFY2(!containsSensitiveTerm(method.name()),
               qPrintable(QStringLiteral("Sensitive controller method: %1::%2")
                              .arg(metaObject->className(), method.name())));
    }
  }
}

void SecurityRegressionTest::gatewayFailsClosedWithoutAccessMaterial() {
  QTemporaryDir directory;
  QVERIFY(directory.isValid());
  aegis::ConfigService config(
      directory.filePath(QStringLiteral("aegis.ini")));
  const auto serviceName =
      QStringLiteral("dev.tux.aegis.test.%1")
          .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
  aegis::SecretStore secrets(serviceName);
  aegis::HttpClient http;
  aegis::GatewayService gateway(&secrets, &config, &http);

  QFutureWatcher<aegis::Result<QJsonObject>> watcher;
  watcher.setFuture(gateway.get(QStringLiteral("status")));
  QSignalSpy finished(&watcher, &QFutureWatcherBase::finished);
  if (!watcher.isFinished()) QVERIFY(finished.wait(5000));
  const auto result = watcher.result();
  QVERIFY(!result);
  QCOMPARE(result.error().code, aegis::ErrorCode::MissingToken);
}

void SecurityRegressionTest::controllersExposeNoCredentialReturningMethod() {
  for (const auto* metaObject : controllerMetaObjects()) {
    for (int index = 0; index < metaObject->methodCount(); ++index) {
      const auto method = metaObject->method(index);
      if (method.returnMetaType().id() != QMetaType::QString &&
          method.returnMetaType().id() != QMetaType::QByteArray) {
        continue;
      }
      QVERIFY2(!containsSensitiveTerm(method.name()),
               qPrintable(QStringLiteral("Credential-returning surface: %1::%2")
                              .arg(metaObject->className(), method.name())));
    }
  }
}

QTEST_MAIN(SecurityRegressionTest)
#include "tst_security_regression.moc"
