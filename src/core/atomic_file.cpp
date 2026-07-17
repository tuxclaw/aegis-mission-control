#include "core/atomic_file.h"

#include <QMutexLocker>
#include <QSaveFile>

namespace aegis {

Result<void> AtomicFile::write(const QString& targetPath,
                               const QByteArray& data) {
  QMutexLocker locker(&mutex_);
  QSaveFile file(targetPath);
  file.setDirectWriteFallback(false);
  if (!file.open(QIODevice::WriteOnly)) {
    return tl::unexpected(makeError(ErrorCode::WriteFailed,
                                    QStringLiteral("temporary file open failed")));
  }
  if (file.write(data) != data.size()) {
    file.cancelWriting();
    return tl::unexpected(makeError(ErrorCode::WriteFailed,
                                    QStringLiteral("temporary file write failed")));
  }
  if (!file.commit()) {
    return tl::unexpected(makeError(ErrorCode::WriteFailed,
                                    QStringLiteral("atomic commit failed")));
  }
  return {};
}

}  // namespace aegis
