#pragma once

#include <QJsonObject>
#include <QString>

#include "core/result.h"
#include "dto/enums.h"

namespace aegis::dto {

struct CreativeRequest {
  CreativeBackend backend = CreativeBackend::Ollama;
  QString model;
  QString prompt;
  double temperature = 0.7;
  int maxTokens = 1024;

  // Parses, validates, and applies the specified numeric clamps.
  static Result<CreativeRequest> fromJson(const QJsonObject& object);

  // Serializes this request into its stable JSON representation.
  [[nodiscard]] QJsonObject toJson() const;
};

struct CreativeResult {
  QString requestId;
  QString text;
  bool done = false;
  QString finishReason;
  quint64 outputBytes = 0;

  // Parses and fully validates a bounded creative result.
  static Result<CreativeResult> fromJson(const QJsonObject& object);

  // Serializes this result into its stable JSON representation.
  [[nodiscard]] QJsonObject toJson() const;
};

}  // namespace aegis::dto
