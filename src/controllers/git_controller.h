#pragma once

#include <QObject>

#include "models/git_file_model.h"

namespace aegis {
class ConfigService;
class GitService;

class GitController final : public QObject {
  Q_OBJECT
  Q_PROPERTY(GitFileModel* files READ files CONSTANT)
  Q_PROPERTY(QString branch READ branch NOTIFY statusChanged)
  Q_PROPERTY(int ahead READ ahead NOTIFY statusChanged)
  Q_PROPERTY(int behind READ behind NOTIFY statusChanged)
  Q_PROPERTY(bool clean READ clean NOTIFY statusChanged)
  Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
  Q_PROPERTY(bool repoConfigured READ repoConfigured NOTIFY statusChanged)

 public:
  // Creates a confirmed-intent adapter over GitService.
  explicit GitController(GitService* service, ConfigService* config,
                         QObject* parent = nullptr);
  [[nodiscard]] GitFileModel* files();
  [[nodiscard]] QString branch() const;
  [[nodiscard]] int ahead() const;
  [[nodiscard]] int behind() const;
  [[nodiscard]] bool clean() const;
  [[nodiscard]] bool busy() const;
  [[nodiscard]] bool repoConfigured() const;
  // Refreshes repository state asynchronously.
  Q_INVOKABLE void refresh();
  // Stages one explicit repository-relative path.
  Q_INVOKABLE void stagePath(QString path);
  // Unstages one explicit repository-relative path.
  Q_INVOKABLE void unstagePath(QString path);
  // Begins two-phase confirmation for a commit.
  Q_INVOKABLE void commit(QString message);
  // Begins two-phase confirmation for an ff-only pull.
  Q_INVOKABLE void pull();
  // Begins two-phase confirmation for a push.
  Q_INVOKABLE void push();
  // Executes the pending confirmed commit.
  Q_INVOKABLE void confirmCommit();
  // Executes the pending confirmed pull.
  Q_INVOKABLE void confirmPull();
  // Executes the pending confirmed push.
  Q_INVOKABLE void confirmPush();

 signals:
  void statusChanged();
  void busyChanged();
  void errorRaised(QString message, bool retryable);
  void confirmRequested(QString action, QString detail);
  void toast(QString message, int level);

 private:
  void setBusy(bool value);
  void handle(Result<void> result);
  GitService* service_;
  ConfigService* config_;
  GitFileModel files_;
  dto::GitStatusDto status_;
  bool busy_ = false;
  QString pendingMessage_;
};
}  // namespace aegis
