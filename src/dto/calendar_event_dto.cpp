#include "dto/calendar_event_dto.h"

#include <QUuid>

#include "dto/json_validation.h"

namespace aegis::dto {

Result<CalendarEventDto> CalendarEventDto::fromJson(const QJsonObject& object) {
  const auto unknown = json::rejectUnknown(
      object, {QStringLiteral("id"), QStringLiteral("title"),
               QStringLiteral("description"), QStringLiteral("start"),
               QStringLiteral("end"), QStringLiteral("allDay"),
               QStringLiteral("location"), QStringLiteral("color"),
               QStringLiteral("createdAt"), QStringLiteral("updatedAt")});
  if (!unknown) return tl::unexpected(unknown.error());

  const auto id = json::requiredString(object, QStringLiteral("id"), 36, 38, true);
  if (!id) return tl::unexpected(id.error());
  const QUuid uuid(id.value());
  if (uuid.isNull()) {
    return tl::unexpected(makeError(ErrorCode::ValidationFailed,
                                    QStringLiteral("id is not a UUID")));
  }
  const auto title = json::requiredString(
      object, QStringLiteral("title"), 1, 200, true);
  if (!title) return tl::unexpected(title.error());
  const auto description = json::optionalString(
      object, QStringLiteral("description"), 4000);
  if (!description) return tl::unexpected(description.error());
  const auto start = json::requiredDateTime(object, QStringLiteral("start"));
  if (!start) return tl::unexpected(start.error());
  const auto end = json::requiredDateTime(object, QStringLiteral("end"));
  if (!end) return tl::unexpected(end.error());
  if (end.value() < start.value()) {
    return tl::unexpected(makeError(ErrorCode::ValidationFailed,
                                    QStringLiteral("end precedes start")));
  }
  const auto allDay = json::requiredBool(object, QStringLiteral("allDay"));
  if (!allDay) return tl::unexpected(allDay.error());
  const auto location = json::optionalString(
      object, QStringLiteral("location"), 200, true);
  if (!location) return tl::unexpected(location.error());
  const auto color = json::requiredString(
      object, QStringLiteral("color"), 1, 20, true);
  if (!color) return tl::unexpected(color.error());
  static const QSet<QString> kPalette = {
      QStringLiteral("cyan"), QStringLiteral("blue"),
      QStringLiteral("violet"), QStringLiteral("green"),
      QStringLiteral("amber"), QStringLiteral("red"),
      QStringLiteral("magenta"), QStringLiteral("slate")};
  if (!kPalette.contains(color.value())) {
    return tl::unexpected(makeError(ErrorCode::ValidationFailed,
                                    QStringLiteral("unknown calendar color")));
  }
  const auto created = json::requiredDateTime(
      object, QStringLiteral("createdAt"));
  if (!created) return tl::unexpected(created.error());
  const auto updated = json::requiredDateTime(
      object, QStringLiteral("updatedAt"));
  if (!updated) return tl::unexpected(updated.error());
  if (updated.value() < created.value()) {
    return tl::unexpected(makeError(ErrorCode::ValidationFailed,
                                    QStringLiteral("updatedAt precedes createdAt")));
  }

  return CalendarEventDto{id.value(), title.value(), description.value(),
                          start.value(), end.value(), allDay.value(),
                          location.value(), color.value(), created.value(),
                          updated.value()};
}

QJsonObject CalendarEventDto::toJson() const {
  return {{QStringLiteral("id"), id},
          {QStringLiteral("title"), title},
          {QStringLiteral("description"), description},
          {QStringLiteral("start"), json::dateTimeValue(start)},
          {QStringLiteral("end"), json::dateTimeValue(end)},
          {QStringLiteral("allDay"), allDay},
          {QStringLiteral("location"), location},
          {QStringLiteral("color"), color},
          {QStringLiteral("createdAt"), json::dateTimeValue(createdAt)},
          {QStringLiteral("updatedAt"), json::dateTimeValue(updatedAt)}};
}

}  // namespace aegis::dto
