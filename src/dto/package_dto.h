#pragma once

#include <QJsonObject>
#include <QString>

#include "core/result.h"

namespace aegis::dto {

struct PackageDto {
  QString name;
  QString version;
  QString source;
  QString summary;

  // Parses and fully validates package metadata.
  static Result<PackageDto> fromJson(const QJsonObject& object);

  // Serializes this package into its stable JSON representation.
  [[nodiscard]] QJsonObject toJson() const;
};

}  // namespace aegis::dto
