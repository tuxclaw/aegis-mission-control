#pragma once

#include <memory>

#include "config/config_service.h"
#include "config/secret_store.h"
#include "core/http_client.h"

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

 private:
  std::unique_ptr<ConfigService> configService_;
  std::unique_ptr<SecretStore> secretStore_;
  std::unique_ptr<HttpClient> httpClient_;
};

}  // namespace aegis
