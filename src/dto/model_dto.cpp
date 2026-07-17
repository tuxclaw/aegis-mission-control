#include "dto/model_dto.h"

#include "dto/json_validation.h"

namespace aegis::dto {

Result<ModelDto> ModelDto::fromJson(const QJsonObject& object) {
  const auto unknown = json::rejectUnknown(
      object, {QStringLiteral("id"), QStringLiteral("provider"),
               QStringLiteral("label"), QStringLiteral("isActive")});
  if (!unknown) return tl::unexpected(unknown.error());
  const auto id = json::requiredString(object, QStringLiteral("id"), 1, 256, true);
  if (!id) return tl::unexpected(id.error());
  const auto provider = json::requiredString(
      object, QStringLiteral("provider"), 1, 128, true);
  if (!provider) return tl::unexpected(provider.error());
  const auto label = json::requiredString(
      object, QStringLiteral("label"), 1, 256, true);
  if (!label) return tl::unexpected(label.error());
  const auto active = json::requiredBool(object, QStringLiteral("isActive"));
  if (!active) return tl::unexpected(active.error());
  return ModelDto{id.value(), provider.value(), label.value(), active.value()};
}

QJsonObject ModelDto::toJson() const {
  return {{QStringLiteral("id"), id},
          {QStringLiteral("provider"), provider},
          {QStringLiteral("label"), label},
          {QStringLiteral("isActive"), isActive}};
}

}  // namespace aegis::dto
