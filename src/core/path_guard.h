#pragma once

#include <QtGlobal>

#include <QString>

#include "core/result.h"

namespace aegis {

class PathGuard {
 public:
  // Creates a guard around an existing canonical root directory.
  static Result<PathGuard> create(const QString& rootPath, quint64 maxFileBytes);

  // Resolves and validates a relative path inside the guarded root.
  [[nodiscard]] Result<QString> resolve(const QString& relativePath,
                                        bool requireMarkdown = true) const;

  // Returns the canonical allowlisted root.
  [[nodiscard]] const QString& rootCanonical() const;

 private:
  PathGuard(QString rootCanonical, quint64 maxFileBytes);

  [[nodiscard]] bool isContained(const QString& canonicalPath) const;
  [[nodiscard]] Result<void> rejectEscapingSymlink(
      const QString& relativePath) const;

  QString rootCanonical_;
  quint64 maxFileBytes_;
};

}  // namespace aegis
