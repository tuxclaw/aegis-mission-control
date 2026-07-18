#include "services/openclaw_cli.h"

#include <algorithm>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QHash>
#include <QProcess>
#include <QTimeZone>

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

bool isActiveSession(const QString& status) {
  return status == QStringLiteral("running") ||
         status == QStringLiteral("active");
}

bool isFailedSession(const QString& status) {
  return status == QStringLiteral("failed") ||
         status == QStringLiteral("error") ||
         status == QStringLiteral("timed_out") ||
         status == QStringLiteral("lost");
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
  return async::flatten(
      this,
      run({QStringLiteral("agents"), QStringLiteral("list"),
           QStringLiteral("--json")},
          std::chrono::milliseconds(timeout.value()), cap.value())
          .then(this, [this, timeout = timeout.value(), cap = cap.value()](
                          const Result<CliResult>& result)
                          -> QFuture<Result<QVector<dto::AgentDto>>> {
            if (!result) {
              return async::run([error = result.error()] {
                return Result<QVector<dto::AgentDto>>(tl::unexpected(error));
              });
            }
            auto agents = parseAgents(result->stdoutData);
            if (!agents) {
              return async::run([error = agents.error()] {
                return Result<QVector<dto::AgentDto>>(tl::unexpected(error));
              });
            }
            return run({QStringLiteral("sessions"),
                        QStringLiteral("--all-agents"),
                        QStringLiteral("--limit"), QStringLiteral("all"),
                        QStringLiteral("--json")},
                       std::chrono::milliseconds(timeout), cap)
                .then(this, [agents = std::move(agents.value())](
                                const Result<CliResult>& sessions) mutable {
                  return sessions
                             ? applyAgentSessions(
                                   std::move(agents), sessions->stdoutData)
                             : Result<QVector<dto::AgentDto>>(
                                   tl::unexpected(sessions.error()));
                });
          }));
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

Result<QVector<dto::AgentDto>> OpenClawCli::applyAgentSessions(
    QVector<dto::AgentDto> agents, const QByteArray& output) {
  QJsonParseError parseError;
  const auto document = QJsonDocument::fromJson(output, &parseError);
  if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
    return tl::unexpected(makeError(ErrorCode::CliOutputMalformed,
                                    QStringLiteral("session JSON parse failed")));
  }
  const auto sessionsValue =
      document.object().value(QStringLiteral("sessions"));
  if (!sessionsValue.isArray()) {
    return tl::unexpected(makeError(ErrorCode::CliOutputMalformed,
                                    QStringLiteral("session JSON shape invalid")));
  }
  const auto sessions = sessionsValue.toArray();
  if (sessions.size() > 100000) {
    return tl::unexpected(makeError(ErrorCode::CliOutputMalformed,
                                    QStringLiteral("session count exceeded cap")));
  }

  struct SessionSummary {
    int activeCount = 0;
    qint64 latestAt = 0;
    QString latestStatus;
  };
  QHash<QString, SessionSummary> summaries;
  for (const auto& sessionValue : sessions) {
    if (!sessionValue.isObject()) {
      return tl::unexpected(makeError(ErrorCode::CliOutputMalformed,
                                      QStringLiteral("session is not object")));
    }
    const auto session = sessionValue.toObject();
    const auto agentId = session.value(QStringLiteral("agentId"));
    const auto updatedAt = session.value(QStringLiteral("updatedAt"));
    if (!agentId.isString() || agentId.toString().isEmpty() ||
        !updatedAt.isDouble() || updatedAt.toDouble() < 0.0) {
      return tl::unexpected(makeError(ErrorCode::CliOutputMalformed,
                                      QStringLiteral("session identity invalid")));
    }
    const auto status = session.value(QStringLiteral("status"))
                            .toString()
                            .trimmed()
                            .toLower();
    auto& summary = summaries[agentId.toString()];
    if (isActiveSession(status)) ++summary.activeCount;
    const auto timestamp = static_cast<qint64>(updatedAt.toDouble());
    if (timestamp >= summary.latestAt) {
      summary.latestAt = timestamp;
      summary.latestStatus = status;
    }
  }

  for (auto& agent : agents) {
    const auto summary = summaries.constFind(agent.id);
    if (summary == summaries.cend()) continue;
    agent.activeSessions = summary->activeCount;
    agent.lastSeen =
        QDateTime::fromMSecsSinceEpoch(summary->latestAt, QTimeZone::UTC);
    agent.statusDetail.clear();
    if (summary->activeCount > 0) {
      agent.status = dto::AgentStatus::Active;
    } else if (isFailedSession(summary->latestStatus)) {
      agent.status = dto::AgentStatus::Error;
      agent.statusDetail = QStringLiteral("Latest session failed");
    } else {
      agent.status = dto::AgentStatus::Idle;
    }
  }
  return agents;
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
