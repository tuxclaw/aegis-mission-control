#pragma once

#include <chrono>

#include <QFuture>
#include <QObject>
#include <QStringList>
#include <QVector>

#include "core/result.h"
#include "dto/agent_dto.h"
#include "dto/cron_job_dto.h"
#include "dto/model_dto.h"

namespace aegis {

class ConfigService;

class OpenClawCli : public QObject {
  Q_OBJECT

 public:
  struct CliResult {
    int exitCode = -1;
    QByteArray stdoutData;
    QByteArray stderrData;
  };

  // Creates an asynchronous wrapper around the configured OpenClaw binary.
  explicit OpenClawCli(ConfigService* config, QObject* parent = nullptr);
  // Runs an argument array without a shell, with deadline and byte cap.
  [[nodiscard]] QFuture<Result<CliResult>> run(
      const QStringList& args,
      std::chrono::milliseconds timeout = std::chrono::milliseconds(20000),
      quint64 maxOutputBytes = 4ULL * 1024ULL * 1024ULL);
  // Fetches and validates the live agent roster.
  [[nodiscard]] QFuture<Result<QVector<dto::AgentDto>>> listAgents();
  // Fetches and validates live cron jobs.
  [[nodiscard]] QFuture<Result<QVector<dto::CronJobDto>>> listCron();
  // Fetches and validates the live model inventory.
  [[nodiscard]] QFuture<Result<QVector<dto::ModelDto>>> listModels();

  // Purely parses structured agent JSON output.
  static Result<QVector<dto::AgentDto>> parseAgents(const QByteArray& output);
  // Purely parses structured cron JSON output.
  static Result<QVector<dto::CronJobDto>> parseCron(const QByteArray& output);
  // Purely parses structured model JSON output.
  static Result<QVector<dto::ModelDto>> parseModels(const QByteArray& output);

 private:
  ConfigService* config_;
};

}  // namespace aegis
