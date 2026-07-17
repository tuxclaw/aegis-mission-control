#include "config/config_service.h"

#include <QDir>
#include <QFileInfo>
#include <QHostAddress>
#include <QMetaType>
#include <QRegularExpression>
#include <QSet>
#include <QStandardPaths>
#include <QVariantMap>

#include "core/logging.h"

namespace aegis {
namespace {

constexpr auto kGatewayDefault = "http://localhost";
constexpr auto kOllamaDefault = "http://localhost:11434";

bool containsNul(const QString& value) { return value.contains(QChar::Null); }

bool isLoopbackHost(const QString& host) {
  if (host.compare(QStringLiteral("localhost"), Qt::CaseInsensitive) == 0) {
    return true;
  }
  QHostAddress address;
  return address.setAddress(host) && address.isLoopback();
}

bool validUrl(const QUrl& url, bool enforceGatewayTls) {
  if (!url.isValid() || url.host().isEmpty() || !url.userInfo().isEmpty() ||
      url.hasFragment() || containsNul(url.toString())) {
    return false;
  }
  const auto scheme = url.scheme().toLower();
  if (scheme != QStringLiteral("http") && scheme != QStringLiteral("https")) {
    return false;
  }
  return !enforceGatewayTls || scheme == QStringLiteral("https") ||
         isLoopbackHost(url.host());
}

QString defaultDataRoot() {
  return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
}

QVariantMap defaultMemoryRoots() {
  return {{QStringLiteral("workspace"),
           QDir::current().absoluteFilePath(QStringLiteral("memory"))}};
}

QStringList defaultPackageCommand() {
#if defined(Q_OS_LINUX)
  const auto rpm = QStandardPaths::findExecutable(QStringLiteral("rpm"));
  if (!rpm.isEmpty()) {
    return {rpm, QStringLiteral("-qa"), QStringLiteral("--qf"),
            QStringLiteral("%{NAME}\t%{VERSION}-%{RELEASE}.%{ARCH}\t%{SUMMARY}\n")};
  }
  const auto dpkg = QStandardPaths::findExecutable(QStringLiteral("dpkg-query"));
  if (!dpkg.isEmpty()) {
    return {dpkg, QStringLiteral("-W"),
            QStringLiteral("-f=${Package}\t${Version}\\n")};
  }
#endif
  return {};
}

}  // namespace

ConfigService::ConfigService(const QString& settingsFile, QObject* parent)
    : QObject(parent) {
  const auto defaultFile =
      QDir(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation))
          .filePath(QStringLiteral("Aegis/aegis.ini"));
  const auto file = settingsFile.isEmpty() ? defaultFile : settingsFile;
  QDir().mkpath(QFileInfo(file).absolutePath());
  settings_ = std::make_unique<QSettings>(file, QSettings::IniFormat);
}

Result<QUrl> ConfigService::gatewayBaseUrl() {
  const auto key = QStringLiteral("gateway.baseUrl");
  const auto fallback = QString::fromLatin1(kGatewayDefault);
  const auto raw = settings_->value(key, fallback).toString().trimmed();
  const QUrl url(raw);
  if (!validUrl(url, true)) {
    const auto reset = invalidAndReset(key, fallback,
                                       QStringLiteral("invalid gateway URL"));
    return tl::unexpected(reset.error());
  }
  return url;
}

Result<int> ConfigService::gatewayConnectTimeoutMs() {
  return boundedInt(QStringLiteral("gateway.connectTimeoutMs"), 5000, 1000,
                    60000);
}

Result<int> ConfigService::gatewayTotalTimeoutMs() {
  return boundedInt(QStringLiteral("gateway.totalTimeoutMs"), 30000, 1000,
                    300000);
}

Result<quint64> ConfigService::gatewayMaxResponseBytes() {
  return boundedUnsigned(QStringLiteral("gateway.maxResponseBytes"), 8388608,
                         65536, 67108864);
}

Result<QString> ConfigService::openclawBinary() {
  const auto key = QStringLiteral("openclaw.binary");
  const auto fallback = QStandardPaths::findExecutable(QStringLiteral("openclaw"));
  const auto configured = settings_->value(key, fallback).toString().trimmed();
  const auto resolved = QDir::isAbsolutePath(configured)
                            ? configured
                            : QStandardPaths::findExecutable(configured);
  const QFileInfo info(resolved);
  if (resolved.isEmpty() || containsNul(resolved) || !info.isFile() ||
      !info.isExecutable()) {
    const auto reset = invalidAndReset(
        key, fallback, QStringLiteral("OpenClaw binary is not executable"));
    return tl::unexpected(reset.error());
  }
  return info.canonicalFilePath();
}

Result<int> ConfigService::openclawCliTimeoutMs() {
  return boundedInt(QStringLiteral("openclaw.cliTimeoutMs"), 20000, 1000,
                    300000);
}

Result<quint64> ConfigService::openclawCliOutputCap() {
  return boundedUnsigned(QStringLiteral("openclaw.cliOutputCap"), 4194304,
                         65536, 67108864);
}

Result<QString> ConfigService::dataRoot() {
  const auto key = QStringLiteral("data.root");
  const auto fallback = defaultDataRoot();
  const auto path = QDir::cleanPath(settings_->value(key, fallback).toString());
  if (!QDir::isAbsolutePath(path) || containsNul(path)) {
    const auto reset = invalidAndReset(key, fallback,
                                       QStringLiteral("data root is invalid"));
    return tl::unexpected(reset.error());
  }
  return path;
}

Result<QMap<QString, QString>> ConfigService::memoryRoots() {
  const auto key = QStringLiteral("memory.roots");
  const auto fallback = defaultMemoryRoots();
  const auto map = settings_->value(key, fallback).toMap();
  static const QRegularExpression kRootId(
      QStringLiteral(R"(^[A-Za-z0-9][A-Za-z0-9_.-]{0,127}$)"));
  if (map.isEmpty() || map.size() > 64) {
    const auto reset = invalidAndReset(key, fallback,
                                       QStringLiteral("memory roots are empty"));
    return tl::unexpected(reset.error());
  }
  QMap<QString, QString> roots;
  for (auto iterator = map.constBegin(); iterator != map.constEnd(); ++iterator) {
    const auto path = QDir::cleanPath(iterator.value().toString());
    if (!kRootId.match(iterator.key()).hasMatch() ||
        !QDir::isAbsolutePath(path) || containsNul(path)) {
      const auto reset = invalidAndReset(
          key, fallback, QStringLiteral("memory root entry is invalid"));
      return tl::unexpected(reset.error());
    }
    roots.insert(iterator.key(), path);
  }
  return roots;
}

Result<quint64> ConfigService::memoryMaxFileBytes() {
  return boundedUnsigned(QStringLiteral("memory.maxFileBytes"), 5242880, 1024,
                         67108864);
}

Result<QString> ConfigService::gitRepoPath() {
  const auto key = QStringLiteral("git.repoPath");
  const auto path = settings_->value(key, QString()).toString().trimmed();
  if (path.isEmpty()) return QString();
  if (!QDir::isAbsolutePath(path) || containsNul(path)) {
    const auto reset = invalidAndReset(key, QString(),
                                       QStringLiteral("git repo path is invalid"));
    return tl::unexpected(reset.error());
  }
  return QDir::cleanPath(path);
}

Result<QString> ConfigService::gitRemoteName() {
  const auto key = QStringLiteral("git.remoteName");
  const auto fallback = QStringLiteral("origin");
  const auto value = settings_->value(key, fallback).toString().trimmed();
  static const QRegularExpression kRemote(
      QStringLiteral(R"(^[A-Za-z0-9][A-Za-z0-9._/-]{0,127}$)"));
  if (!kRemote.match(value).hasMatch()) {
    const auto reset = invalidAndReset(key, fallback,
                                       QStringLiteral("git remote is invalid"));
    return tl::unexpected(reset.error());
  }
  return value;
}

Result<QString> ConfigService::gitPullMode() {
  return enumString(QStringLiteral("git.pullMode"), QStringLiteral("ff-only"),
                    {QStringLiteral("ff-only")});
}

Result<QUrl> ConfigService::ollamaBaseUrl() {
  const auto key = QStringLiteral("ollama.baseUrl");
  const auto fallback = QString::fromLatin1(kOllamaDefault);
  const QUrl url(settings_->value(key, fallback).toString().trimmed());
  if (!validUrl(url, false)) {
    const auto reset = invalidAndReset(key, fallback,
                                       QStringLiteral("invalid Ollama URL"));
    return tl::unexpected(reset.error());
  }
  return url;
}

Result<int> ConfigService::vitalsIntervalMs() {
  return boundedInt(QStringLiteral("vitals.intervalMs"), 1000, 250, 10000);
}

Result<QStringList> ConfigService::packageQueryCommand() {
  const auto key = QStringLiteral("packages.queryCommand");
  const auto fallback = defaultPackageCommand();
  const auto command = settings_->value(key, fallback).toStringList();
  if (command.isEmpty() || command.size() > 64) {
    const auto reset = invalidAndReset(key, fallback,
                                       QStringLiteral("package command is empty"));
    return tl::unexpected(reset.error());
  }
  const auto program = QDir::isAbsolutePath(command.first())
                           ? command.first()
                           : QStandardPaths::findExecutable(command.first());
  const QFileInfo info(program);
  if (program.isEmpty() || !info.isFile() || !info.isExecutable()) {
    const auto reset = invalidAndReset(
        key, fallback, QStringLiteral("package program is not executable"));
    return tl::unexpected(reset.error());
  }
  auto resolved = command;
  resolved[0] = info.canonicalFilePath();
  for (const auto& argument : resolved) {
    if (containsNul(argument) || argument.size() > 4096) {
      const auto reset = invalidAndReset(
          key, fallback, QStringLiteral("package argument is invalid"));
      return tl::unexpected(reset.error());
    }
  }
  return resolved;
}

Result<QString> ConfigService::uiTheme() {
  return enumString(QStringLiteral("ui.theme"), QStringLiteral("dark"),
                    {QStringLiteral("dark"), QStringLiteral("light")});
}

Result<bool> ConfigService::uiReduceMotion() {
  const auto key = QStringLiteral("ui.reduceMotion");
  const auto value = settings_->value(key, false);
  if (value.metaType().id() != QMetaType::Bool) {
    const auto reset = invalidAndReset(key, false,
                                       QStringLiteral("reduce motion is not boolean"));
    return tl::unexpected(reset.error());
  }
  return value.toBool();
}

Result<QString> ConfigService::logLevel() {
  return enumString(
      QStringLiteral("log.level"), QStringLiteral("info"),
      {QStringLiteral("error"), QStringLiteral("warn"), QStringLiteral("info"),
       QStringLiteral("debug")});
}

Result<void> ConfigService::setValue(const QString& key,
                                     const QVariant& value) {
  static const QSet<QString> kKnownKeys = {
      QStringLiteral("gateway.baseUrl"),
      QStringLiteral("gateway.connectTimeoutMs"),
      QStringLiteral("gateway.totalTimeoutMs"),
      QStringLiteral("gateway.maxResponseBytes"),
      QStringLiteral("openclaw.binary"),
      QStringLiteral("openclaw.cliTimeoutMs"),
      QStringLiteral("openclaw.cliOutputCap"),
      QStringLiteral("data.root"),
      QStringLiteral("memory.roots"),
      QStringLiteral("memory.maxFileBytes"),
      QStringLiteral("git.repoPath"),
      QStringLiteral("git.remoteName"),
      QStringLiteral("git.pullMode"),
      QStringLiteral("ollama.baseUrl"),
      QStringLiteral("vitals.intervalMs"),
      QStringLiteral("packages.queryCommand"),
      QStringLiteral("ui.theme"),
      QStringLiteral("ui.reduceMotion"),
      QStringLiteral("log.level")};
  if (!kKnownKeys.contains(key)) {
    return tl::unexpected(makeError(ErrorCode::ConfigInvalid,
                                    QStringLiteral("unknown config key")));
  }
  settings_->setValue(key, value);
  settings_->sync();

  if (key == QStringLiteral("gateway.baseUrl")) return gatewayBaseUrl().transform([](const QUrl&) {});
  if (key == QStringLiteral("gateway.connectTimeoutMs")) return gatewayConnectTimeoutMs().transform([](int) {});
  if (key == QStringLiteral("gateway.totalTimeoutMs")) return gatewayTotalTimeoutMs().transform([](int) {});
  if (key == QStringLiteral("gateway.maxResponseBytes")) return gatewayMaxResponseBytes().transform([](quint64) {});
  if (key == QStringLiteral("openclaw.binary")) return openclawBinary().transform([](const QString&) {});
  if (key == QStringLiteral("openclaw.cliTimeoutMs")) return openclawCliTimeoutMs().transform([](int) {});
  if (key == QStringLiteral("openclaw.cliOutputCap")) return openclawCliOutputCap().transform([](quint64) {});
  if (key == QStringLiteral("data.root")) return dataRoot().transform([](const QString&) {});
  if (key == QStringLiteral("memory.roots")) return memoryRoots().transform([](const auto&) {});
  if (key == QStringLiteral("memory.maxFileBytes")) return memoryMaxFileBytes().transform([](quint64) {});
  if (key == QStringLiteral("git.repoPath")) return gitRepoPath().transform([](const QString&) {});
  if (key == QStringLiteral("git.remoteName")) return gitRemoteName().transform([](const QString&) {});
  if (key == QStringLiteral("git.pullMode")) return gitPullMode().transform([](const QString&) {});
  if (key == QStringLiteral("ollama.baseUrl")) return ollamaBaseUrl().transform([](const QUrl&) {});
  if (key == QStringLiteral("vitals.intervalMs")) return vitalsIntervalMs().transform([](int) {});
  if (key == QStringLiteral("packages.queryCommand")) return packageQueryCommand().transform([](const QStringList&) {});
  if (key == QStringLiteral("ui.theme")) return uiTheme().transform([](const QString&) {});
  if (key == QStringLiteral("ui.reduceMotion")) return uiReduceMotion().transform([](bool) {});
  return logLevel().transform([](const QString&) {});
}

Result<int> ConfigService::boundedInt(const QString& key, int defaultValue,
                                      int minimum, int maximum) {
  bool converted = false;
  const auto value = settings_->value(key, defaultValue).toLongLong(&converted);
  if (!converted || value < minimum || value > maximum) {
    const auto reset = invalidAndReset(key, defaultValue,
                                       QStringLiteral("integer setting out of range"));
    return tl::unexpected(reset.error());
  }
  return static_cast<int>(value);
}

Result<quint64> ConfigService::boundedUnsigned(const QString& key,
                                               quint64 defaultValue,
                                               quint64 minimum,
                                               quint64 maximum) {
  bool converted = false;
  const auto value = settings_->value(key, defaultValue).toULongLong(&converted);
  if (!converted || value < minimum || value > maximum) {
    const auto reset = invalidAndReset(
        key, defaultValue, QStringLiteral("unsigned setting out of range"));
    return tl::unexpected(reset.error());
  }
  return value;
}

Result<QString> ConfigService::enumString(const QString& key,
                                          const QString& defaultValue,
                                          const QStringList& allowed) {
  const auto value = settings_->value(key, defaultValue).toString().trimmed();
  if (!allowed.contains(value)) {
    const auto reset = invalidAndReset(key, defaultValue,
                                       QStringLiteral("enum setting is invalid"));
    return tl::unexpected(reset.error());
  }
  return value;
}

Result<void> ConfigService::invalidAndReset(const QString& key,
                                            const QVariant& defaultValue,
                                            const QString& detail) {
  settings_->setValue(key, defaultValue);
  settings_->sync();
  qCWarning(aegisConfigLog) << "Reset invalid setting" << key;
  return tl::unexpected(makeError(ErrorCode::ConfigInvalid, detail));
}

}  // namespace aegis
