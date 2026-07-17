#include "controllers/git_controller.h"

#include "services/git_service.h"

namespace aegis {
GitController::GitController(GitService* service, QObject* parent)
    : QObject(parent), service_(service), files_(this) {
  connect(service_, &GitService::repoChanged, this, &GitController::refresh);
}
GitFileModel* GitController::files() { return &files_; }
QString GitController::branch() const { return status_.branch; }
int GitController::ahead() const { return status_.ahead; }
int GitController::behind() const { return status_.behind; }
bool GitController::clean() const { return status_.clean; }
bool GitController::busy() const { return busy_; }
bool GitController::repoConfigured() const { return !status_.repoPath.isEmpty(); }
void GitController::setBusy(bool value) {
  if (value == busy_) return;
  busy_ = value;
  emit busyChanged();
}
void GitController::refresh() {
  if (busy_) return;
  setBusy(true);
  service_->status().then(this, [this](const Result<dto::GitStatusDto>& result) {
    setBusy(false);
    if (!result) {
      emit errorRaised(result.error().userMessage, result.error().retryable);
      return;
    }
    status_ = result.value();
    files_.setItems(status_.entries);
    emit statusChanged();
  });
}
void GitController::stagePath(QString path) {
  service_->stage({std::move(path)}).then(this,
      [this](const Result<void>& result) { handle(result); });
}
void GitController::unstagePath(QString path) {
  service_->unstage({std::move(path)}).then(this,
      [this](const Result<void>& result) { handle(result); });
}
void GitController::commit(QString message) {
  pendingMessage_ = std::move(message);
  emit confirmRequested(QStringLiteral("commit"),
                        QStringLiteral("Commit the explicitly staged files?"));
}
void GitController::pull() {
  emit confirmRequested(QStringLiteral("pull"),
                        QStringLiteral("Fast-forward from the configured remote?"));
}
void GitController::push() {
  emit confirmRequested(QStringLiteral("push"),
                        QStringLiteral("Push the current branch?"));
}
void GitController::confirmCommit() {
  setBusy(true);
  service_->commit(pendingMessage_, files_.stagedPaths()).then(
      this, [this](const Result<QString>& result) {
        setBusy(false);
        if (!result) emit errorRaised(result.error().userMessage,
                                      result.error().retryable);
        else {
          emit toast(QStringLiteral("Committed %1").arg(result.value()), 1);
          refresh();
        }
      });
}
void GitController::confirmPull() {
  setBusy(true);
  service_->pull().then(this, [this](const Result<void>& result) {
    setBusy(false);
    handle(result);
  });
}
void GitController::confirmPush() {
  setBusy(true);
  service_->push().then(this, [this](const Result<void>& result) {
    setBusy(false);
    handle(result);
  });
}
void GitController::handle(Result<void> result) {
  if (!result) emit errorRaised(result.error().userMessage,
                                result.error().retryable);
  else refresh();
}
}  // namespace aegis
