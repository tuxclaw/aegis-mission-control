#include <cstdlib>

#include <QDir>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QUrl>

#include "app/app_context.h"
#include "app/qml_registration.h"
#include "core/logging.h"
#include "SystemStats.h"
#include "services/agent_roster.h"
#include "services/container_list.h"

int main(int argc, char* argv[]) {
  QCoreApplication::setOrganizationName(QStringLiteral("Aegis"));
  QCoreApplication::setOrganizationDomain(QStringLiteral("dev.tux"));
  QCoreApplication::setApplicationName(QStringLiteral("AEGIS Mission Control"));
  QCoreApplication::setApplicationVersion(QStringLiteral("1.0.0"));

  QGuiApplication application(argc, argv);
  aegis::logging::installRedactingMessageHandler();
  QFontDatabase::addApplicationFont(
      QStringLiteral(":/fonts/Inter-Variable.ttf"));
  QFontDatabase::addApplicationFont(
      QStringLiteral(":/fonts/JetBrainsMono-Variable.ttf"));

  aegis::AppContext appContext;
  aegis::QmlRegistration::registerTypes();

  QQmlApplicationEngine engine;
  engine.addImportPath(QStringLiteral("qrc:/qt/qml"));
  aegis::QmlRegistration::registerContext(&engine, &appContext);

  SystemStats stats;
  ContainerList containers;
  AgentRoster agents;
  agents.setSource(qEnvironmentVariable(
      "AGENTS_SOURCE",
      QDir::homePath() + QStringLiteral("/.openclaw/agents.json")));

  engine.rootContext()->setContextProperty(QStringLiteral("Stats"), &stats);
  engine.rootContext()->setContextProperty(QStringLiteral("Containers"),
                                           &containers);
  engine.rootContext()->setContextProperty(QStringLiteral("Agents"), &agents);

  const QUrl mainUrl(QStringLiteral("qrc:/qml/Main.qml"));
  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreated, &application,
      [mainUrl](QObject* object, const QUrl& objectUrl) {
        if (object == nullptr && objectUrl == mainUrl) {
          QCoreApplication::exit(EXIT_FAILURE);
        }
      },
      Qt::QueuedConnection);
  engine.load(mainUrl);
  return application.exec();
}
