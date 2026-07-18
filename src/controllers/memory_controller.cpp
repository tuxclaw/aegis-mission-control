#include "controllers/memory_controller.h"

#include <QTimer>

#include <utility>

#include "services/memory_service.h"

namespace aegis {

MemoryController::MemoryController(MemoryService* service, QObject* parent)
    : QObject(parent), service_(service), files_(this) {
  const auto ids = rootIds();
  if (!ids.isEmpty()) currentRoot_ = ids.first();
  QTimer::singleShot(0, this, &MemoryController::refresh);
}

MemoryFileModel* MemoryController::files() { return &files_; }

QString MemoryController::currentContent() const { return currentContent_; }

QString MemoryController::currentRoot() const { return currentRoot_; }

QStringList MemoryController::rootIds() const { return service_->rootIds(); }

void MemoryController::setRoot(QString rootId) {
  if (!rootIds().contains(rootId)) {
    const auto error = makeError(ErrorCode::PathOutsideSandbox);
    emit errorRaised(error.userMessage, error.retryable);
    return;
  }
  currentRoot_ = std::move(rootId);
  currentContent_.clear();
  emit currentRootChanged();
  emit currentContentChanged();
  refresh();
}

void MemoryController::selectFile(QString relativePath) {
  service_->read(currentRoot_, std::move(relativePath))
      .then(this, [this](const Result<QString>& result) {
        if (!result) {
          emit errorRaised(result.error().userMessage,
                           result.error().retryable);
          return;
        }
        currentContent_ = result.value();
        emit currentContentChanged();
      });
}

void MemoryController::refresh() {
  if (currentRoot_.isEmpty()) return;
  service_->list(currentRoot_)
      .then(this,
            [this](const Result<QVector<dto::MemoryFileDto>>& result) {
              if (!result) {
                emit errorRaised(result.error().userMessage,
                                 result.error().retryable);
                return;
              }
              files_.setItems(result.value());
              emit filesChanged();
            });
}

void MemoryController::reconfigureRoots() {
  const auto ids = rootIds();
  if (!ids.contains(currentRoot_)) {
    currentRoot_ = ids.isEmpty() ? QString() : ids.first();
    currentContent_.clear();
    emit currentRootChanged();
    emit currentContentChanged();
  }
  emit rootIdsChanged();
  refresh();
}

}  // namespace aegis
