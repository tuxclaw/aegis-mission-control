#pragma once

#include <QObject>

#include "models/agent_list_model.h"

namespace aegis {
class OpenClawCli;

class AgentController final : public QObject {
  Q_OBJECT
  Q_PROPERTY(AgentListModel* agents READ agents CONSTANT)
  Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
  Q_PROPERTY(int activeCount READ activeCount NOTIFY activeCountChanged)

 public:
  // Creates a roster controller over the live CLI inventory.
  explicit AgentController(OpenClawCli* cli, QObject* parent = nullptr);
  [[nodiscard]] AgentListModel* agents();
  [[nodiscard]] bool loading() const;
  [[nodiscard]] int activeCount() const;
  Q_INVOKABLE void refresh();
  Q_INVOKABLE void filterBy(QString status);

 signals:
  void loadingChanged();
  void activeCountChanged();
  void errorRaised(QString message, bool retryable);
  void toast(QString message, int level);
  void refreshed();

 private:
  OpenClawCli* cli_;
  AgentListModel agents_;
  QVector<dto::AgentDto> all_;
  bool loading_ = false;
  QString filter_ = QStringLiteral("all");
};
}  // namespace aegis
