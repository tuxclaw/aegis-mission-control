#include "dto/cron_job_dto.h"

#include <limits>

#include <QTimeZone>

#include "dto/json_validation.h"

namespace aegis::dto {
namespace {

Result<QString> parseSchedule(const QJsonObject& object) {
  const auto value = object.value(QStringLiteral("schedule"));
  if (!value.isObject()) {
    return tl::unexpected(makeError(ErrorCode::ValidationFailed,
                                    QStringLiteral("schedule must be an object")));
  }
  const auto schedule = value.toObject();
  const auto unknown = json::rejectUnknown(
      schedule, {QStringLiteral("expr"), QStringLiteral("kind"),
                 QStringLiteral("staggerMs"), QStringLiteral("tz")});
  if (!unknown) return tl::unexpected(unknown.error());
  const auto kind = json::requiredString(
      schedule, QStringLiteral("kind"), 1, 32, true);
  if (!kind) return tl::unexpected(kind.error());
  if (kind.value() != QStringLiteral("cron")) {
    return tl::unexpected(makeError(ErrorCode::ValidationFailed,
                                    QStringLiteral("unsupported schedule kind")));
  }
  const auto expression = json::requiredString(
      schedule, QStringLiteral("expr"), 1, 256, true);
  if (!expression) return tl::unexpected(expression.error());
  const auto timezone =
      json::optionalString(schedule, QStringLiteral("tz"), 128, true);
  if (!timezone) return tl::unexpected(timezone.error());
  if (schedule.contains(QStringLiteral("staggerMs"))) {
    const auto stagger = json::requiredUnsigned(
        schedule, QStringLiteral("staggerMs"),
        std::numeric_limits<quint64>::max());
    if (!stagger) return tl::unexpected(stagger.error());
  }
  return expression.value();
}

Result<QDateTime> optionalEpochMs(const QJsonObject& object,
                                  const QString& key) {
  if (!object.contains(key) || object.value(key).isNull()) {
    return QDateTime();
  }
  const auto timestamp = json::requiredUnsigned(
      object, key, static_cast<quint64>(std::numeric_limits<qint64>::max()));
  if (!timestamp) return tl::unexpected(timestamp.error());
  const auto dateTime = QDateTime::fromMSecsSinceEpoch(
      static_cast<qint64>(timestamp.value()), QTimeZone::UTC);
  if (!dateTime.isValid()) {
    return tl::unexpected(makeError(ErrorCode::ValidationFailed,
                                    QStringLiteral("invalid epoch timestamp")));
  }
  return dateTime;
}

Result<QString> commandText(const QJsonObject& object) {
  const auto payloadValue = object.value(QStringLiteral("payload"));
  if (!payloadValue.isObject()) {
    return tl::unexpected(makeError(ErrorCode::ValidationFailed,
                                    QStringLiteral("payload must be an object")));
  }
  const auto payload = payloadValue.toObject();
  const auto unknown = json::rejectUnknown(
      payload, {QStringLiteral("kind"), QStringLiteral("lightContext"),
                QStringLiteral("message"), QStringLiteral("model"),
                QStringLiteral("timeoutSeconds")});
  if (!unknown) return tl::unexpected(unknown.error());
  const auto message = json::optionalString(
      payload, QStringLiteral("message"), 65536, true);
  if (!message) return tl::unexpected(message.error());
  return message->left(500);
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
      object,
      {QStringLiteral("agentId"), QStringLiteral("createdAtMs"),
       QStringLiteral("deleteAfterRun"), QStringLiteral("delivery"),
       QStringLiteral("description"), QStringLiteral("enabled"),
       QStringLiteral("id"), QStringLiteral("lastDelivered"),
       QStringLiteral("lastDeliveryStatus"),
       QStringLiteral("lastFailureNotificationDeliveryStatus"),
       QStringLiteral("lastRunAtMs"), QStringLiteral("lastRunStatus"),
       QStringLiteral("name"), QStringLiteral("nextRunAtMs"),
       QStringLiteral("payload"), QStringLiteral("schedule"),
       QStringLiteral("sessionKey"), QStringLiteral("sessionTarget"),
       QStringLiteral("state"), QStringLiteral("status"),
       QStringLiteral("updatedAtMs"), QStringLiteral("wakeMode")});
  if (!unknown) return tl::unexpected(unknown.error());
  const auto id = json::requiredString(object, QStringLiteral("id"), 1, 128, true);
  if (!id) return tl::unexpected(id.error());
  const auto name = json::requiredString(
      object, QStringLiteral("name"), 1, 200, true);
  if (!name) return tl::unexpected(name.error());
  const auto schedule = parseSchedule(object);
  if (!schedule) return tl::unexpected(schedule.error());
  const auto enabled =
      json::requiredBool(object, QStringLiteral("enabled"));
  if (!enabled) return tl::unexpected(enabled.error());
  const auto command = commandText(object);
  if (!command) return tl::unexpected(command.error());
  const auto lastRun = optionalEpochMs(object, QStringLiteral("lastRunAtMs"));
  if (!lastRun) return tl::unexpected(lastRun.error());
  const auto nextRun = optionalEpochMs(object, QStringLiteral("nextRunAtMs"));
  if (!nextRun) return tl::unexpected(nextRun.error());
  auto lastResult = json::optionalString(
      object, QStringLiteral("lastRunStatus"), 128, true);
  if (!lastResult) return tl::unexpected(lastResult.error());
  if (lastResult->isEmpty()) {
    lastResult = json::optionalString(
        object, QStringLiteral("status"), 128, true);
    if (!lastResult) return tl::unexpected(lastResult.error());
  }
  const auto lastResultText = lastResult.value() == QStringLiteral("ok")
                                  ? QStringLiteral("success")
                                  : lastResult.value();
  return CronJobDto{id.value(), name.value(), schedule.value(),
                    enabled.value() ? CronState::Enabled
                                    : CronState::Disabled,
                    command.value(), lastRun.value(), nextRun.value(),
                    lastResultText};
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
