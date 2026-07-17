#pragma once

#include <QDateTime>
#include <QJsonObject>
#include <QString>

#include "core/result.h"
#include "dto/enums.h"

namespace aegis::dto {

struct AgentDto {
  QString id;
  QString displayName;
  QString model;
  AgentStatus status = AgentStatus::Unknown;
  QString statusDetail;
  QDateTime lastSeen;
  int activeSessions = 0;
  QString avatarSeed;

  // Parses and fully validates an agent object.
  static Result<AgentDto> fromJson(const QJsonObject& object);

  // Serializes this agent into its stable JSON representation.
  [[nodiscard]] QJsonObject toJson() const;
};

}  // namespace aegis::dto
