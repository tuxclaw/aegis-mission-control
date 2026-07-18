#pragma once

#include <QObject>
#include <QSettings>
#include <QString>

class MonitorConfig final : public QObject {
  Q_OBJECT

 public:
  explicit MonitorConfig(QObject* parent = nullptr);

  [[nodiscard]] QString providerCredential(const QString& id,
                                           const QString& key) const;
  [[nodiscard]] bool providerEnabled(const QString& id) const;
  [[nodiscard]] QString gatewayUrl() const;
  [[nodiscard]] QString gatewayToken() const;
  [[nodiscard]] QString openclawConfigPath() const;

 private:
  QSettings m_settings;
};
