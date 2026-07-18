#include "dto/process_dto.h"

#include <limits>

#include <QJsonArray>

#include "dto/json_validation.h"

namespace aegis::dto {
namespace {

constexpr qsizetype kMaxProcesses = 10000;

Result<ProcessInfo> parseProcess(const QJsonObject& object) {
  const auto unknown = json::rejectUnknown(
      object, {QStringLiteral("pid"), QStringLiteral("name"),
               QStringLiteral("user"), QStringLiteral("cpuPct"),
               QStringLiteral("memPct"), QStringLiteral("command")});
  if (!unknown) return tl::unexpected(unknown.error());

  const auto pid = json::requiredInt(object, QStringLiteral("pid"), 1,
                                     std::numeric_limits<int>::max());
  if (!pid) return tl::unexpected(pid.error());
  const auto name =
      json::requiredString(object, QStringLiteral("name"), 1, 256, true);
  if (!name) return tl::unexpected(name.error());
  const auto user =
      json::requiredString(object, QStringLiteral("user"), 1, 256, true);
  if (!user) return tl::unexpected(user.error());
  const auto cpuPct = json::requiredNumber(
      object, QStringLiteral("cpuPct"), 0.0, 1000000.0);
  if (!cpuPct) return tl::unexpected(cpuPct.error());
  const auto memPct =
      json::requiredNumber(object, QStringLiteral("memPct"), 0.0, 100.0);
  if (!memPct) return tl::unexpected(memPct.error());
  const auto command =
      json::optionalString(object, QStringLiteral("command"), 512, true);
  if (!command) return tl::unexpected(command.error());

  return ProcessInfo{pid.value(), name.value(), user.value(), cpuPct.value(),
                     memPct.value(), command.value()};
}

}  // namespace

Result<ProcessDto> ProcessDto::fromJson(const QJsonObject& object) {
  const auto unknown = json::rejectUnknown(
      object, {QStringLiteral("processes"), QStringLiteral("sampledAt")});
  if (!unknown) return tl::unexpected(unknown.error());
  if (!object.value(QStringLiteral("processes")).isArray()) {
    return tl::unexpected(makeError(ErrorCode::ValidationFailed,
                                    QStringLiteral("processes must be an array")));
  }

  const auto array = object.value(QStringLiteral("processes")).toArray();
  if (array.size() > kMaxProcesses) {
    return tl::unexpected(makeError(
        ErrorCode::ValidationFailed,
        QStringLiteral("process count exceeds validation cap")));
  }

  QVector<ProcessInfo> processes;
  processes.reserve(array.size());
  for (const auto& value : array) {
    if (!value.isObject()) {
      return tl::unexpected(makeError(
          ErrorCode::ValidationFailed,
          QStringLiteral("process entry must be an object")));
    }
    const auto process = parseProcess(value.toObject());
    if (!process) return tl::unexpected(process.error());
    processes.append(process.value());
  }

  const auto sampledAt =
      json::requiredDateTime(object, QStringLiteral("sampledAt"));
  if (!sampledAt) return tl::unexpected(sampledAt.error());
  return ProcessDto{processes, sampledAt.value()};
}

QJsonObject ProcessDto::toJson() const {
  QJsonArray array;
  for (const auto& process : processes) {
    array.append(QJsonObject{{QStringLiteral("pid"), process.pid},
                             {QStringLiteral("name"), process.name},
                             {QStringLiteral("user"), process.user},
                             {QStringLiteral("cpuPct"), process.cpuPct},
                             {QStringLiteral("memPct"), process.memPct},
                             {QStringLiteral("command"), process.command}});
  }
  return {{QStringLiteral("processes"), array},
          {QStringLiteral("sampledAt"), json::dateTimeValue(sampledAt)}};
}

}  // namespace aegis::dto
