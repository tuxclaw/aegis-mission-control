#pragma once

#include <memory>

#include "config/config_service.h"
#include "config/secret_store.h"
#include "core/http_client.h"

#include "controllers/agent_controller.h"
#include "controllers/app_controller.h"
#include "controllers/calendar_controller.h"
#include "controllers/container_controller.h"
#include "controllers/cron_controller.h"
#include "controllers/git_controller.h"
#include "controllers/memory_controller.h"
#include "controllers/model_controller.h"
#include "controllers/package_controller.h"
#include "controllers/process_controller.h"
#include "controllers/settings_controller.h"
#include "controllers/vitals_controller.h"
#include "services/calendar_store.h"
#include "services/container_service.h"
#include "services/cron_service.h"
#include "services/gateway_service.h"
#include "services/git_service.h"
#include "services/memory_service.h"
#include "services/model_service.h"
#include "services/openclaw_cli.h"
#include "services/package_service.h"
#include "services/process_service.h"
#include "services/vitals_service.h"

namespace aegis {

class AppContext {
 public:
  // Constructs and wires the foundational application services.
  AppContext();
  // Destroys services in reverse dependency order.
  ~AppContext();

  AppContext(const AppContext&) = delete;
  AppContext& operator=(const AppContext&) = delete;

  // Returns a non-owning pointer to the application configuration service.
  [[nodiscard]] ConfigService* configService() const;
  // Returns a non-owning pointer to the fail-closed secret store.
  [[nodiscard]] SecretStore* secretStore() const;
  // Returns a non-owning pointer to the shared HTTP client.
  [[nodiscard]] HttpClient* httpClient() const;
  // Returns the application controller.
  [[nodiscard]] AppController* appController() const;
  // Returns the agent-roster controller.
  [[nodiscard]] AgentController* agentController() const;
  // Returns the system-vitals controller.
  [[nodiscard]] VitalsController* vitalsController() const;
  // Returns the container-inventory controller.
  [[nodiscard]] ContainerController* containerController() const;
  // Returns the top-process controller.
  [[nodiscard]] ProcessController* processController() const;
  // Returns the calendar controller.
  [[nodiscard]] CalendarController* calendarController() const;
  // Returns the cron controller.
  [[nodiscard]] CronController* cronController() const;
  // Returns the memory controller.
  [[nodiscard]] MemoryController* memoryController() const;
  // Returns the model controller.
  [[nodiscard]] ModelController* modelController() const;
  // Returns the package controller.
  [[nodiscard]] PackageController* packageController() const;
  // Returns the git controller.
  [[nodiscard]] GitController* gitController() const;
  // Returns the settings controller.
  [[nodiscard]] SettingsController* settingsController() const;

 private:
  std::unique_ptr<ConfigService> configService_;
  std::unique_ptr<SecretStore> secretStore_;
  std::unique_ptr<HttpClient> httpClient_;
  std::unique_ptr<GatewayService> gatewayService_;
  std::unique_ptr<OpenClawCli> openClawCli_;
  std::unique_ptr<VitalsService> vitalsService_;
  std::unique_ptr<ContainerService> containerService_;
  std::unique_ptr<ProcessService> processService_;
  std::unique_ptr<CalendarStore> calendarStore_;
  std::unique_ptr<CronService> cronService_;
  std::unique_ptr<MemoryService> memoryService_;
  std::unique_ptr<ModelService> modelService_;
  std::unique_ptr<PackageService> packageService_;
  std::unique_ptr<GitService> gitService_;
  std::unique_ptr<AppController> appController_;
  std::unique_ptr<AgentController> agentController_;
  std::unique_ptr<VitalsController> vitalsController_;
  std::unique_ptr<ContainerController> containerController_;
  std::unique_ptr<ProcessController> processController_;
  std::unique_ptr<CalendarController> calendarController_;
  std::unique_ptr<CronController> cronController_;
  std::unique_ptr<MemoryController> memoryController_;
  std::unique_ptr<ModelController> modelController_;
  std::unique_ptr<PackageController> packageController_;
  std::unique_ptr<GitController> gitController_;
  std::unique_ptr<SettingsController> settingsController_;
};

}  // namespace aegis
