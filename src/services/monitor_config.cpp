#include "services/monitor_config.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>

namespace {

QString monitorConfigPath() {
  return QDir::homePath() +
         QStringLiteral("/.config/openclaw-monitor/config.ini");
}

QString credentialEnvironmentVariable(const QString& id,
                                      const QString& key) {
  if (id == QStringLiteral("zai") && key == QStringLiteral("apiKey")) {
    return QStringLiteral("Z_AI_API_KEY");
  }
  if (id == QStringLiteral("anthropic") &&
      key == QStringLiteral("adminKey")) {
    return QStringLiteral("ANTHROPIC_ADMIN_KEY");
  }
  if (id == QStringLiteral("gemini") && key == QStringLiteral("apiKey")) {
    return QStringLiteral("GEMINI_API_KEY");
  }
  return {};
}

}  // namespace

MonitorConfig::MonitorConfig(QObject* parent)
    : QObject(parent),
      m_settings(monitorConfigPath(), QSettings::IniFormat) {}

QString MonitorConfig::providerCredential(const QString& id,
                                          const QString& key) const {
  const QString environmentVariable =
      credentialEnvironmentVariable(id, key);
  if (!environmentVariable.isEmpty()) {
    const QString value = qEnvironmentVariable(environmentVariable.toUtf8());
    if (!value.isEmpty()) {
      return value;
    }
  }

  return m_settings
      .value(QStringLiteral("providers/%1/%2").arg(id, key))
      .toString();
}

bool MonitorConfig::providerEnabled(const QString& id) const {
  if (!QFileInfo::exists(m_settings.fileName())) {
    return false;
  }
  return m_settings
      .value(QStringLiteral("providers/%1/enabled").arg(id), false)
      .toBool();
}

QString MonitorConfig::gatewayUrl() const {
  const QString environmentValue =
      qEnvironmentVariable("OPENCLAW_MONITOR_GATEWAY");
  if (!environmentValue.isEmpty()) {
    return environmentValue;
  }
  return m_settings
      .value(QStringLiteral("gateway/url"),
             QStringLiteral("http://127.0.0.1:18789"))
      .toString();
}

QString MonitorConfig::gatewayToken() const {
  const QString environmentValue =
      qEnvironmentVariable("OPENCLAW_MONITOR_TOKEN");
  if (!environmentValue.isEmpty()) {
    return environmentValue;
  }

  const QString configured =
      m_settings.value(QStringLiteral("gateway/token")).toString();
  if (!configured.isEmpty()) {
    return configured;
  }

  QFile file(openclawConfigPath());
  if (!file.open(QIODevice::ReadOnly)) {
    return {};
  }
  const QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
  return root.value(QStringLiteral("gateway"))
      .toObject()
      .value(QStringLiteral("auth"))
      .toObject()
      .value(QStringLiteral("token"))
      .toString();
}

QString MonitorConfig::openclawConfigPath() const {
  return m_settings
      .value(QStringLiteral("scraper/openclawConfigPath"),
             QDir::homePath() + QStringLiteral("/.openclaw/openclaw.json"))
      .toString();
}
