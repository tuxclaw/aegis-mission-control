#include "app/qml_registration.h"

#include <qqml.h>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "app/app_context.h"
#include "dto/enums.h"

namespace aegis {

void QmlRegistration::registerTypes() {
  for (const auto* name : {"AgentStatus", "CronState", "GitFileState",
                           "CreativeBackend"}) {
    qmlRegisterUncreatableMetaObject(dto::staticMetaObject, "Aegis", 1, 0,
                                     name,
                                     QStringLiteral("Enums are read-only"));
  }
  qmlRegisterUncreatableType<ErrorCodeValues>(
      "Aegis", 1, 0, "ErrorCode", QStringLiteral("Enums are read-only"));
  qmlRegisterUncreatableType<ConnectionStateValues>(
      "Aegis", 1, 0, "ConnectionState",
      QStringLiteral("Enums are read-only"));
}

void QmlRegistration::registerContext(QQmlApplicationEngine* engine,
                                      AppContext* context) {
  auto* root = engine->rootContext();
  root->setContextProperty(QStringLiteral("app"), context->appController());
  root->setContextProperty(QStringLiteral("agents"), context->agentController());
  root->setContextProperty(QStringLiteral("vitals"), context->vitalsController());
  root->setContextProperty(QStringLiteral("calendar"), context->calendarController());
  root->setContextProperty(QStringLiteral("cron"), context->cronController());
  root->setContextProperty(QStringLiteral("memory"), context->memoryController());
  root->setContextProperty(QStringLiteral("models"), context->modelController());
  root->setContextProperty(QStringLiteral("packages"), context->packageController());
  root->setContextProperty(QStringLiteral("git"), context->gitController());
  root->setContextProperty(QStringLiteral("settings"), context->settingsController());
}

}  // namespace aegis
