#pragma once

#include <QObject>

#include "models/cron_job_model.h"

namespace aegis {
class CronService;

class CronController final : public QObject {
  Q_OBJECT
  Q_PROPERTY(CronJobModel* jobs READ jobs CONSTANT)
  Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)

 public:
  // Creates a cron intent adapter.
  explicit CronController(CronService* service, QObject* parent = nullptr);
  [[nodiscard]] CronJobModel* jobs();
  [[nodiscard]] bool loading() const;
  Q_INVOKABLE void refresh();
  Q_INVOKABLE void toggleJob(QString id, bool enable);
  Q_INVOKABLE void runJob(QString id);

 signals:
  void loadingChanged();
  void errorRaised(QString message, bool retryable);
  void toast(QString message, int level);

 private:
  CronService* service_;
  CronJobModel jobs_;
  bool loading_ = false;
};
}  // namespace aegis
