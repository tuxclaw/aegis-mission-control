#pragma once

#include <QDateTime>
#include <QJsonObject>
#include <QString>
#include <QVector>

#include "core/result.h"

namespace aegis::dto {

struct ContainerInfo {
  QString id;
  QString name;
  QString image;
  QString status;
  QString ports;
  QDateTime created;
};

struct ContainerDto {
  QVector<ContainerInfo> containers;
  QDateTime sampledAt;

  // Parses and validates a complete container inventory sample.
  static Result<ContainerDto> fromJson(const QJsonObject& object);
  // Serializes this sample into its stable JSON representation.
  [[nodiscard]] QJsonObject toJson() const;
};

}  // namespace aegis::dto
