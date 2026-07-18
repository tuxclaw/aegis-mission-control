#include "services/process_service.h"

#include <pwd.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
#include <optional>
#include <vector>

#include <QDir>
#include <QFile>
#include <QFutureWatcher>
#include <QHash>
#include <QTextStream>

#include "core/async.h"

namespace aegis {
namespace {

constexpr int kIntervalMs = 5000;
constexpr int kTopProcessCount = 15;
constexpr qsizetype kCommandLimit = 512;

struct CpuSnapshot {
  quint64 ticks = 0;
  quint64 startTicks = 0;
};

struct RawProcess {
  int pid = 0;
  QString name;
  uid_t uid = 0;
  quint64 rssKiB = 0;
  quint64 cpuTicks = 0;
  quint64 startTicks = 0;
  QString command;
};

Result<quint64> readMemTotalKiB() {
  QFile file(QStringLiteral("/proc/meminfo"));
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return tl::unexpected(makeError(ErrorCode::PathNotFound,
                                    QStringLiteral("/proc/meminfo unavailable")));
  }
  QTextStream stream(&file);
  QString line;
  while (stream.readLineInto(&line)) {
    if (!line.startsWith(QStringLiteral("MemTotal:"))) continue;
    bool ok = false;
    const auto value = line.simplified()
                           .section(QLatin1Char(' '), 1, 1)
                           .toULongLong(&ok);
    if (ok && value > 0) return value;
    break;
  }
  return tl::unexpected(makeError(ErrorCode::ResponseMalformed,
                                  QStringLiteral("MemTotal is malformed")));
}

Result<double> readUptimeSeconds() {
  QFile file(QStringLiteral("/proc/uptime"));
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return tl::unexpected(makeError(ErrorCode::PathNotFound,
                                    QStringLiteral("/proc/uptime unavailable")));
  }
  bool ok = false;
  const auto uptime = QString::fromLatin1(file.read(256))
                          .section(QLatin1Char(' '), 0, 0)
                          .toDouble(&ok);
  if (!ok || !std::isfinite(uptime) || uptime < 0.0) {
    return tl::unexpected(makeError(ErrorCode::ResponseMalformed,
                                    QStringLiteral("system uptime is malformed")));
  }
  return uptime;
}

std::optional<RawProcess> readProcess(int pid) {
  const auto base = QStringLiteral("/proc/%1").arg(pid);
  QFile statFile(base + QStringLiteral("/stat"));
  if (!statFile.open(QIODevice::ReadOnly | QIODevice::Text)) return std::nullopt;
  const auto stat = statFile.read(64 * 1024);
  const auto openParen = stat.indexOf('(');
  const auto closeParen = stat.lastIndexOf(')');
  if (openParen <= 0 || closeParen <= openParen + 1) return std::nullopt;
  const auto fields = QString::fromLatin1(stat.mid(closeParen + 1))
                          .split(QLatin1Char(' '), Qt::SkipEmptyParts);
  if (fields.size() <= 19) return std::nullopt;
  bool userTicksOk = false;
  bool systemTicksOk = false;
  bool startTicksOk = false;
  const auto userTicks = fields.at(11).toULongLong(&userTicksOk);
  const auto systemTicks = fields.at(12).toULongLong(&systemTicksOk);
  const auto startTicks = fields.at(19).toULongLong(&startTicksOk);
  if (!userTicksOk || !systemTicksOk || !startTicksOk) return std::nullopt;

  RawProcess process;
  process.pid = pid;
  process.name = QString::fromLocal8Bit(
      stat.mid(openParen + 1, closeParen - openParen - 1));
  process.cpuTicks = userTicks + systemTicks;
  process.startTicks = startTicks;

  QFile statusFile(base + QStringLiteral("/status"));
  if (!statusFile.open(QIODevice::ReadOnly | QIODevice::Text)) return std::nullopt;
  bool uidFound = false;
  QTextStream stream(&statusFile);
  QString line;
  while (stream.readLineInto(&line)) {
    if (line.startsWith(QStringLiteral("Name:"))) {
      const auto name = line.mid(5).trimmed();
      if (!name.isEmpty()) process.name = name;
    } else if (line.startsWith(QStringLiteral("Uid:"))) {
      bool ok = false;
      const auto value = line.mid(4).simplified()
                             .section(QLatin1Char(' '), 0, 0)
                             .toUInt(&ok);
      if (ok) {
        process.uid = static_cast<uid_t>(value);
        uidFound = true;
      }
    } else if (line.startsWith(QStringLiteral("VmRSS:"))) {
      bool ok = false;
      const auto value = line.mid(6).simplified()
                             .section(QLatin1Char(' '), 0, 0)
                             .toULongLong(&ok);
      if (ok) process.rssKiB = value;
    }
  }
  if (!uidFound || process.name.isEmpty()) return std::nullopt;

  QFile commandFile(base + QStringLiteral("/cmdline"));
  if (commandFile.open(QIODevice::ReadOnly)) {
    auto command = commandFile.read(4096);
    command.replace('\0', ' ');
    process.command = QString::fromUtf8(command).simplified();
  }
  if (process.command.isEmpty()) {
    process.command = QStringLiteral("[%1]").arg(process.name);
  }
  if (process.command.size() > kCommandLimit) {
    process.command = process.command.left(kCommandLimit - 1) + QChar(0x2026);
  }
  return process;
}

QString username(uid_t uid) {
  const auto configuredSize = sysconf(_SC_GETPW_R_SIZE_MAX);
  const auto bufferSize = configuredSize > 0
                              ? static_cast<std::size_t>(configuredSize)
                              : static_cast<std::size_t>(16384);
  std::vector<char> buffer(bufferSize);
  passwd entry{};
  passwd* result = nullptr;
  if (getpwuid_r(uid, &entry, buffer.data(), buffer.size(), &result) == 0 &&
      result != nullptr && result->pw_name != nullptr) {
    return QString::fromLocal8Bit(result->pw_name);
  }
  return QString::number(static_cast<qulonglong>(uid));
}

double lifetimeCpuPct(const RawProcess& process, double uptimeSeconds,
                      long clockTicks) {
  const auto startedAt =
      static_cast<double>(process.startTicks) / static_cast<double>(clockTicks);
  const auto lifetime = uptimeSeconds - startedAt;
  if (lifetime <= 0.0) return 0.0;
  return 100.0 * static_cast<double>(process.cpuTicks) /
         static_cast<double>(clockTicks) / lifetime;
}

}  // namespace

struct ProcessService::SamplerState {
  QHash<int, CpuSnapshot> previous;
  std::chrono::steady_clock::time_point previousAt;
  bool hasPrevious = false;
};

ProcessService::ProcessService(QObject* parent)
    : QObject(parent), state_(std::make_shared<SamplerState>()) {
  timer_.setInterval(kIntervalMs);
  timer_.setSingleShot(false);
  connect(&timer_, &QTimer::timeout, this, &ProcessService::sampleNow);
}

void ProcessService::start() {
  timer_.start();
  sampleNow();
}

void ProcessService::stop() { timer_.stop(); }

void ProcessService::refresh() { sampleNow(); }

Result<dto::ProcessDto> ProcessService::collect(
    const std::shared_ptr<SamplerState>& state) {
  const auto memoryTotal = readMemTotalKiB();
  if (!memoryTotal) return tl::unexpected(memoryTotal.error());
  const auto uptime = readUptimeSeconds();
  if (!uptime) return tl::unexpected(uptime.error());
  const auto clockTicks = sysconf(_SC_CLK_TCK);
  if (clockTicks <= 0) {
    return tl::unexpected(makeError(ErrorCode::ResponseMalformed,
                                    QStringLiteral("clock tick rate unavailable")));
  }

  const QDir proc(QStringLiteral("/proc"));
  if (!proc.exists()) {
    return tl::unexpected(makeError(ErrorCode::PathNotFound,
                                    QStringLiteral("/proc is unavailable")));
  }

  QVector<RawProcess> rawProcesses;
  const auto entries =
      proc.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Unsorted);
  rawProcesses.reserve(entries.size());
  for (const auto& entry : entries) {
    bool ok = false;
    const auto pidValue = entry.toLongLong(&ok);
    if (!ok || pidValue <= 0 ||
        pidValue > std::numeric_limits<int>::max()) {
      continue;
    }
    auto process = readProcess(static_cast<int>(pidValue));
    if (process) rawProcesses.append(std::move(process.value()));
  }

  const auto now = std::chrono::steady_clock::now();
  const auto elapsedSeconds = state->hasPrevious
                                  ? std::chrono::duration<double>(
                                        now - state->previousAt)
                                        .count()
                                  : 0.0;
  const auto processorCount = std::max<long>(1, sysconf(_SC_NPROCESSORS_ONLN));
  const auto cpuCap = 100.0 * static_cast<double>(processorCount);
  QHash<int, CpuSnapshot> current;
  current.reserve(rawProcesses.size());
  QVector<dto::ProcessInfo> processes;
  processes.reserve(rawProcesses.size());
  for (const auto& process : rawProcesses) {
    double cpuPct = lifetimeCpuPct(process, uptime.value(), clockTicks);
    const auto previous = state->previous.constFind(process.pid);
    if (elapsedSeconds > 0.0 && previous != state->previous.cend() &&
        previous->startTicks == process.startTicks &&
        process.cpuTicks >= previous->ticks) {
      cpuPct = 100.0 * static_cast<double>(process.cpuTicks - previous->ticks) /
               static_cast<double>(clockTicks) / elapsedSeconds;
    }
    cpuPct = std::clamp(cpuPct, 0.0, cpuCap);
    const auto memPct = std::clamp(
        100.0 * static_cast<double>(process.rssKiB) /
            static_cast<double>(memoryTotal.value()),
        0.0, 100.0);
    processes.append(dto::ProcessInfo{process.pid, process.name,
                                      username(process.uid), cpuPct, memPct,
                                      process.command});
    current.insert(process.pid,
                   CpuSnapshot{process.cpuTicks, process.startTicks});
  }

  state->previous = std::move(current);
  state->previousAt = now;
  state->hasPrevious = true;
  std::sort(processes.begin(), processes.end(),
            [](const dto::ProcessInfo& left, const dto::ProcessInfo& right) {
              if (left.cpuPct != right.cpuPct) return left.cpuPct > right.cpuPct;
              if (left.memPct != right.memPct) return left.memPct > right.memPct;
              return left.pid < right.pid;
            });
  if (processes.size() > kTopProcessCount) {
    processes.resize(kTopProcessCount);
  }
  return dto::ProcessDto{processes, QDateTime::currentDateTimeUtc()};
}

void ProcessService::sampleNow() {
  if (sampling_) return;
  sampling_ = true;
  auto* watcher = new QFutureWatcher<Result<dto::ProcessDto>>(this);
  connect(watcher, &QFutureWatcher<Result<dto::ProcessDto>>::finished, this,
          [this, watcher] {
            sampling_ = false;
            const auto result = watcher->result();
            watcher->deleteLater();
            if (result) {
              emit sampled(result.value());
            } else {
              emit failed(result.error());
            }
          });
  watcher->setFuture(async::run(
      [state = state_] { return ProcessService::collect(state); }));
}

}  // namespace aegis
