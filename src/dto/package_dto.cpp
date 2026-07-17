#include "dto/package_dto.h"

#include "dto/json_validation.h"

namespace aegis::dto {

Result<PackageDto> PackageDto::fromJson(const QJsonObject& object) {
  const auto unknown = json::rejectUnknown(
      object, {QStringLiteral("name"), QStringLiteral("version"),
               QStringLiteral("source"), QStringLiteral("summary")});
  if (!unknown) return tl::unexpected(unknown.error());
  const auto name = json::requiredString(
      object, QStringLiteral("name"), 1, 512, true);
  if (!name) return tl::unexpected(name.error());
  const auto version = json::requiredString(
      object, QStringLiteral("version"), 1, 512, true);
  if (!version) return tl::unexpected(version.error());
  const auto source = json::requiredString(
      object, QStringLiteral("source"), 1, 64, true);
  if (!source) return tl::unexpected(source.error());
  const auto summary = json::optionalString(
      object, QStringLiteral("summary"), 200, true);
  if (!summary) return tl::unexpected(summary.error());
  return PackageDto{name.value(), version.value(), source.value(), summary.value()};
}

QJsonObject PackageDto::toJson() const {
  return {{QStringLiteral("name"), name},
          {QStringLiteral("version"), version},
          {QStringLiteral("source"), source},
          {QStringLiteral("summary"), summary}};
}

}  // namespace aegis::dto
