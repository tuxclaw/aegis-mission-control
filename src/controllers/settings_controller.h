#pragma once

#include <QObject>
#include <QStringList>
#include <QVariantMap>

namespace aegis {
class ConfigService;
class SecretStore;

class SettingsController final : public QObject {
  Q_OBJECT
  Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
  Q_PROPERTY(QString gatewayBaseUrl READ gatewayBaseUrl WRITE setGatewayBaseUrl NOTIFY settingsChanged)
  Q_PROPERTY(bool gatewayAccessConfigured READ gatewayAccessConfigured NOTIFY
                 gatewayAccessConfiguredChanged)
  Q_PROPERTY(int gatewayConnectTimeoutMs READ gatewayConnectTimeoutMs WRITE setGatewayConnectTimeoutMs NOTIFY settingsChanged)
  Q_PROPERTY(int gatewayTotalTimeoutMs READ gatewayTotalTimeoutMs WRITE setGatewayTotalTimeoutMs NOTIFY settingsChanged)
  Q_PROPERTY(quint64 gatewayMaxResponseBytes READ gatewayMaxResponseBytes WRITE setGatewayMaxResponseBytes NOTIFY settingsChanged)
  Q_PROPERTY(QString openclawBinary READ openclawBinary WRITE setOpenclawBinary NOTIFY settingsChanged)
  Q_PROPERTY(int openclawCliTimeoutMs READ openclawCliTimeoutMs WRITE setOpenclawCliTimeoutMs NOTIFY settingsChanged)
  Q_PROPERTY(quint64 openclawCliOutputCap READ openclawCliOutputCap WRITE setOpenclawCliOutputCap NOTIFY settingsChanged)
  Q_PROPERTY(QString dataRoot READ dataRoot WRITE setDataRoot NOTIFY settingsChanged)
  Q_PROPERTY(QVariantMap memoryRoots READ memoryRoots WRITE setMemoryRoots NOTIFY settingsChanged)
  Q_PROPERTY(quint64 memoryMaxFileBytes READ memoryMaxFileBytes WRITE setMemoryMaxFileBytes NOTIFY settingsChanged)
  Q_PROPERTY(QString gitRepoPath READ gitRepoPath WRITE setGitRepoPath NOTIFY settingsChanged)
  Q_PROPERTY(QString gitRemoteName READ gitRemoteName WRITE setGitRemoteName NOTIFY settingsChanged)
  Q_PROPERTY(bool gitAccessConfigured READ gitAccessConfigured NOTIFY
                 gitAccessConfiguredChanged)
  Q_PROPERTY(QString gitPullMode READ gitPullMode CONSTANT)
  Q_PROPERTY(QString ollamaBaseUrl READ ollamaBaseUrl WRITE setOllamaBaseUrl NOTIFY settingsChanged)
  Q_PROPERTY(int vitalsIntervalMs READ vitalsIntervalMs WRITE setVitalsIntervalMs NOTIFY settingsChanged)
  Q_PROPERTY(QStringList packageQueryCommand READ packageQueryCommand WRITE setPackageQueryCommand NOTIFY settingsChanged)
  Q_PROPERTY(QString theme READ theme WRITE setTheme NOTIFY settingsChanged)
  Q_PROPERTY(bool reduceMotion READ reduceMotion WRITE setReduceMotion NOTIFY settingsChanged)
  Q_PROPERTY(QString logLevel READ logLevel WRITE setLogLevel NOTIFY settingsChanged)

 public:
  // Creates a non-secret settings editor over ConfigService and SecretStore.
  explicit SettingsController(ConfigService* config, SecretStore* secrets,
                              QObject* parent = nullptr);
  [[nodiscard]] bool busy() const;
  [[nodiscard]] QString gatewayBaseUrl() const;
  [[nodiscard]] bool gatewayAccessConfigured() const;
  [[nodiscard]] int gatewayConnectTimeoutMs() const;
  [[nodiscard]] int gatewayTotalTimeoutMs() const;
  [[nodiscard]] quint64 gatewayMaxResponseBytes() const;
  [[nodiscard]] QString openclawBinary() const;
  [[nodiscard]] int openclawCliTimeoutMs() const;
  [[nodiscard]] quint64 openclawCliOutputCap() const;
  [[nodiscard]] QString dataRoot() const;
  [[nodiscard]] QVariantMap memoryRoots() const;
  [[nodiscard]] quint64 memoryMaxFileBytes() const;
  [[nodiscard]] QString gitRepoPath() const;
  [[nodiscard]] QString gitRemoteName() const;
  [[nodiscard]] bool gitAccessConfigured() const;
  [[nodiscard]] QString gitPullMode() const;
  [[nodiscard]] QString ollamaBaseUrl() const;
  [[nodiscard]] int vitalsIntervalMs() const;
  [[nodiscard]] QStringList packageQueryCommand() const;
  [[nodiscard]] QString theme() const;
  [[nodiscard]] bool reduceMotion() const;
  [[nodiscard]] QString logLevel() const;

  void setGatewayBaseUrl(QString value);
  void setGatewayConnectTimeoutMs(int value);
  void setGatewayTotalTimeoutMs(int value);
  void setGatewayMaxResponseBytes(quint64 value);
  void setOpenclawBinary(QString value);
  void setOpenclawCliTimeoutMs(int value);
  void setOpenclawCliOutputCap(quint64 value);
  void setDataRoot(QString value);
  void setMemoryRoots(QVariantMap value);
  void setMemoryMaxFileBytes(quint64 value);
  void setGitRepoPath(QString value);
  void setGitRemoteName(QString value);
  void setOllamaBaseUrl(QString value);
  void setVitalsIntervalMs(int value);
  void setPackageQueryCommand(QStringList value);
  void setTheme(QString value);
  void setReduceMotion(bool value);
  void setLogLevel(QString value);

  // Validates and persists all non-secret fields.
  Q_INVOKABLE void save();
  // Restores the editable fields to documented defaults before save.
  Q_INVOKABLE void resetDefaults();
  // Updates gateway access material without ever reading it back to QML.
  Q_INVOKABLE void updateGatewayAccess(QString value);
  // Updates Git access material without ever reading it back to QML.
  Q_INVOKABLE void updateGitAccess(QString value);

 signals:
  void busyChanged();
  void settingsChanged();
  void gatewayAccessConfiguredChanged();
  void gitAccessConfiguredChanged();
  void settingsApplied();
  void settingsError(QString message);
  void errorRaised(QString message, bool retryable);
  void toast(QString message, int level);

 private:
  void reload();
  void refreshGatewayAccessConfigured();
  void refreshGitAccessConfigured();
  ConfigService* config_;
  SecretStore* secrets_;
  QString gatewayBaseUrl_, openclawBinary_, dataRoot_, gitRepoPath_;
  QString gitRemoteName_, ollamaBaseUrl_, theme_, logLevel_;
  int gatewayConnectTimeoutMs_ = 5000, gatewayTotalTimeoutMs_ = 30000;
  int openclawCliTimeoutMs_ = 20000, vitalsIntervalMs_ = 1000;
  quint64 gatewayMaxResponseBytes_ = 8388608;
  quint64 openclawCliOutputCap_ = 4194304, memoryMaxFileBytes_ = 5242880;
  QVariantMap memoryRoots_;
  QStringList packageQueryCommand_;
  bool busy_ = false;
  bool gatewayAccessConfigured_ = false;
  bool gitAccessConfigured_ = false;
  bool reduceMotion_ = false;
};
}  // namespace aegis
