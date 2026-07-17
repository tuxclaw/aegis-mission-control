#pragma once

#include <QByteArray>
#include <QMutex>
#include <QString>

#include "core/result.h"

namespace aegis {

class AtomicFile {
 public:
  // Atomically replaces a target using a synced temporary file and rename.
  Result<void> write(const QString& targetPath, const QByteArray& data);

 private:
  QMutex mutex_;
};

}  // namespace aegis
