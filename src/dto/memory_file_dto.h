#pragma once

#include <QDateTime>
#include <QJsonObject>
#include <QString>

#include "core/result.h"

namespace aegis::dto {

struct MemoryFileDto {
  QString name;
  QString relativePath;
  QString rootId;
  quint64 sizeBytes = 0;
  QDateTime modifiedAt;

  // Parses and fully validates memory-file metadata.
  static Result<MemoryFileDto> fromJson(const QJsonObject& object);

  // Serializes this metadata into its stable JSON representation.
  [[nodiscard]] QJsonObject toJson() const;
};

}  // namespace aegis::dto
