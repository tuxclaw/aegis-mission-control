#include <cstdlib>

#include <QDir>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
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

  // Generate agents.json from OpenClaw CLI if no custom source is set.
  const QString agentsPath = qEnvironmentVariable(
      "AGENTS_SOURCE",
      QDir::homePath() + QStringLiteral("/.openclaw/agents.json"));
  if (!qEnvironmentVariableIsSet("AGENTS_SOURCE")) {
    QProcess cli;
    cli.start(QStringLiteral("openclaw"),
              {QStringLiteral("agents"), QStringLiteral("list"),
               QStringLiteral("--json")});
    if (cli.waitForFinished(5000) && cli.exitCode() == 0) {
      const QJsonDocument doc =
          QJsonDocument::fromJson(cli.readAllStandardOutput());
      if (doc.isArray()) {
        QJsonArray out;
        for (const auto& v : doc.array()) {
          if (!v.isObject()) continue;
          const auto obj = v.toObject();
          QJsonObject entry;
          entry[QStringLiteral("name")] =
              obj.value(QStringLiteral("id")).toString();
          entry[QStringLiteral("model")] =
              obj.value(QStringLiteral("model")).toString();
          entry[QStringLiteral("state")] =
              obj.value(QStringLiteral("isDefault")).toBool()
                  ? QStringLiteral("active")
                  : QStringLiteral("idle");
          entry[QStringLiteral("detail")] =
              obj.value(QStringLiteral("identityName")).toString();
          out.append(entry);
        }
        QFile f(agentsPath);
        if (f.open(QIODevice::WriteOnly))
          f.write(QJsonDocument(out).toJson());
      }
    }
  }

  SystemStats stats;
  ContainerList containers;
  AgentRoster agents;
  agents.setSource(agentsPath);

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
