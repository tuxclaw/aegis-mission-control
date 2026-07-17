#include "dto/memory_file_dto.h"

#include <QDir>
#include <QFileInfo>

#include "dto/json_validation.h"

namespace aegis::dto {

Result<MemoryFileDto> MemoryFileDto::fromJson(const QJsonObject& object) {
  const auto unknown = json::rejectUnknown(
      object, {QStringLiteral("name"), QStringLiteral("relativePath"),
               QStringLiteral("rootId"), QStringLiteral("sizeBytes"),
               QStringLiteral("modifiedAt")});
  if (!unknown) return tl::unexpected(unknown.error());
  const auto name = json::requiredString(
      object, QStringLiteral("name"), 3, 255, true);
  if (!name) return tl::unexpected(name.error());
  if (QFileInfo(name.value()).fileName() != name.value() ||
      !name->endsWith(QStringLiteral(".md"), Qt::CaseInsensitive)) {
    return tl::unexpected(makeError(ErrorCode::ValidationFailed,
                                    QStringLiteral("invalid memory basename")));
  }
  const auto path = json::requiredString(
      object, QStringLiteral("relativePath"), 3, 4096);
  if (!path) return tl::unexpected(path.error());
  if (QDir::isAbsolutePath(path.value()) || path->contains(QChar::Null) ||
      QDir::cleanPath(path.value()).startsWith(QStringLiteral("../")) ||
      QDir::cleanPath(path.value()) == QStringLiteral("..") ||
      !path->endsWith(QStringLiteral(".md"), Qt::CaseInsensitive)) {
    return tl::unexpected(makeError(ErrorCode::ValidationFailed,
                                    QStringLiteral("invalid relative memory path")));
  }
  const auto root = json::requiredString(
      object, QStringLiteral("rootId"), 1, 128, true);
  if (!root) return tl::unexpected(root.error());
  const auto size = json::requiredUnsigned(
      object, QStringLiteral("sizeBytes"), 64ULL * 1024ULL * 1024ULL);
  if (!size) return tl::unexpected(size.error());
  const auto modified = json::requiredDateTime(
      object, QStringLiteral("modifiedAt"));
  if (!modified) return tl::unexpected(modified.error());
  return MemoryFileDto{name.value(), path.value(), root.value(), size.value(),
                       modified.value()};
}

QJsonObject MemoryFileDto::toJson() const {
  return {{QStringLiteral("name"), name},
          {QStringLiteral("relativePath"), relativePath},
          {QStringLiteral("rootId"), rootId},
          {QStringLiteral("sizeBytes"), static_cast<double>(sizeBytes)},
          {QStringLiteral("modifiedAt"), json::dateTimeValue(modifiedAt)}};
}

}  // namespace aegis::dto
