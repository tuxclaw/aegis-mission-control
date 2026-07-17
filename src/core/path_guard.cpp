#include "core/path_guard.h"

#include <QDir>
#include <QFileInfo>
#include <QStringList>

namespace aegis {

PathGuard::PathGuard(QString rootCanonical, quint64 maxFileBytes)
    : rootCanonical_(std::move(rootCanonical)), maxFileBytes_(maxFileBytes) {}

Result<PathGuard> PathGuard::create(const QString& rootPath,
                                    quint64 maxFileBytes) {
  const QFileInfo rootInfo(rootPath);
  const auto canonical = rootInfo.canonicalFilePath();
  if (canonical.isEmpty() || !rootInfo.isDir()) {
    return tl::unexpected(makeError(ErrorCode::PathNotFound,
                                    QStringLiteral("allowlisted root is missing")));
  }
  if (maxFileBytes == 0) {
    return tl::unexpected(makeError(ErrorCode::ConfigInvalid,
                                    QStringLiteral("path size cap is zero")));
  }
  return PathGuard(canonical, maxFileBytes);
}

Result<QString> PathGuard::resolve(const QString& relativePath,
                                   bool requireMarkdown) const {
  if (relativePath.contains(QChar::Null)) {
    return tl::unexpected(makeError(ErrorCode::ValidationFailed,
                                    QStringLiteral("path contains NUL")));
  }
  if (QDir::isAbsolutePath(relativePath)) {
    return tl::unexpected(makeError(ErrorCode::PathOutsideSandbox,
                                    QStringLiteral("absolute path rejected")));
  }

  const auto symlinkResult = rejectEscapingSymlink(relativePath);
  if (!symlinkResult) {
    return tl::unexpected(symlinkResult.error());
  }

  const auto joined = QDir(rootCanonical_).filePath(relativePath);
  const QFileInfo fileInfo(joined);
  const auto canonical = fileInfo.canonicalFilePath();
  if (canonical.isEmpty()) {
    return tl::unexpected(makeError(ErrorCode::PathNotFound,
                                    QStringLiteral("canonical path is empty")));
  }
  if (!isContained(canonical)) {
    return tl::unexpected(makeError(ErrorCode::PathOutsideSandbox,
                                    QStringLiteral("canonical path escaped root")));
  }
  if (requireMarkdown &&
      !canonical.endsWith(QStringLiteral(".md"), Qt::CaseInsensitive)) {
    return tl::unexpected(makeError(ErrorCode::ValidationFailed,
                                    QStringLiteral("non-markdown file rejected")));
  }
  if (!fileInfo.isFile()) {
    return tl::unexpected(makeError(ErrorCode::PathNotFound,
                                    QStringLiteral("path is not a regular file")));
  }
  const auto size = fileInfo.size();
  if (size < 0 || static_cast<quint64>(size) > maxFileBytes_) {
    return tl::unexpected(makeError(ErrorCode::FileTooLarge,
                                    QStringLiteral("file exceeded configured cap")));
  }
  return canonical;
}

const QString& PathGuard::rootCanonical() const { return rootCanonical_; }

bool PathGuard::isContained(const QString& canonicalPath) const {
  return canonicalPath == rootCanonical_ ||
         canonicalPath.startsWith(rootCanonical_ + QDir::separator());
}

Result<void> PathGuard::rejectEscapingSymlink(
    const QString& relativePath) const {
  const auto normalized = QDir::cleanPath(relativePath);
  const auto components = normalized.split(QDir::separator(), Qt::SkipEmptyParts);
  auto current = rootCanonical_;
  for (const auto& component : components) {
    current = QDir(current).filePath(component);
    const QFileInfo info(current);
    if (!info.exists() && !info.isSymLink()) {
      break;
    }
    if (info.isSymLink()) {
      const auto target = info.canonicalFilePath();
      if (target.isEmpty() || !isContained(target)) {
        return tl::unexpected(makeError(
            ErrorCode::PathSymlinkRejected,
            QStringLiteral("symlink component escaped allowlisted root")));
      }
    }
  }
  return {};
}

}  // namespace aegis
