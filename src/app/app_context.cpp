#include "app/app_context.h"

#include "core/async.h"

namespace aegis {

AppContext::AppContext()
    : configService_(std::make_unique<ConfigService>()),
      secretStore_(std::make_unique<SecretStore>()),
      httpClient_(std::make_unique<HttpClient>()) {
  async::configureGlobalThreadPool();
}

AppContext::~AppContext() = default;

ConfigService* AppContext::configService() const { return configService_.get(); }

SecretStore* AppContext::secretStore() const { return secretStore_.get(); }

HttpClient* AppContext::httpClient() const { return httpClient_.get(); }

}  // namespace aegis
