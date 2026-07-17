#pragma once

#include <memory>

#include <QMap>
#include <QObject>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVariant>

#include "core/result.h"

namespace aegis {

class ConfigService : public QObject {
 Q_OBJECT

 public:
  // Opens the standard Aegis INI file or an explicit test/configuration file.
  explicit ConfigService(const QString& settingsFile = {},
                         QObject* parent = nullptr);

  // Returns the validated gateway base URL, enforcing TLS off loopback.
  Result<QUrl> gatewayBaseUrl();
  // Returns the validated gateway connect timeout in milliseconds.
  Result<int> gatewayConnectTimeoutMs();
  // Returns the validated gateway total timeout in milliseconds.
  Result<int> gatewayTotalTimeoutMs();
  // Returns the validated gateway response byte cap.
  Result<quint64> gatewayMaxResponseBytes();
  // Returns the resolved executable path for the OpenClaw CLI.
  Result<QString> openclawBinary();
  // Returns the validated OpenClaw CLI timeout in milliseconds.
  Result<int> openclawCliTimeoutMs();
  // Returns the validated OpenClaw CLI output cap.
  Result<quint64> openclawCliOutputCap();
  // Returns the validated application data root.
  Result<QString> dataRoot();
  // Returns the validated allowlisted memory-root map.
  Result<QMap<QString, QString>> memoryRoots();
  // Returns the validated memory-file byte cap.
  Result<quint64> memoryMaxFileBytes();
  // Returns the configured repository path, or an empty string if unset.
  Result<QString> gitRepoPath();
  // Returns the validated git remote name.
  Result<QString> gitRemoteName();
  // Returns the validated git pull mode; v1 accepts only ff-only.
  Result<QString> gitPullMode();
  // Returns the validated Ollama base URL.
  Result<QUrl> ollamaBaseUrl();
  // Returns the validated vitals sampling interval in milliseconds.
  Result<int> vitalsIntervalMs();
  // Returns the package inventory program and argument array.
  Result<QStringList> packageQueryCommand();
  // Returns the validated UI theme name.
  Result<QString> uiTheme();
  // Returns whether non-essential motion is disabled.
  Result<bool> uiReduceMotion();
  // Returns the validated logging level.
  Result<QString> logLevel();

  // Validates and persists one known key; unknown keys are rejected.
  Result<void> setValue(const QString& key, const QVariant& value);

 private:
  Result<int> boundedInt(const QString& key, int defaultValue, int minimum,
                         int maximum);
  Result<quint64> boundedUnsigned(const QString& key, quint64 defaultValue,
                                  quint64 minimum, quint64 maximum);
  Result<QString> enumString(const QString& key, const QString& defaultValue,
                             const QStringList& allowed);
  Result<void> invalidAndReset(const QString& key, const QVariant& defaultValue,
                               const QString& detail);

  std::unique_ptr<QSettings> settings_;
};

}  // namespace aegis
