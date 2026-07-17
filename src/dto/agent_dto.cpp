#include "dto/agent_dto.h"

#include <QSet>

#include "dto/json_validation.h"

namespace aegis::dto {
namespace {

Result<AgentStatus> parseStatus(const QJsonValue& value) {
  if (!value.isString()) {
    return tl::unexpected(makeError(ErrorCode::ValidationFailed,
                                    QStringLiteral("status must be a string")));
  }
  const auto status = value.toString();
  if (status == QStringLiteral("active")) return AgentStatus::Active;
  if (status == QStringLiteral("idle")) return AgentStatus::Idle;
  if (status == QStringLiteral("error")) return AgentStatus::Error;
  if (status == QStringLiteral("unknown")) return AgentStatus::Unknown;
  return tl::unexpected(makeError(ErrorCode::ValidationFailed,
                                  QStringLiteral("unknown agent status")));
}

QString statusName(AgentStatus status) {
  switch (status) {
    case AgentStatus::Active: return QStringLiteral("active");
    case AgentStatus::Idle: return QStringLiteral("idle");
    case AgentStatus::Error: return QStringLiteral("error");
    case AgentStatus::Unknown: return QStringLiteral("unknown");
  }
  return QStringLiteral("unknown");
}

}  // namespace

Result<AgentDto> AgentDto::fromJson(const QJsonObject& object) {
  const auto unknown = json::rejectUnknown(
      object, {QStringLiteral("id"), QStringLiteral("displayName"),
               QStringLiteral("model"), QStringLiteral("status"),
               QStringLiteral("statusDetail"), QStringLiteral("lastSeen"),
               QStringLiteral("activeSessions"), QStringLiteral("avatarSeed")});
  if (!unknown) return tl::unexpected(unknown.error());

  const auto id = json::requiredString(object, QStringLiteral("id"), 1, 128, true);
  if (!id) return tl::unexpected(id.error());
  const auto displayName = json::optionalString(
      object, QStringLiteral("displayName"), 200, true);
  if (!displayName) return tl::unexpected(displayName.error());
  const auto model = json::optionalString(object, QStringLiteral("model"), 256);
  if (!model) return tl::unexpected(model.error());
  const auto detail = json::optionalString(
      object, QStringLiteral("statusDetail"), 500, true);
  if (!detail) return tl::unexpected(detail.error());
  const auto seen = json::optionalDateTime(object, QStringLiteral("lastSeen"));
  if (!seen) return tl::unexpected(seen.error());

  AgentStatus status = AgentStatus::Unknown;
  if (object.contains(QStringLiteral("status"))) {
    const auto parsed = parseStatus(object.value(QStringLiteral("status")));
    if (!parsed) return tl::unexpected(parsed.error());
    status = parsed.value();
  }

  int sessions = 0;
  if (object.contains(QStringLiteral("activeSessions"))) {
    const auto parsed = json::requiredInt(
        object, QStringLiteral("activeSessions"), 0, 100000);
    if (!parsed) return tl::unexpected(parsed.error());
    sessions = parsed.value();
  }
  const auto seed = json::optionalString(
      object, QStringLiteral("avatarSeed"), 128, true);
  if (!seed) return tl::unexpected(seed.error());

  return AgentDto{id.value(),
                  displayName->isEmpty() ? id.value() : displayName.value(),
                  model.value(), status, detail.value(), seen.value(), sessions,
                  seed->isEmpty() ? id.value() : seed.value()};
}

QJsonObject AgentDto::toJson() const {
  return {{QStringLiteral("id"), id},
          {QStringLiteral("displayName"), displayName},
          {QStringLiteral("model"), model},
          {QStringLiteral("status"), statusName(status)},
          {QStringLiteral("statusDetail"), statusDetail},
          {QStringLiteral("lastSeen"), json::dateTimeValue(lastSeen)},
          {QStringLiteral("activeSessions"), activeSessions},
          {QStringLiteral("avatarSeed"), avatarSeed}};
}

}  // namespace aegis::dto
