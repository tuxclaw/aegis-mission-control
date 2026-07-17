#include "services/openclaw_cli.h"

#include <algorithm>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QProcess>

#include "config/config_service.h"
#include "core/async.h"

namespace aegis {
namespace {

template <typename Dto>
Result<QVector<Dto>> parseArray(const QByteArray& output, const QString& key) {
  QJsonParseError parseError;
  const auto document = QJsonDocument::fromJson(output, &parseError);
  if (parseError.error != QJsonParseError::NoError) {
    return tl::unexpected(makeError(ErrorCode::CliOutputMalformed,
                                    QStringLiteral("CLI JSON parse failed")));
  }
  QJsonArray array;
  if (document.isArray()) {
    array = document.array();
  } else if (document.isObject() && document.object().value(key).isArray()) {
    array = document.object().value(key).toArray();
  } else {
    return tl::unexpected(makeError(ErrorCode::CliOutputMalformed,
                                    QStringLiteral("CLI JSON shape invalid")));
  }
  if (array.size() > 100000) {
    return tl::unexpected(makeError(ErrorCode::CliOutputMalformed,
                                    QStringLiteral("CLI item count exceeded cap")));
  }
  QVector<Dto> result;
  result.reserve(array.size());
  for (const auto& value : array) {
    if (!value.isObject()) {
      return tl::unexpected(makeError(ErrorCode::CliOutputMalformed,
                                      QStringLiteral("CLI item is not object")));
    }
    const auto parsed = Dto::fromJson(value.toObject());
    if (!parsed) {
      return tl::unexpected(makeError(ErrorCode::CliOutputMalformed,
                                      parsed.error().debugContext));
    }
    result.append(parsed.value());
  }
  return result;
}

}  // namespace

OpenClawCli::OpenClawCli(ConfigService* config, QObject* parent)
    : QObject(parent), config_(config) {}

QFuture<Result<OpenClawCli::CliResult>> OpenClawCli::run(
    const QStringList& args, std::chrono::milliseconds timeout,
    quint64 maxOutputBytes) {
  const auto program = config_->openclawBinary();
  if (!program) {
    return async::run([error = program.error()] {
      return Result<CliResult>(tl::unexpected(error));
    });
  }
  return async::run([program = program.value(), args, timeout,
                     maxOutputBytes]() -> Result<CliResult> {
    if (timeout.count() <= 0 || maxOutputBytes == 0 || args.size() > 128) {
      return tl::unexpected(makeError(ErrorCode::ValidationFailed,
                                      QStringLiteral("invalid CLI limits")));
    }
    for (const auto& argument : args) {
      if (argument.contains(QChar::Null) || argument.size() > 8192) {
        return tl::unexpected(makeError(ErrorCode::ValidationFailed,
                                        QStringLiteral("invalid CLI argument")));
      }
    }
    QProcess process;
    process.setProgram(program);
    process.setArguments(args);
    process.setProcessChannelMode(QProcess::SeparateChannels);
    process.start();
    if (!process.waitForStarted(5000)) {
      return tl::unexpected(makeError(ErrorCode::ProcessSpawnFailed,
                                      QStringLiteral("CLI process did not start")));
    }
    const auto deadline = QDeadlineTimer(timeout);
    QByteArray standardOutput;
    QByteArray standardError;
    while (process.state() != QProcess::NotRunning) {
      process.waitForReadyRead(static_cast<int>(
          std::min<qint64>(100, deadline.remainingTime())));
      standardOutput.append(process.readAllStandardOutput());
      standardError.append(process.readAllStandardError());
      const auto total = static_cast<quint64>(standardOutput.size()) +
                         static_cast<quint64>(standardError.size());
      if (total > maxOutputBytes) {
        process.kill();
        process.waitForFinished(1000);
        return tl::unexpected(makeError(ErrorCode::ResponseTooLarge,
                                        QStringLiteral("CLI output cap exceeded")));
      }
      if (deadline.hasExpired()) {
        process.kill();
        process.waitForFinished(1000);
        return tl::unexpected(makeError(ErrorCode::ProcessTimeout,
                                        QStringLiteral("CLI deadline expired")));
      }
    }
    standardOutput.append(process.readAllStandardOutput());
    standardError.append(process.readAllStandardError());
    if (static_cast<quint64>(standardOutput.size() + standardError.size()) >
        maxOutputBytes) {
      return tl::unexpected(makeError(ErrorCode::ResponseTooLarge,
                                      QStringLiteral("CLI output cap exceeded")));
    }
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
      return tl::unexpected(makeError(ErrorCode::ProcessNonZeroExit,
                                      QStringLiteral("CLI returned non-zero")));
    }
    return CliResult{process.exitCode(), standardOutput, standardError};
  });
}

QFuture<Result<QVector<dto::AgentDto>>> OpenClawCli::listAgents() {
  const auto timeout = config_->openclawCliTimeoutMs();
  const auto cap = config_->openclawCliOutputCap();
  if (!timeout || !cap) {
    const auto error = !timeout ? timeout.error() : cap.error();
    return async::run([error] {
      return Result<QVector<dto::AgentDto>>(tl::unexpected(error));
    });
  }
  return run({QStringLiteral("agents"), QStringLiteral("list"),
              QStringLiteral("--json")},
             std::chrono::milliseconds(timeout.value()), cap.value())
      .then(this, [](const Result<CliResult>& result) {
        return result ? parseAgents(result->stdoutData)
                      : Result<QVector<dto::AgentDto>>(
                            tl::unexpected(result.error()));
      });
}

QFuture<Result<QVector<dto::CronJobDto>>> OpenClawCli::listCron() {
  const auto timeout = config_->openclawCliTimeoutMs();
  const auto cap = config_->openclawCliOutputCap();
  if (!timeout || !cap) {
    const auto error = !timeout ? timeout.error() : cap.error();
    return async::run([error] {
      return Result<QVector<dto::CronJobDto>>(tl::unexpected(error));
    });
  }
  return run({QStringLiteral("cron"), QStringLiteral("list"),
              QStringLiteral("--json")},
             std::chrono::milliseconds(timeout.value()), cap.value())
      .then(this, [](const Result<CliResult>& result) {
        return result ? parseCron(result->stdoutData)
                      : Result<QVector<dto::CronJobDto>>(
                            tl::unexpected(result.error()));
      });
}

QFuture<Result<QVector<dto::ModelDto>>> OpenClawCli::listModels() {
  const auto timeout = config_->openclawCliTimeoutMs();
  const auto cap = config_->openclawCliOutputCap();
  if (!timeout || !cap) {
    const auto error = !timeout ? timeout.error() : cap.error();
    return async::run([error] {
      return Result<QVector<dto::ModelDto>>(tl::unexpected(error));
    });
  }
  return run({QStringLiteral("models"), QStringLiteral("list"),
              QStringLiteral("--json")},
             std::chrono::milliseconds(timeout.value()), cap.value())
      .then(this, [](const Result<CliResult>& result) {
        return result ? parseModels(result->stdoutData)
                      : Result<QVector<dto::ModelDto>>(
                            tl::unexpected(result.error()));
      });
}

Result<QVector<dto::AgentDto>> OpenClawCli::parseAgents(
    const QByteArray& output) {
  return parseArray<dto::AgentDto>(output, QStringLiteral("agents"));
}

Result<QVector<dto::CronJobDto>> OpenClawCli::parseCron(
    const QByteArray& output) {
  return parseArray<dto::CronJobDto>(output, QStringLiteral("jobs"));
}

Result<QVector<dto::ModelDto>> OpenClawCli::parseModels(
    const QByteArray& output) {
  return parseArray<dto::ModelDto>(output, QStringLiteral("models"));
}

}  // namespace aegis
