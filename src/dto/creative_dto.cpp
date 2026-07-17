#include "dto/creative_dto.h"

#include <QSet>

#include "dto/json_validation.h"

namespace aegis::dto {
namespace {

Result<CreativeBackend> parseBackend(const QJsonValue& value) {
  if (!value.isString()) {
    return tl::unexpected(makeError(ErrorCode::ValidationFailed,
                                    QStringLiteral("backend must be a string")));
  }
  const auto backend = value.toString();
  if (backend == QStringLiteral("ollama")) return CreativeBackend::Ollama;
  if (backend == QStringLiteral("gateway")) {
    return CreativeBackend::GatewayCreative;
  }
  return tl::unexpected(makeError(ErrorCode::ValidationFailed,
                                  QStringLiteral("unknown creative backend")));
}

QString backendName(CreativeBackend backend) {
  return backend == CreativeBackend::Ollama ? QStringLiteral("ollama")
                                             : QStringLiteral("gateway");
}

}  // namespace

Result<CreativeRequest> CreativeRequest::fromJson(const QJsonObject& object) {
  const auto unknown = json::rejectUnknown(
      object, {QStringLiteral("backend"), QStringLiteral("model"),
               QStringLiteral("prompt"), QStringLiteral("temperature"),
               QStringLiteral("maxTokens")});
  if (!unknown) return tl::unexpected(unknown.error());
  const auto backend = parseBackend(object.value(QStringLiteral("backend")));
  if (!backend) return tl::unexpected(backend.error());
  const auto model = json::requiredString(
      object, QStringLiteral("model"), 1, 256, true);
  if (!model) return tl::unexpected(model.error());
  const auto prompt = json::requiredString(
      object, QStringLiteral("prompt"), 1, 8000, true);
  if (!prompt) return tl::unexpected(prompt.error());
  const auto rawTemperature = json::requiredNumber(
      object, QStringLiteral("temperature"), -1000000.0, 1000000.0);
  if (!rawTemperature) return tl::unexpected(rawTemperature.error());
  const auto rawMaxTokens = json::requiredInt(
      object, QStringLiteral("maxTokens"), -1000000, 1000000);
  if (!rawMaxTokens) return tl::unexpected(rawMaxTokens.error());
  return CreativeRequest{backend.value(), model.value(), prompt.value(),
                         qBound(0.0, rawTemperature.value(), 2.0),
                         qBound(1, rawMaxTokens.value(), 8192)};
}

QJsonObject CreativeRequest::toJson() const {
  return {{QStringLiteral("backend"), backendName(backend)},
          {QStringLiteral("model"), model},
          {QStringLiteral("prompt"), prompt},
          {QStringLiteral("temperature"), temperature},
          {QStringLiteral("maxTokens"), maxTokens}};
}

Result<CreativeResult> CreativeResult::fromJson(const QJsonObject& object) {
  const auto unknown = json::rejectUnknown(
      object, {QStringLiteral("requestId"), QStringLiteral("text"),
               QStringLiteral("done"), QStringLiteral("finishReason"),
               QStringLiteral("outputBytes")});
  if (!unknown) return tl::unexpected(unknown.error());
  const auto requestId = json::requiredString(
      object, QStringLiteral("requestId"), 1, 128, true);
  if (!requestId) return tl::unexpected(requestId.error());
  const auto text = json::optionalString(
      object, QStringLiteral("text"), 16 * 1024 * 1024);
  if (!text) return tl::unexpected(text.error());
  const auto done = json::requiredBool(object, QStringLiteral("done"));
  if (!done) return tl::unexpected(done.error());
  const auto reason = json::optionalString(
      object, QStringLiteral("finishReason"), 32, true);
  if (!reason) return tl::unexpected(reason.error());
  static const QSet<QString> kFinishReasons = {
      QString(), QStringLiteral("stop"), QStringLiteral("length"),
      QStringLiteral("cancelled"), QStringLiteral("error")};
  if (!kFinishReasons.contains(reason.value()) ||
      (done.value() && reason->isEmpty())) {
    return tl::unexpected(makeError(ErrorCode::ValidationFailed,
                                    QStringLiteral("invalid finish reason")));
  }
  const auto bytes = json::requiredUnsigned(
      object, QStringLiteral("outputBytes"), 16ULL * 1024ULL * 1024ULL);
  if (!bytes) return tl::unexpected(bytes.error());
  if (bytes.value() != static_cast<quint64>(text->toUtf8().size())) {
    return tl::unexpected(makeError(ErrorCode::ValidationFailed,
                                    QStringLiteral("output byte count mismatch")));
  }
  return CreativeResult{requestId.value(), text.value(), done.value(),
                        reason.value(), bytes.value()};
}

QJsonObject CreativeResult::toJson() const {
  return {{QStringLiteral("requestId"), requestId},
          {QStringLiteral("text"), text},
          {QStringLiteral("done"), done},
          {QStringLiteral("finishReason"), finishReason},
          {QStringLiteral("outputBytes"), static_cast<double>(outputBytes)}};
}

}  // namespace aegis::dto
