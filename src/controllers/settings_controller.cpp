#include "controllers/settings_controller.h"

#include <utility>

#include <QFutureWatcher>
#include <QDir>
#include <QList>
#include <QStandardPaths>
#include <QtConcurrent>

#include "config/config_service.h"
#include "config/secret_store.h"

namespace aegis {
namespace {

struct SettingsSnapshot {
  QString gatewayBaseUrl;
  int gatewayConnectTimeoutMs;
  int gatewayTotalTimeoutMs;
  quint64 gatewayMaxResponseBytes;
  QString openclawBinary;
  int openclawCliTimeoutMs;
  quint64 openclawCliOutputCap;
  QString dataRoot;
  QVariantMap memoryRoots;
  quint64 memoryMaxFileBytes;
  QString gitRepoPath;
  QString gitRemoteName;
  QString ollamaBaseUrl;
  int vitalsIntervalMs;
  QStringList packageQueryCommand;
  QString theme;
  bool reduceMotion;
  QString logLevel;

  [[nodiscard]] QList<QPair<QString, QVariant>> values() const {
    return {{QStringLiteral("gateway.baseUrl"), gatewayBaseUrl},
            {QStringLiteral("gateway.connectTimeoutMs"),
             gatewayConnectTimeoutMs},
            {QStringLiteral("gateway.totalTimeoutMs"), gatewayTotalTimeoutMs},
            {QStringLiteral("gateway.maxResponseBytes"),
             QVariant::fromValue(gatewayMaxResponseBytes)},
            {QStringLiteral("openclaw.binary"), openclawBinary},
            {QStringLiteral("openclaw.cliTimeoutMs"), openclawCliTimeoutMs},
            {QStringLiteral("openclaw.cliOutputCap"),
             QVariant::fromValue(openclawCliOutputCap)},
            {QStringLiteral("data.root"), dataRoot},
            {QStringLiteral("memory.roots"), memoryRoots},
            {QStringLiteral("memory.maxFileBytes"),
             QVariant::fromValue(memoryMaxFileBytes)},
            {QStringLiteral("git.repoPath"), gitRepoPath},
            {QStringLiteral("git.remoteName"), gitRemoteName},
            {QStringLiteral("git.pullMode"), QStringLiteral("ff-only")},
            {QStringLiteral("ollama.baseUrl"), ollamaBaseUrl},
            {QStringLiteral("vitals.intervalMs"), vitalsIntervalMs},
            {QStringLiteral("packages.queryCommand"), packageQueryCommand},
            {QStringLiteral("ui.theme"), theme},
            {QStringLiteral("ui.reduceMotion"), reduceMotion},
            {QStringLiteral("log.level"), logLevel}};
  }
};

}  // namespace

SettingsController::SettingsController(ConfigService* config,
                                       SecretStore* secrets, QObject* parent)
    : QObject(parent), config_(config), secrets_(secrets) {
  reload();
  refreshGatewayAccessConfigured();
  refreshGitAccessConfigured();
}

bool SettingsController::busy() const { return busy_; }
QString SettingsController::gatewayBaseUrl() const { return gatewayBaseUrl_; }
bool SettingsController::gatewayAccessConfigured() const {
  return gatewayAccessConfigured_;
}
int SettingsController::gatewayConnectTimeoutMs() const {
  return gatewayConnectTimeoutMs_;
}
int SettingsController::gatewayTotalTimeoutMs() const {
  return gatewayTotalTimeoutMs_;
}
quint64 SettingsController::gatewayMaxResponseBytes() const {
  return gatewayMaxResponseBytes_;
}
QString SettingsController::openclawBinary() const { return openclawBinary_; }
int SettingsController::openclawCliTimeoutMs() const {
  return openclawCliTimeoutMs_;
}
quint64 SettingsController::openclawCliOutputCap() const {
  return openclawCliOutputCap_;
}
QString SettingsController::dataRoot() const { return dataRoot_; }
QVariantMap SettingsController::memoryRoots() const { return memoryRoots_; }
quint64 SettingsController::memoryMaxFileBytes() const {
  return memoryMaxFileBytes_;
}
QString SettingsController::gitRepoPath() const { return gitRepoPath_; }
QString SettingsController::gitRemoteName() const { return gitRemoteName_; }
bool SettingsController::gitAccessConfigured() const {
  return gitAccessConfigured_;
}
QString SettingsController::gitPullMode() const {
  return QStringLiteral("ff-only");
}
QString SettingsController::ollamaBaseUrl() const { return ollamaBaseUrl_; }
int SettingsController::vitalsIntervalMs() const { return vitalsIntervalMs_; }
QStringList SettingsController::packageQueryCommand() const {
  return packageQueryCommand_;
}
QString SettingsController::theme() const { return theme_; }
bool SettingsController::reduceMotion() const { return reduceMotion_; }
QString SettingsController::logLevel() const { return logLevel_; }

void SettingsController::setGatewayBaseUrl(QString value) {
  gatewayBaseUrl_ = std::move(value);
  emit settingsChanged();
}

void SettingsController::setOpenclawBinary(QString value) {
  openclawBinary_ = std::move(value);
  emit settingsChanged();
}

void SettingsController::setDataRoot(QString value) {
  dataRoot_ = std::move(value);
  emit settingsChanged();
}

void SettingsController::setGitRepoPath(QString value) {
  gitRepoPath_ = std::move(value);
  emit settingsChanged();
}

void SettingsController::setGitRemoteName(QString value) {
  gitRemoteName_ = std::move(value);
  emit settingsChanged();
}

void SettingsController::setOllamaBaseUrl(QString value) {
  ollamaBaseUrl_ = std::move(value);
  emit settingsChanged();
}

void SettingsController::setTheme(QString value) {
  theme_ = std::move(value);
  emit settingsChanged();
}

void SettingsController::setLogLevel(QString value) {
  logLevel_ = std::move(value);
  emit settingsChanged();
}

void SettingsController::setMemoryRoots(QVariantMap value) {
  memoryRoots_ = std::move(value);
  emit settingsChanged();
}

void SettingsController::setPackageQueryCommand(QStringList value) {
  packageQueryCommand_ = std::move(value);
  emit settingsChanged();
}

void SettingsController::setGatewayConnectTimeoutMs(int value) {
  gatewayConnectTimeoutMs_ = value;
  emit settingsChanged();
}

void SettingsController::setGatewayTotalTimeoutMs(int value) {
  gatewayTotalTimeoutMs_ = value;
  emit settingsChanged();
}

void SettingsController::setGatewayMaxResponseBytes(quint64 value) {
  gatewayMaxResponseBytes_ = value;
  emit settingsChanged();
}

void SettingsController::setOpenclawCliTimeoutMs(int value) {
  openclawCliTimeoutMs_ = value;
  emit settingsChanged();
}

void SettingsController::setOpenclawCliOutputCap(quint64 value) {
  openclawCliOutputCap_ = value;
  emit settingsChanged();
}

void SettingsController::setMemoryMaxFileBytes(quint64 value) {
  memoryMaxFileBytes_ = value;
  emit settingsChanged();
}

void SettingsController::setVitalsIntervalMs(int value) {
  vitalsIntervalMs_ = value;
  emit settingsChanged();
}

void SettingsController::setReduceMotion(bool value) {
  reduceMotion_ = value;
  emit settingsChanged();
}

void SettingsController::reload() {
  if (const auto value = config_->gatewayBaseUrl()) {
    gatewayBaseUrl_ = value->toString();
  }
  if (const auto value = config_->gatewayConnectTimeoutMs()) {
    gatewayConnectTimeoutMs_ = value.value();
  }
  if (const auto value = config_->gatewayTotalTimeoutMs()) {
    gatewayTotalTimeoutMs_ = value.value();
  }
  if (const auto value = config_->gatewayMaxResponseBytes()) {
    gatewayMaxResponseBytes_ = value.value();
  }
  if (const auto value = config_->openclawBinary()) {
    openclawBinary_ = value.value();
  }
  if (const auto value = config_->openclawCliTimeoutMs()) {
    openclawCliTimeoutMs_ = value.value();
  }
  if (const auto value = config_->openclawCliOutputCap()) {
    openclawCliOutputCap_ = value.value();
  }
  if (const auto value = config_->dataRoot()) dataRoot_ = value.value();
  if (const auto value = config_->memoryRoots()) {
    memoryRoots_.clear();
    for (auto iterator = value->cbegin(); iterator != value->cend(); ++iterator) {
      memoryRoots_.insert(iterator.key(), iterator.value());
    }
  }
  if (const auto value = config_->memoryMaxFileBytes()) {
    memoryMaxFileBytes_ = value.value();
  }
  if (const auto value = config_->gitRepoPath()) gitRepoPath_ = value.value();
  if (const auto value = config_->gitRemoteName()) {
    gitRemoteName_ = value.value();
  }
  if (const auto value = config_->ollamaBaseUrl()) {
    ollamaBaseUrl_ = value->toString();
  }
  if (const auto value = config_->vitalsIntervalMs()) {
    vitalsIntervalMs_ = value.value();
  }
  if (const auto value = config_->packageQueryCommand()) {
    packageQueryCommand_ = value.value();
  }
  if (const auto value = config_->uiTheme()) theme_ = value.value();
  if (const auto value = config_->uiReduceMotion()) {
    reduceMotion_ = value.value();
  }
  if (const auto value = config_->logLevel()) logLevel_ = value.value();
  emit settingsChanged();
}

void SettingsController::refreshGatewayAccessConfigured() {
  secrets_->has(QStringLiteral("gateway.token"))
      .then(this, [this](const Result<bool>& result) {
        gatewayAccessConfigured_ = result ? result.value() : false;
        emit gatewayAccessConfiguredChanged();
      });
}

void SettingsController::refreshGitAccessConfigured() {
  secrets_->has(QStringLiteral("git.credential"))
      .then(this, [this](const Result<bool>& result) {
        gitAccessConfigured_ = result ? result.value() : false;
        emit gitAccessConfiguredChanged();
      });
}

void SettingsController::save() {
  if (busy_) return;
  const SettingsSnapshot snapshot{
      gatewayBaseUrl_,          gatewayConnectTimeoutMs_,
      gatewayTotalTimeoutMs_,   gatewayMaxResponseBytes_,
      openclawBinary_,          openclawCliTimeoutMs_,
      openclawCliOutputCap_,    dataRoot_,
      memoryRoots_,             memoryMaxFileBytes_,
      gitRepoPath_,             gitRemoteName_,
      ollamaBaseUrl_,           vitalsIntervalMs_,
      packageQueryCommand_,     theme_,
      reduceMotion_,            logLevel_};
  busy_ = true;
  emit busyChanged();

  auto* watcher = new QFutureWatcher<Result<void>>(this);
  connect(watcher, &QFutureWatcher<Result<void>>::finished, this,
          [this, watcher] {
            const auto result = watcher->result();
            watcher->deleteLater();
            busy_ = false;
            emit busyChanged();
            if (!result) {
              emit settingsError(result.error().userMessage);
              emit errorRaised(result.error().userMessage,
                               result.error().retryable);
              return;
            }
            emit settingsApplied();
            emit toast(QStringLiteral("Settings saved"), 1);
          });
  watcher->setFuture(QtConcurrent::run(
      [config = config_, values = snapshot.values()] {
        return config->persistValues(values);
      }));
}

void SettingsController::resetDefaults() {
  gatewayBaseUrl_ = QStringLiteral("http://localhost");
  gatewayConnectTimeoutMs_ = 5000;
  gatewayTotalTimeoutMs_ = 30000;
  gatewayMaxResponseBytes_ = 8388608;
  openclawBinary_ = QStringLiteral("openclaw");
  openclawCliTimeoutMs_ = 20000;
  openclawCliOutputCap_ = 4194304;
  dataRoot_ = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  memoryRoots_ = {{QStringLiteral("workspace"),
                   QDir::current().absoluteFilePath(QStringLiteral("memory"))}};
  memoryMaxFileBytes_ = 5242880;
  gitRepoPath_.clear();
  gitRemoteName_ = QStringLiteral("origin");
  ollamaBaseUrl_ = QStringLiteral("http://localhost:11434");
  vitalsIntervalMs_ = 1000;
  const auto packageCommand = config_->packageQueryCommand();
  packageQueryCommand_ = packageCommand ? packageCommand.value() : QStringList{};
  theme_ = QStringLiteral("dark");
  reduceMotion_ = false;
  logLevel_ = QStringLiteral("info");
  emit settingsChanged();
}

void SettingsController::updateGatewayAccess(QString value) {
  secrets_->write(QStringLiteral("gateway.token"), value)
      .then(this, [this](const Result<void>& result) {
        if (!result) {
          emit errorRaised(result.error().userMessage, result.error().retryable);
          return;
        }
        refreshGatewayAccessConfigured();
        emit toast(QStringLiteral("Gateway access updated"), 1);
      });
}

void SettingsController::updateGitAccess(QString value) {
  secrets_->write(QStringLiteral("git.credential"), value)
      .then(this, [this](const Result<void>& result) {
        if (!result) {
          emit errorRaised(result.error().userMessage, result.error().retryable);
          return;
        }
        refreshGitAccessConfigured();
        emit toast(QStringLiteral("Git access updated"), 1);
      });
}

}  // namespace aegis
