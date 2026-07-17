#pragma once

#include <QDateTime>
#include <QJsonObject>
#include <QString>

#include "core/result.h"
#include "dto/enums.h"

namespace aegis::dto {

struct CronJobDto {
  QString id;
  QString name;
  QString schedule;
  CronState state = CronState::Unknown;
  QString command;
  QDateTime lastRun;
  QDateTime nextRun;
  QString lastResult;

  // Parses and fully validates a cron job object.
  static Result<CronJobDto> fromJson(const QJsonObject& object);

  // Serializes this cron job into its stable JSON representation.
  [[nodiscard]] QJsonObject toJson() const;
};

}  // namespace aegis::dto
