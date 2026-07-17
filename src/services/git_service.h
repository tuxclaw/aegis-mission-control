#pragma once

#include <QFuture>
#include <QObject>
#include <QStringList>

#include "core/result.h"
#include "dto/git_status_dto.h"

namespace aegis {

class ConfigService;
class SecretStore;

class GitService : public QObject {
  Q_OBJECT

 public:
  // Creates a libgit2 repository service from validated configuration.
  explicit GitService(ConfigService* config, SecretStore* secrets,
                      QObject* parent = nullptr);
  // Releases the process-wide libgit2 initialization when it succeeded.
  static void shutdownLibrary();
  // Returns current worktree, branch, divergence, and file status.
  [[nodiscard]] QFuture<Result<dto::GitStatusDto>> status();
  // Stages exactly the validated repository-relative paths.
  [[nodiscard]] QFuture<Result<void>> stage(QStringList explicitPaths);
  // Unstages exactly the validated repository-relative paths.
  [[nodiscard]] QFuture<Result<void>> unstage(QStringList explicitPaths);
  // Commits only when the explicit path set matches the staged set.
  [[nodiscard]] QFuture<Result<QString>> commit(QString message,
                                                 QStringList stagedPaths);
  // Fetches and fast-forwards the configured remote only.
  [[nodiscard]] QFuture<Result<void>> pull();
  // Pushes the current branch to the configured remote.
  [[nodiscard]] QFuture<Result<void>> push();

 signals:
  void repoChanged();

 private:
  ConfigService* config_;
  SecretStore* secrets_;
};

}  // namespace aegis
