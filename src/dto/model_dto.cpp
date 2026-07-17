#include "dto/model_dto.h"

#include <limits>

#include <QJsonArray>

#include "dto/json_validation.h"

namespace aegis::dto {

Result<ModelDto> ModelDto::fromJson(const QJsonObject& object) {
  const auto unknown = json::rejectUnknown(
      object, {QStringLiteral("available"), QStringLiteral("contextWindow"),
               QStringLiteral("input"), QStringLiteral("key"),
               QStringLiteral("local"), QStringLiteral("missing"),
               QStringLiteral("name"), QStringLiteral("tags")});
  if (!unknown) return tl::unexpected(unknown.error());
  const auto id =
      json::requiredString(object, QStringLiteral("key"), 1, 256, true);
  if (!id) return tl::unexpected(id.error());
  const auto label =
      json::optionalString(object, QStringLiteral("name"), 256, true);
  if (!label) return tl::unexpected(label.error());
  const auto input =
      json::optionalString(object, QStringLiteral("input"), 128, true);
  if (!input) return tl::unexpected(input.error());
  for (const auto& key : {QStringLiteral("available"), QStringLiteral("local"),
                          QStringLiteral("missing")}) {
    const auto value = json::requiredBool(object, key);
    if (!value) return tl::unexpected(value.error());
  }
  const auto contextWindow = json::requiredUnsigned(
      object, QStringLiteral("contextWindow"),
      std::numeric_limits<quint64>::max());
  if (!contextWindow) return tl::unexpected(contextWindow.error());

  const auto tagsValue = object.value(QStringLiteral("tags"));
  if (!tagsValue.isArray()) {
    return tl::unexpected(makeError(ErrorCode::ValidationFailed,
                                    QStringLiteral("tags must be an array")));
  }
  bool isActive = false;
  for (const auto& tag : tagsValue.toArray()) {
    if (!tag.isString()) {
      return tl::unexpected(makeError(
          ErrorCode::ValidationFailed,
          QStringLiteral("model tags must contain strings")));
    }
    if (tag.toString() == QStringLiteral("default")) isActive = true;
  }

  const auto separator = id->indexOf(QLatin1Char('/'));
  if (separator <= 0 || separator > 128) {
    return tl::unexpected(makeError(ErrorCode::ValidationFailed,
                                    QStringLiteral("model key has no provider")));
  }
  const auto provider = id->left(separator);
  return ModelDto{id.value(), provider,
                  label->isEmpty() ? id.value() : label.value(), isActive};
}

QJsonObject ModelDto::toJson() const {
  return {{QStringLiteral("id"), id},
          {QStringLiteral("provider"), provider},
          {QStringLiteral("label"), label},
          {QStringLiteral("isActive"), isActive}};
}

}  // namespace aegis::dto
