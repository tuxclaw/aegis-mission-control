#include "app/app_context.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QTimer>

#include "core/async.h"

#include <git2.h>

namespace aegis {

AppContext::AppContext()
    : configService_(std::make_unique<ConfigService>()),
      secretStore_(std::make_unique<SecretStore>()),
      httpClient_(std::make_unique<HttpClient>()),
      gatewayService_(std::make_unique<GatewayService>(
          secretStore_.get(), configService_.get(), httpClient_.get())),
      openClawCli_(std::make_unique<OpenClawCli>(configService_.get())),
      vitalsService_(std::make_unique<VitalsService>(configService_.get())),
      containerService_(std::make_unique<ContainerService>()),
      processService_(std::make_unique<ProcessService>()),
      calendarStore_(std::make_unique<CalendarStore>(configService_.get())),
      cronService_(std::make_unique<CronService>(openClawCli_.get())),
      memoryService_(std::make_unique<MemoryService>(configService_.get())),
      modelService_(std::make_unique<ModelService>(openClawCli_.get())),
      packageService_(std::make_unique<PackageService>(configService_.get())),
      gitService_(std::make_unique<GitService>(configService_.get(),
                                               secretStore_.get())),
      appController_(std::make_unique<AppController>(gatewayService_.get())),
      agentController_(std::make_unique<AgentController>(openClawCli_.get())),
      vitalsController_(std::make_unique<VitalsController>(vitalsService_.get())),
      containerController_(
          std::make_unique<ContainerController>(containerService_.get())),
      processController_(
          std::make_unique<ProcessController>(processService_.get())),
      calendarController_(std::make_unique<CalendarController>(calendarStore_.get())),
      cronController_(std::make_unique<CronController>(cronService_.get())),
      memoryController_(std::make_unique<MemoryController>(memoryService_.get())),
      modelController_(std::make_unique<ModelController>(modelService_.get())),
      packageController_(std::make_unique<PackageController>(packageService_.get())),
      gitController_(std::make_unique<GitController>(gitService_.get(),
                                                     configService_.get())),
      settingsController_(std::make_unique<SettingsController>(
          configService_.get(), secretStore_.get())) {
  async::configureGlobalThreadPool();

  const auto checkGateway = [this] {
    auto request = gatewayService_->get(QStringLiteral("/status"));
    Q_UNUSED(request);
  };

  // Auto-detect gateway token from OpenClaw config if not in SecretStore.
  const QString configPath = QDir::homePath() + QStringLiteral("/.openclaw/openclaw.json");
  QFile configFile(configPath);
  bool healthCheckPending = false;
  if (configFile.open(QIODevice::ReadOnly)) {
    const auto doc = QJsonDocument::fromJson(configFile.readAll());
    const auto root = doc.object();
    const auto gatewayAuth = root.value(QStringLiteral("gateway"))
                                 .toObject()
                                 .value(QStringLiteral("auth"))
                                 .toObject();
    auto token = gatewayAuth.value(QStringLiteral("token")).toString();
    if (token.isEmpty()) {
      token = root.value(QStringLiteral("auth"))
                  .toObject()
                  .value(QStringLiteral("token"))
                  .toString();
    }
    if (!token.isEmpty()) {
      healthCheckPending = true;
      auto tokenWrite =
          secretStore_->write(QStringLiteral("gateway.token"), token);
      (void)tokenWrite.then(gatewayService_.get(),
                            [checkGateway](const Result<void>&) {
                              checkGateway();
                            });
    }
  }
  if (!healthCheckPending) {
    QTimer::singleShot(0, gatewayService_.get(), checkGateway);
  }
  auto* gatewayHealthTimer = new QTimer(gatewayService_.get());
  QObject::connect(gatewayHealthTimer, &QTimer::timeout,
                   gatewayService_.get(), checkGateway);
  gatewayHealthTimer->start(30000);

  QObject::connect(appController_.get(), &AppController::refreshAllRequested,
                   agentController_.get(), &AgentController::refresh);
  QObject::connect(appController_.get(), &AppController::refreshAllRequested,
                   containerController_.get(), &ContainerController::refresh);
  QObject::connect(appController_.get(), &AppController::refreshAllRequested,
                   processController_.get(), &ProcessController::refresh);
  QObject::connect(appController_.get(), &AppController::refreshAllRequested,
                   calendarController_.get(), &CalendarController::refresh);
  QObject::connect(appController_.get(), &AppController::refreshAllRequested,
                   cronController_.get(), &CronController::refresh);
  QObject::connect(appController_.get(), &AppController::refreshAllRequested,
                   memoryController_.get(), &MemoryController::refresh);
  QObject::connect(appController_.get(), &AppController::refreshAllRequested,
                   modelController_.get(), &ModelController::refresh);
  QObject::connect(appController_.get(), &AppController::refreshAllRequested,
                   packageController_.get(), &PackageController::refresh);
  QObject::connect(appController_.get(), &AppController::refreshAllRequested,
                   gitController_.get(), &GitController::refresh);
  QObject::connect(agentController_.get(), &AgentController::refreshed,
                   appController_.get(), &AppController::markSynced);
  QObject::connect(settingsController_.get(),
                   &SettingsController::settingsApplied,
                   memoryController_.get(),
                   &MemoryController::reconfigureRoots);
  QObject::connect(settingsController_.get(),
                   &SettingsController::settingsApplied, [this] {
                     const auto interval = configService_->vitalsIntervalMs();
                     if (interval) {
                       vitalsService_->start(
                           std::chrono::milliseconds(interval.value()));
                     }
                   });
  auto interval = configService_->vitalsIntervalMs();
  vitalsService_->start(std::chrono::milliseconds(interval ? interval.value() : 1000));
  containerService_->start();
  processService_->start();
}

AppContext::~AppContext() {
  GitService::shutdownLibrary();
}

ConfigService* AppContext::configService() const { return configService_.get(); }

SecretStore* AppContext::secretStore() const { return secretStore_.get(); }

HttpClient* AppContext::httpClient() const { return httpClient_.get(); }
AppController* AppContext::appController() const { return appController_.get(); }
AgentController* AppContext::agentController() const { return agentController_.get(); }
VitalsController* AppContext::vitalsController() const { return vitalsController_.get(); }
ContainerController* AppContext::containerController() const {
  return containerController_.get();
}
ProcessController* AppContext::processController() const {
  return processController_.get();
}
CalendarController* AppContext::calendarController() const { return calendarController_.get(); }
CronController* AppContext::cronController() const { return cronController_.get(); }
MemoryController* AppContext::memoryController() const { return memoryController_.get(); }
ModelController* AppContext::modelController() const { return modelController_.get(); }
PackageController* AppContext::packageController() const { return packageController_.get(); }
GitController* AppContext::gitController() const { return gitController_.get(); }
SettingsController* AppContext::settingsController() const { return settingsController_.get(); }

}  // namespace aegis
