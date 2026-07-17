#pragma once

#include <QDateTime>
#include <QObject>

namespace aegis {
class GatewayService;

class AppController final : public QObject {
  Q_OBJECT
  Q_PROPERTY(int connectionState READ connectionState NOTIFY connectionStateChanged)
  Q_PROPERTY(int activeView READ activeView WRITE setActiveView NOTIFY activeViewChanged)
  Q_PROPERTY(QDateTime lastSyncTime READ lastSyncTime NOTIFY lastSyncTimeChanged)

 public:
  // Creates the application-level state coordinator.
  explicit AppController(GatewayService* gateway, QObject* parent = nullptr);
  // Returns the gateway connection-state enum value.
  [[nodiscard]] int connectionState() const;
  // Returns the active view index.
  [[nodiscard]] int activeView() const;
  // Returns the latest successful synchronization time.
  [[nodiscard]] QDateTime lastSyncTime() const;
  // Selects a non-negative view index.
  Q_INVOKABLE void setActiveView(int index);
  // Requests all refreshable controllers to synchronize.
  Q_INVOKABLE void refreshAll();

 public slots:
  void markSynced();

 signals:
  void connectionStateChanged();
  void activeViewChanged();
  void lastSyncTimeChanged();
  void refreshAllRequested();

 private:
  GatewayService* gateway_;
  int activeView_ = 0;
  QDateTime lastSyncTime_;
};
}  // namespace aegis
