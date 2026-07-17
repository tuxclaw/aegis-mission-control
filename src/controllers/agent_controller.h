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
  Q_PROPERTY(int totalCount READ totalCount NOTIFY totalCountChanged)

 public:
  // Creates a roster controller over the live CLI inventory.
  explicit AgentController(OpenClawCli* cli, QObject* parent = nullptr);
  [[nodiscard]] AgentListModel* agents();
  [[nodiscard]] bool loading() const;
  [[nodiscard]] int activeCount() const;
  [[nodiscard]] int totalCount() const;
  Q_INVOKABLE void refresh();
  Q_INVOKABLE void filterBy(QString status);

 signals:
  void loadingChanged();
  void activeCountChanged();
  void totalCountChanged();
  void errorRaised(QString message, bool retryable);
  void toast(QString message, int level);
  void refreshed();

 private:
  OpenClawCli* cli_;
  AgentListModel agents_;
  QVector<dto::AgentDto> all_;
  bool loading_ = false;
  int activeCount_ = 0;
  int totalCount_ = 0;
  QString filter_ = QStringLiteral("all");
};
}  // namespace aegis
