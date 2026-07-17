#include "dto/agent_dto.h"

#include <QSet>

#include "dto/json_validation.h"

namespace aegis::dto {
namespace {

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
      object,
      {QStringLiteral("agentDir"), QStringLiteral("bindings"),
       QStringLiteral("id"), QStringLiteral("identityEmoji"),
       QStringLiteral("identityName"), QStringLiteral("identitySource"),
       QStringLiteral("isDefault"), QStringLiteral("model"),
       QStringLiteral("name"), QStringLiteral("workspace")});
  if (!unknown) return tl::unexpected(unknown.error());

  const auto id = json::requiredString(object, QStringLiteral("id"), 1, 128, true);
  if (!id) return tl::unexpected(id.error());
  const auto name =
      json::optionalString(object, QStringLiteral("name"), 200, true);
  if (!name) return tl::unexpected(name.error());
  const auto identityName = json::optionalString(
      object, QStringLiteral("identityName"), 200, true);
  if (!identityName) return tl::unexpected(identityName.error());
  const auto identityEmoji = json::optionalString(
      object, QStringLiteral("identityEmoji"), 64, true);
  if (!identityEmoji) return tl::unexpected(identityEmoji.error());
  const auto model = json::optionalString(object, QStringLiteral("model"), 256);
  if (!model) return tl::unexpected(model.error());
  for (const auto& key : {QStringLiteral("agentDir"),
                          QStringLiteral("identitySource"),
                          QStringLiteral("workspace")}) {
    const auto value = json::optionalString(object, key, 4096, true);
    if (!value) return tl::unexpected(value.error());
  }
  if (object.contains(QStringLiteral("bindings"))) {
    const auto bindings = json::requiredInt(
        object, QStringLiteral("bindings"), 0, 100000);
    if (!bindings) return tl::unexpected(bindings.error());
  }
  if (object.contains(QStringLiteral("isDefault"))) {
    const auto isDefault =
        json::requiredBool(object, QStringLiteral("isDefault"));
    if (!isDefault) return tl::unexpected(isDefault.error());
  }

  const auto displayName = !name->isEmpty()
                               ? name.value()
                               : (!identityName->isEmpty() ? identityName.value()
                                                           : id.value());
  const auto avatarSeed = identityEmoji->isEmpty() ? id.value()
                                                    : identityEmoji.value();

  return AgentDto{id.value(), displayName, model.value(), AgentStatus::Unknown,
                  {}, QDateTime(), 0, avatarSeed};
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
