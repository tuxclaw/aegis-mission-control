#include "dto/container_dto.h"

#include <QJsonArray>
#include <QRegularExpression>

#include "dto/json_validation.h"

namespace aegis::dto {
namespace {

constexpr qsizetype kMaxContainers = 10000;

Result<ContainerInfo> parseContainer(const QJsonObject& object) {
  const auto unknown = json::rejectUnknown(
      object, {QStringLiteral("id"), QStringLiteral("name"),
               QStringLiteral("image"), QStringLiteral("status"),
               QStringLiteral("ports"), QStringLiteral("created")});
  if (!unknown) return tl::unexpected(unknown.error());

  const auto id =
      json::requiredString(object, QStringLiteral("id"), 12, 12, true);
  if (!id) return tl::unexpected(id.error());
  static const QRegularExpression kContainerId(
      QStringLiteral(R"(^[0-9a-fA-F]{12}$)"));
  if (!kContainerId.match(id.value()).hasMatch()) {
    return tl::unexpected(makeError(ErrorCode::ValidationFailed,
                                    QStringLiteral("container id is invalid")));
  }

  const auto name =
      json::requiredString(object, QStringLiteral("name"), 1, 256, true);
  if (!name) return tl::unexpected(name.error());
  const auto image =
      json::requiredString(object, QStringLiteral("image"), 1, 2048, true);
  if (!image) return tl::unexpected(image.error());
  const auto status =
      json::requiredString(object, QStringLiteral("status"), 1, 64, true);
  if (!status) return tl::unexpected(status.error());
  static const QRegularExpression kStatus(
      QStringLiteral(R"(^[a-z][a-z0-9_-]*$)"));
  if (!kStatus.match(status.value()).hasMatch()) {
    return tl::unexpected(makeError(ErrorCode::ValidationFailed,
                                    QStringLiteral("container status is invalid")));
  }
  const auto ports =
      json::optionalString(object, QStringLiteral("ports"), 4096, true);
  if (!ports) return tl::unexpected(ports.error());
  const auto created =
      json::requiredDateTime(object, QStringLiteral("created"));
  if (!created) return tl::unexpected(created.error());

  return ContainerInfo{id.value(), name.value(), image.value(),
                       status.value(), ports.value(), created.value()};
}

}  // namespace

Result<ContainerDto> ContainerDto::fromJson(const QJsonObject& object) {
  const auto unknown = json::rejectUnknown(
      object,
      {QStringLiteral("containers"), QStringLiteral("sampledAt")});
  if (!unknown) return tl::unexpected(unknown.error());
  if (!object.value(QStringLiteral("containers")).isArray()) {
    return tl::unexpected(makeError(ErrorCode::ValidationFailed,
                                    QStringLiteral("containers must be an array")));
  }

  const auto array = object.value(QStringLiteral("containers")).toArray();
  if (array.size() > kMaxContainers) {
    return tl::unexpected(makeError(
        ErrorCode::ValidationFailed,
        QStringLiteral("container count exceeds validation cap")));
  }

  QVector<ContainerInfo> containers;
  containers.reserve(array.size());
  for (const auto& value : array) {
    if (!value.isObject()) {
      return tl::unexpected(makeError(
          ErrorCode::ValidationFailed,
          QStringLiteral("container entry must be an object")));
    }
    const auto container = parseContainer(value.toObject());
    if (!container) return tl::unexpected(container.error());
    containers.append(container.value());
  }

  const auto sampledAt =
      json::requiredDateTime(object, QStringLiteral("sampledAt"));
  if (!sampledAt) return tl::unexpected(sampledAt.error());
  return ContainerDto{containers, sampledAt.value()};
}

QJsonObject ContainerDto::toJson() const {
  QJsonArray array;
  for (const auto& container : containers) {
    array.append(QJsonObject{
        {QStringLiteral("id"), container.id},
        {QStringLiteral("name"), container.name},
        {QStringLiteral("image"), container.image},
        {QStringLiteral("status"), container.status},
        {QStringLiteral("ports"), container.ports},
        {QStringLiteral("created"), json::dateTimeValue(container.created)}});
  }
  return {{QStringLiteral("containers"), array},
          {QStringLiteral("sampledAt"), json::dateTimeValue(sampledAt)}};
}

}  // namespace aegis::dto
