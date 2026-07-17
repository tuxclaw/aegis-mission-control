#pragma once

#include <QFuture>
#include <QObject>
#include <QVector>

#include "core/result.h"
#include "dto/cron_job_dto.h"

namespace aegis {

class OpenClawCli;

class CronService : public QObject {
  Q_OBJECT

 public:
  // Creates a cron adapter over the structured OpenClaw CLI.
  explicit CronService(OpenClawCli* cli, QObject* parent = nullptr);
  // Lists live cron jobs.
  [[nodiscard]] QFuture<Result<QVector<dto::CronJobDto>>> list();
  // Enables or disables one validated job identifier.
  [[nodiscard]] QFuture<Result<dto::CronJobDto>> toggle(QString jobId,
                                                         bool enable);
  // Starts one validated job immediately.
  [[nodiscard]] QFuture<Result<QString>> runNow(QString jobId);

 private:
  OpenClawCli* cli_;
};

}  // namespace aegis
