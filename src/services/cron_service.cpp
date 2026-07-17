#include "services/cron_service.h"

#include <QRegularExpression>

#include "core/async.h"
#include "services/openclaw_cli.h"

namespace aegis {
namespace {

Result<void> validateJobId(const QString& id) {
  static const QRegularExpression pattern(
      QStringLiteral(R"(^[A-Za-z0-9][A-Za-z0-9_.:-]{0,255}$)"));
  if (!pattern.match(id).hasMatch()) {
    return tl::unexpected(makeError(ErrorCode::ValidationFailed,
                                    QStringLiteral("invalid cron job id")));
  }
  return {};
}

}  // namespace

CronService::CronService(OpenClawCli* cli, QObject* parent)
    : QObject(parent), cli_(cli) {}

QFuture<Result<QVector<dto::CronJobDto>>> CronService::list() {
  return cli_->listCron();
}

QFuture<Result<dto::CronJobDto>> CronService::toggle(QString jobId,
                                                      bool enable) {
  const auto valid = validateJobId(jobId);
  if (!valid) {
    return async::run([error = valid.error()] {
      return Result<dto::CronJobDto>(tl::unexpected(error));
    });
  }
  return cli_->run({QStringLiteral("cron"),
                    enable ? QStringLiteral("enable")
                           : QStringLiteral("disable"),
                    jobId, QStringLiteral("--json")})
      .then(this, [jobId](const Result<OpenClawCli::CliResult>& result) {
        if (!result) {
          return Result<dto::CronJobDto>(tl::unexpected(result.error()));
        }
        const auto parsed = OpenClawCli::parseCron(result->stdoutData);
        if (!parsed || parsed->size() != 1 || parsed->first().id != jobId) {
          return Result<dto::CronJobDto>(tl::unexpected(makeError(
              ErrorCode::CliOutputMalformed,
              QStringLiteral("cron toggle output invalid"))));
        }
        return Result<dto::CronJobDto>(parsed->first());
      });
}

QFuture<Result<QString>> CronService::runNow(QString jobId) {
  const auto valid = validateJobId(jobId);
  if (!valid) {
    return async::run([error = valid.error()] {
      return Result<QString>(tl::unexpected(error));
    });
  }
  return cli_->run({QStringLiteral("cron"), QStringLiteral("run"), jobId,
                    QStringLiteral("--json")})
      .then(this, [jobId](const Result<OpenClawCli::CliResult>& result) {
        if (!result) return Result<QString>(tl::unexpected(result.error()));
        const auto output = QString::fromUtf8(result->stdoutData).trimmed();
        return Result<QString>(output.isEmpty() ? jobId : output.left(1024));
      });
}

}  // namespace aegis
