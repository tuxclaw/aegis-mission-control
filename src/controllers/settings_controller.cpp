#include "controllers/settings_controller.h"
#include "config/config_service.h"
#include "config/secret_store.h"
namespace aegis {
SettingsController::SettingsController(ConfigService* config,
                                       SecretStore* secrets, QObject* parent)
    : QObject(parent), config_(config), secrets_(secrets) {
  reload();
  refreshGatewayTokenSet();
  refreshGitCredentialSet();
}
#define GETTER(type,name) type SettingsController::name()const{return name##_ ;}
GETTER(QString,gatewayBaseUrl) GETTER(int,gatewayConnectTimeoutMs) GETTER(int,gatewayTotalTimeoutMs) GETTER(quint64,gatewayMaxResponseBytes) GETTER(QString,openclawBinary) GETTER(int,openclawCliTimeoutMs) GETTER(quint64,openclawCliOutputCap) GETTER(QString,dataRoot) GETTER(QVariantMap,memoryRoots) GETTER(quint64,memoryMaxFileBytes) GETTER(QString,gitRepoPath) GETTER(QString,gitRemoteName) GETTER(QString,ollamaBaseUrl) GETTER(int,vitalsIntervalMs) GETTER(QStringList,packageQueryCommand) GETTER(QString,theme) GETTER(bool,reduceMotion) GETTER(QString,logLevel)
#undef GETTER
bool SettingsController::gatewayTokenSet() const { return gatewayTokenSet_; }
bool SettingsController::gitCredentialSet() const { return gitCredentialSet_; }
QString SettingsController::gitPullMode()const{return QStringLiteral("ff-only");}
#define SETTER(type,method,field) void SettingsController::set##method(type v){field##_ =std::move(v);emit settingsChanged();}
SETTER(QString,GatewayBaseUrl,gatewayBaseUrl) SETTER(QString,OpenclawBinary,openclawBinary) SETTER(QString,DataRoot,dataRoot) SETTER(QVariantMap,MemoryRoots,memoryRoots) SETTER(QString,GitRepoPath,gitRepoPath) SETTER(QString,GitRemoteName,gitRemoteName) SETTER(QString,OllamaBaseUrl,ollamaBaseUrl) SETTER(QStringList,PackageQueryCommand,packageQueryCommand) SETTER(QString,Theme,theme) SETTER(QString,LogLevel,logLevel)
#undef SETTER
void SettingsController::setGatewayConnectTimeoutMs(int v){gatewayConnectTimeoutMs_=v;emit settingsChanged();}void SettingsController::setGatewayTotalTimeoutMs(int v){gatewayTotalTimeoutMs_=v;emit settingsChanged();}void SettingsController::setGatewayMaxResponseBytes(quint64 v){gatewayMaxResponseBytes_=v;emit settingsChanged();}void SettingsController::setOpenclawCliTimeoutMs(int v){openclawCliTimeoutMs_=v;emit settingsChanged();}void SettingsController::setOpenclawCliOutputCap(quint64 v){openclawCliOutputCap_=v;emit settingsChanged();}void SettingsController::setMemoryMaxFileBytes(quint64 v){memoryMaxFileBytes_=v;emit settingsChanged();}void SettingsController::setVitalsIntervalMs(int v){vitalsIntervalMs_=v;emit settingsChanged();}void SettingsController::setReduceMotion(bool v){reduceMotion_=v;emit settingsChanged();}
void SettingsController::reload(){if(auto v=config_->gatewayBaseUrl())gatewayBaseUrl_=v->toString();if(auto v=config_->gatewayConnectTimeoutMs())gatewayConnectTimeoutMs_=v.value();if(auto v=config_->gatewayTotalTimeoutMs())gatewayTotalTimeoutMs_=v.value();if(auto v=config_->gatewayMaxResponseBytes())gatewayMaxResponseBytes_=v.value();if(auto v=config_->openclawBinary())openclawBinary_=v.value();if(auto v=config_->openclawCliTimeoutMs())openclawCliTimeoutMs_=v.value();if(auto v=config_->openclawCliOutputCap())openclawCliOutputCap_=v.value();if(auto v=config_->dataRoot())dataRoot_=v.value();if(auto v=config_->memoryRoots()){memoryRoots_.clear();for(auto i=v->cbegin();i!=v->cend();++i)memoryRoots_.insert(i.key(),i.value());}if(auto v=config_->memoryMaxFileBytes())memoryMaxFileBytes_=v.value();if(auto v=config_->gitRepoPath())gitRepoPath_=v.value();if(auto v=config_->gitRemoteName())gitRemoteName_=v.value();if(auto v=config_->ollamaBaseUrl())ollamaBaseUrl_=v->toString();if(auto v=config_->vitalsIntervalMs())vitalsIntervalMs_=v.value();if(auto v=config_->packageQueryCommand())packageQueryCommand_=v.value();if(auto v=config_->uiTheme())theme_=v.value();if(auto v=config_->uiReduceMotion())reduceMotion_=v.value();if(auto v=config_->logLevel())logLevel_=v.value();emit settingsChanged();}
void SettingsController::refreshGatewayTokenSet() {
  secrets_->has(QStringLiteral("gateway.token"))
      .then(this, [this](const Result<bool>& result) {
        gatewayTokenSet_ = result ? result.value() : false;
        emit gatewayTokenSetChanged();
      });
}
void SettingsController::refreshGitCredentialSet() {
  secrets_->has(QStringLiteral("git.credential"))
      .then(this, [this](const Result<bool>& result) {
        gitCredentialSet_ = result ? result.value() : false;
        emit gitCredentialSetChanged();
      });
}
void SettingsController::save(){const QList<QPair<QString,QVariant>>v={{"gateway.baseUrl",gatewayBaseUrl_},{"gateway.connectTimeoutMs",gatewayConnectTimeoutMs_},{"gateway.totalTimeoutMs",gatewayTotalTimeoutMs_},{"gateway.maxResponseBytes",QVariant::fromValue(gatewayMaxResponseBytes_)},{"openclaw.binary",openclawBinary_},{"openclaw.cliTimeoutMs",openclawCliTimeoutMs_},{"openclaw.cliOutputCap",QVariant::fromValue(openclawCliOutputCap_)},{"data.root",dataRoot_},{"memory.roots",memoryRoots_},{"memory.maxFileBytes",QVariant::fromValue(memoryMaxFileBytes_)},{"git.repoPath",gitRepoPath_},{"git.remoteName",gitRemoteName_},{"git.pullMode","ff-only"},{"ollama.baseUrl",ollamaBaseUrl_},{"vitals.intervalMs",vitalsIntervalMs_},{"packages.queryCommand",packageQueryCommand_},{"ui.theme",theme_},{"ui.reduceMotion",reduceMotion_},{"log.level",logLevel_}};for(const auto&i:v){auto r=config_->setValue(i.first,i.second);if(!r){reload();emit errorRaised(r.error().userMessage,r.error().retryable);return;}}emit toast(QStringLiteral("Settings saved"),1);}
void SettingsController::resetDefaults(){gatewayBaseUrl_=QStringLiteral("http://localhost");gatewayConnectTimeoutMs_=5000;gatewayTotalTimeoutMs_=30000;gatewayMaxResponseBytes_=8388608;openclawBinary_.clear();openclawCliTimeoutMs_=20000;openclawCliOutputCap_=4194304;dataRoot_.clear();memoryRoots_.clear();memoryMaxFileBytes_=5242880;gitRepoPath_.clear();gitRemoteName_=QStringLiteral("origin");ollamaBaseUrl_=QStringLiteral("http://localhost:11434");vitalsIntervalMs_=1000;packageQueryCommand_.clear();theme_=QStringLiteral("dark");reduceMotion_=false;logLevel_=QStringLiteral("info");emit settingsChanged();}
void SettingsController::setGatewayToken(QString value) {
  secrets_->write(QStringLiteral("gateway.token"), value)
      .then(this, [this](const Result<void>& result) {
        if (!result) {
          emit errorRaised(result.error().userMessage, result.error().retryable);
          return;
        }
        refreshGatewayTokenSet();
        emit toast(QStringLiteral("Gateway credential updated"), 1);
      });
}
void SettingsController::setGitCredential(QString value) {
  secrets_->write(QStringLiteral("git.credential"), value)
      .then(this, [this](const Result<void>& result) {
        if (!result) {
          emit errorRaised(result.error().userMessage, result.error().retryable);
          return;
        }
        refreshGitCredentialSet();
        emit toast(QStringLiteral("Git credential updated"), 1);
      });
}
}
