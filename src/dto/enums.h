#pragma once

#include <QObject>

namespace aegis::dto {
Q_NAMESPACE

enum class AgentStatus { Active, Idle, Error, Unknown };
Q_ENUM_NS(AgentStatus)

enum class CronState { Enabled, Disabled, Unknown };
Q_ENUM_NS(CronState)

enum class GitFileState {
  New,
  Modified,
  Deleted,
  Renamed,
  Untracked,
  Conflicted,
  Ignored,
};
Q_ENUM_NS(GitFileState)

enum class CreativeBackend { Ollama, GatewayCreative };
Q_ENUM_NS(CreativeBackend)

}  // namespace aegis::dto
