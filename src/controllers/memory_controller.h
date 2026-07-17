#pragma once

#include <QObject>
#include <QStringList>

#include "models/memory_file_model.h"

namespace aegis {
class MemoryService;

class MemoryController final : public QObject {
  Q_OBJECT
  Q_PROPERTY(MemoryFileModel* files READ files CONSTANT)
  Q_PROPERTY(QString currentContent READ currentContent NOTIFY currentContentChanged)
  Q_PROPERTY(QString currentRoot READ currentRoot NOTIFY currentRootChanged)
  Q_PROPERTY(QStringList rootIds READ rootIds CONSTANT)

 public:
  // Creates a sandboxed memory-view controller.
  explicit MemoryController(MemoryService* service, QObject* parent = nullptr);
  [[nodiscard]] MemoryFileModel* files();
  [[nodiscard]] QString currentContent() const;
  [[nodiscard]] QString currentRoot() const;
  [[nodiscard]] QStringList rootIds() const;
  Q_INVOKABLE void setRoot(QString rootId);
  Q_INVOKABLE void selectFile(QString relativePath);
  Q_INVOKABLE void refresh();

 signals:
  void currentContentChanged();
  void currentRootChanged();
  void errorRaised(QString message, bool retryable);

 private:
  MemoryService* service_;
  MemoryFileModel files_;
  QString currentContent_;
  QString currentRoot_;
};
}  // namespace aegis
