#include "dto/json_validation.h"

#include <cmath>
#include <limits>

#include <QJsonValue>

namespace aegis::dto::json {
namespace {

Result<void> invalid(const QString& detail) {
  return tl::unexpected(makeError(ErrorCode::ValidationFailed, detail));
}

bool isWellFormedText(const QString& value) {
  if (value.contains(QChar::Null)) {
    return false;
  }
  for (qsizetype index = 0; index < value.size(); ++index) {
    const auto character = value.at(index);
    if (character.isHighSurrogate()) {
      if (index + 1 >= value.size() || !value.at(index + 1).isLowSurrogate()) {
        return false;
      }
      ++index;
    } else if (character.isLowSurrogate()) {
      return false;
    }
  }
  return true;
}

Result<QString> checkedString(const QJsonValue& value, const QString& key,
                              qsizetype minLength, qsizetype maxLength,
                              bool trim) {
  if (!value.isString()) {
    return tl::unexpected(makeError(
        ErrorCode::ValidationFailed,
        QStringLiteral("%1 must be a string").arg(key)));
  }
  auto text = value.toString();
  if (trim) {
    text = text.trimmed();
  }
  if (text.size() < minLength || text.size() > maxLength ||
      !isWellFormedText(text)) {
    return tl::unexpected(makeError(
        ErrorCode::ValidationFailed,
        QStringLiteral("%1 has an invalid length or encoding").arg(key)));
  }
  return text;
}

Result<QDateTime> parseDateTime(const QJsonValue& value, const QString& key) {
  if (!value.isString()) {
    return tl::unexpected(makeError(
        ErrorCode::ValidationFailed,
        QStringLiteral("%1 must be an ISO timestamp").arg(key)));
  }
  auto parsed = QDateTime::fromString(value.toString(), Qt::ISODateWithMs);
  if (!parsed.isValid()) {
    parsed = QDateTime::fromString(value.toString(), Qt::ISODate);
  }
  if (!parsed.isValid() || parsed.timeSpec() == Qt::LocalTime) {
    return tl::unexpected(makeError(
        ErrorCode::ValidationFailed,
        QStringLiteral("%1 is not a valid zoned timestamp").arg(key)));
  }
  return parsed.toUTC();
}

}  // namespace

Result<void> rejectUnknown(const QJsonObject& object,
                           const QSet<QString>& allowed) {
  for (auto iterator = object.constBegin(); iterator != object.constEnd();
       ++iterator) {
    if (!allowed.contains(iterator.key())) {
      return invalid(QStringLiteral("unknown field: %1").arg(iterator.key()));
    }
  }
  return {};
}

Result<QString> requiredString(const QJsonObject& object, const QString& key,
                               qsizetype minLength, qsizetype maxLength,
                               bool trim) {
  if (!object.contains(key)) {
    return tl::unexpected(makeError(
        ErrorCode::ValidationFailed,
        QStringLiteral("missing required field: %1").arg(key)));
  }
  return checkedString(object.value(key), key, minLength, maxLength, trim);
}

Result<QString> optionalString(const QJsonObject& object, const QString& key,
                               qsizetype maxLength, bool trim) {
  if (!object.contains(key) || object.value(key).isNull()) {
    return QString();
  }
  return checkedString(object.value(key), key, 0, maxLength, trim);
}

Result<bool> requiredBool(const QJsonObject& object, const QString& key) {
  if (!object.contains(key) || !object.value(key).isBool()) {
    return tl::unexpected(makeError(
        ErrorCode::ValidationFailed,
        QStringLiteral("%1 must be a boolean").arg(key)));
  }
  return object.value(key).toBool();
}

Result<int> requiredInt(const QJsonObject& object, const QString& key,
                        int minimum, int maximum) {
  if (!object.contains(key) || !object.value(key).isDouble()) {
    return tl::unexpected(makeError(
        ErrorCode::ValidationFailed,
        QStringLiteral("%1 must be an integer").arg(key)));
  }
  const auto number = object.value(key).toDouble();
  if (!std::isfinite(number) || std::floor(number) != number ||
      number < static_cast<double>(minimum) ||
      number > static_cast<double>(maximum)) {
    return tl::unexpected(makeError(
        ErrorCode::ValidationFailed,
        QStringLiteral("%1 is outside its valid integer range").arg(key)));
  }
  return static_cast<int>(number);
}

Result<quint64> requiredUnsigned(const QJsonObject& object, const QString& key,
                                 quint64 maximum) {
  constexpr auto kMaxExactJsonInteger = 9007199254740991.0;
  if (!object.contains(key) || !object.value(key).isDouble()) {
    return tl::unexpected(makeError(
        ErrorCode::ValidationFailed,
        QStringLiteral("%1 must be an unsigned integer").arg(key)));
  }
  const auto number = object.value(key).toDouble();
  const auto effectiveMaximum =
      qMin(static_cast<double>(maximum), kMaxExactJsonInteger);
  if (!std::isfinite(number) || std::floor(number) != number || number < 0.0 ||
      number > effectiveMaximum) {
    return tl::unexpected(makeError(
        ErrorCode::ValidationFailed,
        QStringLiteral("%1 is outside its valid unsigned range").arg(key)));
  }
  return static_cast<quint64>(number);
}

Result<double> requiredNumber(const QJsonObject& object, const QString& key,
                              double minimum, double maximum) {
  if (!object.contains(key) || !object.value(key).isDouble()) {
    return tl::unexpected(makeError(
        ErrorCode::ValidationFailed,
        QStringLiteral("%1 must be a number").arg(key)));
  }
  const auto number = object.value(key).toDouble();
  if (!std::isfinite(number) || number < minimum || number > maximum) {
    return tl::unexpected(makeError(
        ErrorCode::ValidationFailed,
        QStringLiteral("%1 is outside its valid range").arg(key)));
  }
  return number;
}

Result<double> nullableNumber(const QJsonObject& object, const QString& key,
                              double minimum, double maximum) {
  if (!object.contains(key)) {
    return tl::unexpected(makeError(
        ErrorCode::ValidationFailed,
        QStringLiteral("missing required field: %1").arg(key)));
  }
  if (object.value(key).isNull()) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  return requiredNumber(object, key, minimum, maximum);
}

Result<QDateTime> requiredDateTime(const QJsonObject& object,
                                   const QString& key) {
  if (!object.contains(key)) {
    return tl::unexpected(makeError(
        ErrorCode::ValidationFailed,
        QStringLiteral("missing required field: %1").arg(key)));
  }
  return parseDateTime(object.value(key), key);
}

Result<QDateTime> optionalDateTime(const QJsonObject& object,
                                   const QString& key) {
  if (!object.contains(key) || object.value(key).isNull()) {
    return QDateTime();
  }
  return parseDateTime(object.value(key), key);
}

QJsonValue dateTimeValue(const QDateTime& value) {
  if (!value.isValid()) {
    return QJsonValue(QJsonValue::Null);
  }
  return value.toUTC().toString(Qt::ISODateWithMs);
}

}  // namespace aegis::dto::json
