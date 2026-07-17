#include <cstdlib>

#include <QFontDatabase>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QUrl>

#include "app/app_context.h"
#include "app/qml_registration.h"
#include "core/logging.h"

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
  aegis::QmlRegistration::registerContext(&engine, &appContext);
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
