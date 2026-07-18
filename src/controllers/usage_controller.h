#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>

class ProviderManager;

class UsageController final : public QObject {
  Q_OBJECT
  Q_PROPERTY(QVariantList providers READ providers NOTIFY providersChanged)
  Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
  Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorChanged)

 public:
  explicit UsageController(ProviderManager* manager,
                           QObject* parent = nullptr);

  Q_INVOKABLE void refresh();

  [[nodiscard]] QVariantList providers() const;
  [[nodiscard]] bool loading() const;
  [[nodiscard]] QString errorMessage() const;

 signals:
  void providersChanged();
  void loadingChanged();
  void errorChanged();

 private:
  ProviderManager* manager_;
  QVariantList providers_;
  QString errorMessage_;
  bool loading_ = false;
};
