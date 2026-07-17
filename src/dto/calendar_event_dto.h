#pragma once

#include <QDateTime>
#include <QJsonObject>
#include <QString>

#include "core/result.h"

namespace aegis::dto {

struct CalendarEventDto {
  QString id;
  QString title;
  QString description;
  QDateTime start;
  QDateTime end;
  bool allDay = false;
  QString location;
  QString color;
  QDateTime createdAt;
  QDateTime updatedAt;

  // Parses and fully validates a persisted calendar event.
  static Result<CalendarEventDto> fromJson(const QJsonObject& object);

  // Serializes this calendar event into its stable JSON representation.
  [[nodiscard]] QJsonObject toJson() const;
};

}  // namespace aegis::dto
