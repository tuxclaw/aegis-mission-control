#include "dto/cron_job_dto.h"

#include "dto/json_validation.h"

namespace aegis::dto {
namespace {

Result<CronState> parseState(const QJsonValue& value) {
  if (!value.isString()) {
    return tl::unexpected(makeError(ErrorCode::ValidationFailed,
                                    QStringLiteral("state must be a string")));
  }
  const auto state = value.toString();
  if (state == QStringLiteral("enabled")) return CronState::Enabled;
  if (state == QStringLiteral("disabled")) return CronState::Disabled;
  if (state == QStringLiteral("unknown")) return CronState::Unknown;
  return tl::unexpected(makeError(ErrorCode::ValidationFailed,
                                  QStringLiteral("unknown cron state")));
}

QString stateName(CronState state) {
  switch (state) {
    case CronState::Enabled: return QStringLiteral("enabled");
    case CronState::Disabled: return QStringLiteral("disabled");
    case CronState::Unknown: return QStringLiteral("unknown");
  }
  return QStringLiteral("unknown");
}

}  // namespace

Result<CronJobDto> CronJobDto::fromJson(const QJsonObject& object) {
  const auto unknown = json::rejectUnknown(
      object, {QStringLiteral("id"), QStringLiteral("name"),
               QStringLiteral("schedule"), QStringLiteral("state"),
               QStringLiteral("command"), QStringLiteral("lastRun"),
               QStringLiteral("nextRun"), QStringLiteral("lastResult")});
  if (!unknown) return tl::unexpected(unknown.error());
  const auto id = json::requiredString(object, QStringLiteral("id"), 1, 128, true);
  if (!id) return tl::unexpected(id.error());
  const auto name = json::requiredString(
      object, QStringLiteral("name"), 1, 200, true);
  if (!name) return tl::unexpected(name.error());
  const auto schedule = json::requiredString(
      object, QStringLiteral("schedule"), 1, 256, true);
  if (!schedule) return tl::unexpected(schedule.error());
  const auto state = parseState(object.value(QStringLiteral("state")));
  if (!state) return tl::unexpected(state.error());
  const auto command = json::optionalString(
      object, QStringLiteral("command"), 500, true);
  if (!command) return tl::unexpected(command.error());
  const auto lastRun = json::optionalDateTime(
      object, QStringLiteral("lastRun"));
  if (!lastRun) return tl::unexpected(lastRun.error());
  const auto nextRun = json::optionalDateTime(
      object, QStringLiteral("nextRun"));
  if (!nextRun) return tl::unexpected(nextRun.error());
  const auto result = json::optionalString(
      object, QStringLiteral("lastResult"), 500, true);
  if (!result) return tl::unexpected(result.error());
  return CronJobDto{id.value(), name.value(), schedule.value(), state.value(),
                    command.value(), lastRun.value(), nextRun.value(),
                    result.value()};
}

QJsonObject CronJobDto::toJson() const {
  return {{QStringLiteral("id"), id},
          {QStringLiteral("name"), name},
          {QStringLiteral("schedule"), schedule},
          {QStringLiteral("state"), stateName(state)},
          {QStringLiteral("command"), command},
          {QStringLiteral("lastRun"), json::dateTimeValue(lastRun)},
          {QStringLiteral("nextRun"), json::dateTimeValue(nextRun)},
          {QStringLiteral("lastResult"), lastResult}};
}

}  // namespace aegis::dto
