#include "controllers/agent_controller.h"

#include <algorithm>

#include "core/logging.h"
#include "services/openclaw_cli.h"

namespace aegis {

AgentController::AgentController(OpenClawCli* cli, QObject* parent)
    : QObject(parent), cli_(cli), agents_(this) {}

AgentListModel* AgentController::agents() { return &agents_; }

bool AgentController::loading() const { return loading_; }

int AgentController::activeCount() const { return activeCount_; }

int AgentController::totalCount() const { return totalCount_; }

void AgentController::refresh() {
  if (loading_) return;
  loading_ = true;
  emit loadingChanged();
  cli_->listAgents().then(
      this, [this](const Result<QVector<dto::AgentDto>>& result) {
        loading_ = false;
        emit loadingChanged();
        if (!result) {
          emit errorRaised(result.error().userMessage,
                           result.error().retryable);
          return;
        }

        all_ = result.value();
        const auto activeCount = static_cast<int>(std::count_if(
            all_.cbegin(), all_.cend(), [](const auto& agent) {
              return agent.status == dto::AgentStatus::Active;
            }));
        const auto totalCount = static_cast<int>(all_.size());
        qCDebug(aegisAppLog) << "Agent roster refreshed"
                             << "active" << activeCount << "total"
                             << totalCount;
        if (activeCount_ != activeCount) {
          activeCount_ = activeCount;
          emit activeCountChanged();
        }
        if (totalCount_ != totalCount) {
          totalCount_ = totalCount;
          emit totalCountChanged();
        }
        filterBy(filter_);
        emit refreshed();
      });
}

void AgentController::filterBy(QString status) {
  filter_ = status.toLower();
  if (filter_.isEmpty() || filter_ == QStringLiteral("all")) {
    agents_.setItems(all_);
    return;
  }

  QVector<dto::AgentDto> visible;
  for (const auto& agent : all_) {
    const auto active = filter_ == QStringLiteral("active") &&
                        agent.status == dto::AgentStatus::Active;
    const auto idle = filter_ == QStringLiteral("idle") &&
                      agent.status == dto::AgentStatus::Idle;
    const auto error = filter_ == QStringLiteral("error") &&
                       agent.status == dto::AgentStatus::Error;
    if (active || idle || error) visible.append(agent);
  }
  agents_.setItems(std::move(visible));
}

}  // namespace aegis
