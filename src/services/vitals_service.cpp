#include "services/vitals_service.h"

#include <sys/statvfs.h>

#include <cmath>

#include <QDir>
#include <QFile>
#include <QFutureWatcher>
#include <QMutex>
#include <QMutexLocker>
#include <QRegularExpression>
#include <QTextStream>

#include "config/config_service.h"
#include "core/async.h"

namespace aegis {
namespace {

struct CpuCounter {
  quint64 total = 0;
  quint64 idle = 0;
};

Result<QVector<CpuCounter>> readCpuCounters() {
  QFile file(QStringLiteral("/proc/stat"));
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return tl::unexpected(makeError(ErrorCode::PathNotFound,
                                    QStringLiteral("CPU statistics unavailable")));
  }
  QVector<CpuCounter> counters;
  QTextStream stream(&file);
  while (!stream.atEnd()) {
    const auto line = stream.readLine();
    if (!line.startsWith(QStringLiteral("cpu"))) break;
    const auto fields = line.simplified().split(QLatin1Char(' '));
    if (fields.size() < 8) {
      return tl::unexpected(makeError(ErrorCode::ResponseMalformed,
                                      QStringLiteral("CPU statistics malformed")));
    }
    CpuCounter counter;
    for (int index = 1; index < fields.size(); ++index) {
      bool ok = false;
      const auto value = fields.at(index).toULongLong(&ok);
      if (!ok) {
        return tl::unexpected(makeError(ErrorCode::ResponseMalformed,
                                        QStringLiteral("CPU counter malformed")));
      }
      counter.total += value;
      if (index == 4 || index == 5) counter.idle += value;
    }
    counters.append(counter);
  }
  if (counters.isEmpty()) {
    return tl::unexpected(makeError(ErrorCode::ResponseMalformed,
                                    QStringLiteral("CPU statistics empty")));
  }
  return counters;
}

double utilization(const CpuCounter& previous, const CpuCounter& current) {
  if (current.total <= previous.total || current.idle < previous.idle) return 0.0;
  const auto totalDelta = current.total - previous.total;
  const auto idleDelta = current.idle - previous.idle;
  return std::clamp(100.0 * static_cast<double>(totalDelta - idleDelta) /
                        static_cast<double>(totalDelta),
                    0.0, 100.0);
}

double readNumber(const QString& path, double scale = 1.0) {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return qQNaN();
  bool ok = false;
  const auto value = QString::fromUtf8(file.readAll()).trimmed().toDouble(&ok);
  return ok ? value / scale : qQNaN();
}

dto::GpuVitals readGpu() {
  const QDir drm(QStringLiteral("/sys/class/drm"));
  const auto cards = drm.entryList({QStringLiteral("card[0-9]*")}, QDir::Dirs);
  for (const auto& card : cards) {
    const auto base = drm.filePath(card + QStringLiteral("/device"));
    const auto busyPath = QDir(base).filePath(QStringLiteral("gpu_busy_percent"));
    if (QFileInfo::exists(busyPath)) {
      return {true, QStringLiteral("amd"), readNumber(busyPath), 0.0, 0.0,
              readNumber(QDir(base).filePath(
                             QStringLiteral("hwmon/hwmon0/temp1_input")),
                         1000.0)};
    }
  }
  return {};
}

Result<dto::MemVitals> readMemory() {
  QFile file(QStringLiteral("/proc/meminfo"));
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return tl::unexpected(makeError(ErrorCode::PathNotFound,
                                    QStringLiteral("memory statistics unavailable")));
  }
  QHash<QString, quint64> values;
  QTextStream stream(&file);
  while (!stream.atEnd()) {
    const auto fields = stream.readLine().simplified().split(QLatin1Char(' '));
    if (fields.size() >= 2) {
      bool ok = false;
      const auto value = fields.at(1).toULongLong(&ok);
      if (ok) values.insert(fields.at(0).chopped(1), value * 1024ULL);
    }
  }
  const auto total = values.value(QStringLiteral("MemTotal"));
  const auto available = values.value(QStringLiteral("MemAvailable"));
  const auto swapTotal = values.value(QStringLiteral("SwapTotal"));
  const auto swapFree = values.value(QStringLiteral("SwapFree"));
  if (total == 0 || available > total || swapFree > swapTotal) {
    return tl::unexpected(makeError(ErrorCode::ResponseMalformed,
                                    QStringLiteral("memory statistics malformed")));
  }
  return dto::MemVitals{total, total - available, available, swapTotal,
                        swapTotal - swapFree};
}

Result<QHash<QString, QPair<quint64, quint64>>> readNetworkCounters() {
  QFile file(QStringLiteral("/proc/net/dev"));
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return tl::unexpected(makeError(ErrorCode::PathNotFound,
                                    QStringLiteral("network statistics unavailable")));
  }
  QHash<QString, QPair<quint64, quint64>> result;
  QTextStream stream(&file);
  stream.readLine();
  stream.readLine();
  while (!stream.atEnd()) {
    const auto line = stream.readLine();
    const auto colon = line.indexOf(QLatin1Char(':'));
    if (colon <= 0) continue;
    const auto name = line.left(colon).trimmed();
    const auto fields = line.mid(colon + 1).simplified().split(QLatin1Char(' '));
    if (fields.size() < 16 || name == QStringLiteral("lo")) continue;
    bool rxOk = false;
    bool txOk = false;
    const auto rx = fields.at(0).toULongLong(&rxOk);
    const auto tx = fields.at(8).toULongLong(&txOk);
    if (rxOk && txOk) result.insert(name, {rx, tx});
  }
  return result;
}

}  // namespace

struct VitalsService::SamplerState {
  QMutex mutex;
  QVector<CpuCounter> previousCpu;
  QHash<QString, QPair<quint64, quint64>> previousNetwork;
  QDateTime previousAt;
};

VitalsService::VitalsService(ConfigService* config, QObject* parent)
    : QObject(parent), config_(config), state_(std::make_shared<SamplerState>()) {
  timer_.setSingleShot(false);
  connect(&timer_, &QTimer::timeout, this, &VitalsService::sampleNow);
}

void VitalsService::start(std::chrono::milliseconds interval) {
  auto milliseconds = interval.count();
  if (milliseconds <= 0) {
    const auto configured = config_->vitalsIntervalMs();
    if (!configured) {
      emit failed(configured.error());
      return;
    }
    milliseconds = configured.value();
  }
  milliseconds = std::clamp<qint64>(milliseconds, 250, 10000);
  timer_.start(static_cast<int>(milliseconds));
  sampleNow();
}

void VitalsService::stop() { timer_.stop(); }

void VitalsService::sampleNow() {
  if (sampling_) return;
  sampling_ = true;
  auto future = async::run([state = state_]() -> Result<dto::VitalsDto> {
    const auto cpu = readCpuCounters();
    if (!cpu) return tl::unexpected(cpu.error());
    const auto memory = readMemory();
    if (!memory) return tl::unexpected(memory.error());
    const auto network = readNetworkCounters();
    if (!network) return tl::unexpected(network.error());
    const auto now = QDateTime::currentDateTimeUtc();
    dto::VitalsDto sample;
    sample.sampledAt = now;
    sample.mem = memory.value();
    sample.gpu = readGpu();
    struct statvfs diskInfo {};
    if (statvfs("/", &diskInfo) == 0) {
      const auto total = static_cast<quint64>(diskInfo.f_blocks) *
                         static_cast<quint64>(diskInfo.f_frsize);
      const auto available = static_cast<quint64>(diskInfo.f_bavail) *
                             static_cast<quint64>(diskInfo.f_frsize);
      sample.disks.append({QStringLiteral("/"), total, total - available});
    }
    QFile loadFile(QStringLiteral("/proc/loadavg"));
    if (loadFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
      bool ok = false;
      sample.cpu.loadAvg1 = QString::fromUtf8(loadFile.readLine())
                                .section(QLatin1Char(' '), 0, 0)
                                .toDouble(&ok);
      if (!ok) sample.cpu.loadAvg1 = 0.0;
    }
    sample.cpu.tempC = readNumber(
        QStringLiteral("/sys/class/thermal/thermal_zone0/temp"), 1000.0);
    sample.cpu.coreCount = static_cast<int>(cpu->size() - 1);
    QMutexLocker locker(&state->mutex);
    if (state->previousCpu.size() == cpu->size()) {
      sample.cpu.utilizationPct = utilization(state->previousCpu.first(),
                                              cpu->first());
      for (int index = 1; index < cpu->size(); ++index) {
        sample.cpu.perCorePct.append(
            utilization(state->previousCpu.at(index), cpu->at(index)));
      }
    } else {
      sample.cpu.perCorePct.fill(0.0, sample.cpu.coreCount);
    }
    const auto elapsedMs = state->previousAt.isValid()
                               ? state->previousAt.msecsTo(now)
                               : 0;
    for (auto iterator = network->constBegin(); iterator != network->constEnd();
         ++iterator) {
      quint64 rxRate = 0;
      quint64 txRate = 0;
      if (elapsedMs > 0 && state->previousNetwork.contains(iterator.key())) {
        const auto previous = state->previousNetwork.value(iterator.key());
        if (iterator.value().first >= previous.first) {
          rxRate = (iterator.value().first - previous.first) * 1000ULL /
                   static_cast<quint64>(elapsedMs);
        }
        if (iterator.value().second >= previous.second) {
          txRate = (iterator.value().second - previous.second) * 1000ULL /
                   static_cast<quint64>(elapsedMs);
        }
      }
      sample.nets.append({iterator.key(), rxRate, txRate});
    }
    state->previousCpu = cpu.value();
    state->previousNetwork = network.value();
    state->previousAt = now;
    return sample;
  });
  auto* watcher = new QFutureWatcher<Result<dto::VitalsDto>>(this);
  connect(watcher, &QFutureWatcher<Result<dto::VitalsDto>>::finished, this,
          [this, watcher] {
            sampling_ = false;
            const auto result = watcher->result();
            watcher->deleteLater();
            if (result) emit sampled(result.value());
            else emit failed(result.error());
          });
  watcher->setFuture(std::move(future));
}

}  // namespace aegis
