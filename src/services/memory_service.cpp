#include "services/memory_service.h"

#include <algorithm>

#include <QDirIterator>
#include <QFile>

#include "config/config_service.h"
#include "core/async.h"
#include "core/path_guard.h"

namespace aegis {

MemoryService::MemoryService(ConfigService* config, QObject* parent)
    : QObject(parent), config_(config) {}

QFuture<Result<QVector<dto::MemoryFileDto>>> MemoryService::list(
    QString rootId) {
  const auto roots = config_->memoryRoots();
  const auto cap = config_->memoryMaxFileBytes();
  return async::run([roots, cap, rootId = std::move(rootId)]()
                        -> Result<QVector<dto::MemoryFileDto>> {
    if (!roots) return tl::unexpected(roots.error());
    if (!cap) return tl::unexpected(cap.error());
    if (!roots->contains(rootId)) {
      return tl::unexpected(makeError(ErrorCode::PathOutsideSandbox,
                                      QStringLiteral("memory root not allowlisted")));
    }
    const auto guard = PathGuard::create(roots->value(rootId), cap.value());
    if (!guard) return tl::unexpected(guard.error());
    QVector<dto::MemoryFileDto> files;
    QDirIterator iterator(guard->rootCanonical(),
                          {QStringLiteral("*.md")}, QDir::Files,
                          QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
      iterator.next();
      const auto relative = QDir(guard->rootCanonical()).relativeFilePath(
          iterator.filePath());
      const auto resolved = guard->resolve(relative);
      if (!resolved) return tl::unexpected(resolved.error());
      const QFileInfo info(resolved.value());
      files.append(dto::MemoryFileDto{
          info.fileName(), relative, rootId, static_cast<quint64>(info.size()),
          info.lastModified().toUTC()});
    }
    std::sort(files.begin(), files.end(), [](const auto& left,
                                             const auto& right) {
      return left.relativePath < right.relativePath;
    });
    return files;
  });
}

QFuture<Result<QString>> MemoryService::read(QString rootId,
                                              QString relativePath) {
  const auto roots = config_->memoryRoots();
  const auto cap = config_->memoryMaxFileBytes();
  return async::run([roots, cap, rootId = std::move(rootId),
                     relativePath = std::move(relativePath)]()
                        -> Result<QString> {
    if (!roots) return tl::unexpected(roots.error());
    if (!cap) return tl::unexpected(cap.error());
    if (!roots->contains(rootId)) {
      return tl::unexpected(makeError(ErrorCode::PathOutsideSandbox,
                                      QStringLiteral("memory root not allowlisted")));
    }
    const auto guard = PathGuard::create(roots->value(rootId), cap.value());
    if (!guard) return tl::unexpected(guard.error());
    const auto path = guard->resolve(relativePath, true);
    if (!path) return tl::unexpected(path.error());
    QFile file(path.value());
    if (!file.open(QIODevice::ReadOnly)) {
      return tl::unexpected(makeError(ErrorCode::PathNotFound,
                                      QStringLiteral("memory file unreadable")));
    }
    const auto data = file.readAll();
    if (static_cast<quint64>(data.size()) > cap.value()) {
      return tl::unexpected(makeError(ErrorCode::FileTooLarge,
                                      QStringLiteral("memory file exceeded cap")));
    }
    return QString::fromUtf8(data);
  });
}

QStringList MemoryService::rootIds() const {
  const auto roots = config_->memoryRoots();
  return roots ? roots->keys() : QStringList();
}

}  // namespace aegis
