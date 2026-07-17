#pragma once

#include <QFuture>
#include <QObject>
#include <QStringList>
#include <QVector>

#include "core/result.h"
#include "dto/memory_file_dto.h"

namespace aegis {

class ConfigService;

class MemoryService : public QObject {
  Q_OBJECT

 public:
  // Creates a sandboxed memory browser from configured allowlisted roots.
  explicit MemoryService(ConfigService* config, QObject* parent = nullptr);
  // Lists validated Markdown files below an allowlisted root.
  [[nodiscard]] QFuture<Result<QVector<dto::MemoryFileDto>>> list(
      QString rootId);
  // Reads a validated Markdown file as inert UTF-8 text.
  [[nodiscard]] QFuture<Result<QString>> read(QString rootId,
                                               QString relativePath);
  // Returns configured allowlisted root identifiers.
  [[nodiscard]] QStringList rootIds() const;

 private:
  ConfigService* config_;
};

}  // namespace aegis
