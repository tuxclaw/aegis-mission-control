#pragma once

#include <QJsonObject>
#include <QString>

#include "core/result.h"

namespace aegis::dto {

struct ModelDto {
  QString id;
  QString provider;
  QString label;
  bool isActive = false;

  // Parses and fully validates a model object.
  static Result<ModelDto> fromJson(const QJsonObject& object);

  // Serializes this model into its stable JSON representation.
  [[nodiscard]] QJsonObject toJson() const;
};

}  // namespace aegis::dto
