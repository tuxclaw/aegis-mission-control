#pragma once

#include <QDateTime>
#include <QJsonObject>
#include <QString>
#include <QVector>

#include "core/result.h"

namespace aegis::dto {

struct ProcessInfo {
  int pid = 0;
  QString name;
  QString user;
  double cpuPct = 0.0;
  double memPct = 0.0;
  QString command;
};

struct ProcessDto {
  QVector<ProcessInfo> processes;
  QDateTime sampledAt;

  // Parses and validates a complete process sample.
  static Result<ProcessDto> fromJson(const QJsonObject& object);
  // Serializes this sample into its stable JSON representation.
  [[nodiscard]] QJsonObject toJson() const;
};

}  // namespace aegis::dto
