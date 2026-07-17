#pragma once

#include <QDateTime>
#include <QJsonObject>
#include <QSet>
#include <QString>

#include "core/result.h"

namespace aegis::dto::json {

Result<void> rejectUnknown(const QJsonObject& object,
                           const QSet<QString>& allowed);
Result<QString> requiredString(const QJsonObject& object, const QString& key,
                               qsizetype minLength, qsizetype maxLength,
                               bool trim = false);
Result<QString> optionalString(const QJsonObject& object, const QString& key,
                               qsizetype maxLength, bool trim = false);
Result<bool> requiredBool(const QJsonObject& object, const QString& key);
Result<int> requiredInt(const QJsonObject& object, const QString& key,
                        int minimum, int maximum);
Result<quint64> requiredUnsigned(const QJsonObject& object, const QString& key,
                                 quint64 maximum);
Result<double> requiredNumber(const QJsonObject& object, const QString& key,
                              double minimum, double maximum);
Result<double> nullableNumber(const QJsonObject& object, const QString& key,
                              double minimum, double maximum);
Result<QDateTime> requiredDateTime(const QJsonObject& object,
                                   const QString& key);
Result<QDateTime> optionalDateTime(const QJsonObject& object,
                                   const QString& key);
QJsonValue dateTimeValue(const QDateTime& value);

}  // namespace aegis::dto::json
