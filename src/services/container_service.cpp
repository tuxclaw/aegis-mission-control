#include "services/container_service.h"

#include <algorithm>

#include <QFutureWatcher>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonValue>
#include <QProcess>
#include <QStandardPaths>
#include <QTimeZone>

#include "core/async.h"

namespace aegis {
namespace {

constexpr int kIntervalMs = 10000;
constexpr int kStartTimeoutMs = 5000;
constexpr int kFinishTimeoutMs = 10000;
constexpr qsizetype kOutputCap = 8 * 1024 * 1024;
constexpr qsizetype kContainerCap = 10000;

dto::ContainerDto emptySample() {
  return {{}, QDateTime::currentDateTimeUtc()};
}

QString containerName(const QJsonObject& object) {
  const auto names = object.value(QStringLiteral("Names"));
  if (names.isArray() && !names.toArray().isEmpty()) {
    return names.toArray().first().toString().trimmed();
  }
  return names.toString().trimmed();
}

QString containerStatus(const QJsonObject& object) {
  auto status = object.value(QStringLiteral("State")).toString().trimmed().toLower();
  if (!status.isEmpty()) return status;

  const auto description =
      object.value(QStringLiteral("Status")).toString().trimmed().toLower();
  if (description.startsWith(QStringLiteral("up"))) {
    return QStringLiteral("running");
  }
  if (description.startsWith(QStringLiteral("exited"))) {
    return QStringLiteral("exited");
  }
  if (description.startsWith(QStringLiteral("paused"))) {
    return QStringLiteral("paused");
  }
  if (description.startsWith(QStringLiteral("created"))) {
    return QStringLiteral("created");
  }
  return QStringLiteral("unknown");
}

QString portNumber(int start, int range) {
  if (range <= 1) return QString::number(start);
  return QStringLiteral("%1-%2").arg(start).arg(start + range - 1);
}

QString formatPorts(const QJsonValue& value) {
  if (value.isString()) return value.toString().trimmed();
  if (!value.isArray()) return {};

  QStringList mappings;
  for (const auto& item : value.toArray()) {
    if (!item.isObject()) continue;
    const auto port = item.toObject();
    const auto containerPort =
        port.value(QStringLiteral("container_port")).toInt();
    const auto hostPort = port.value(QStringLiteral("host_port")).toInt();
    const auto range = std::max(1, port.value(QStringLiteral("range")).toInt(1));
    const auto protocol =
        port.value(QStringLiteral("protocol")).toString(QStringLiteral("tcp"));
    if (containerPort <= 0) continue;

    const auto containerText = portNumber(containerPort, range);
    if (hostPort <= 0) {
      mappings.append(QStringLiteral("%1/%2").arg(containerText, protocol));
      continue;
    }

    const auto hostText = portNumber(hostPort, range);
    const auto hostIp = port.value(QStringLiteral("host_ip")).toString().trimmed();
    const auto endpoint = hostIp.isEmpty()
                              ? hostText
                              : QStringLiteral("%1:%2").arg(hostIp, hostText);
    mappings.append(QStringLiteral("%1->%2/%3")
                        .arg(endpoint, containerText, protocol));
  }
  return mappings.join(QStringLiteral(", "));
}

QDateTime createdAt(const QJsonObject& object) {
  const auto created = object.value(QStringLiteral("Created"));
  if (created.isDouble()) {
    const auto seconds = created.toDouble();
    if (seconds >= 0.0 && seconds <= 253402300799.0) {
      return QDateTime::fromSecsSinceEpoch(static_cast<qint64>(seconds),
                                           QTimeZone::UTC);
    }
  }

  for (const auto& key : {QStringLiteral("Created"),
                          QStringLiteral("CreatedAt")}) {
    const auto value = object.value(key);
    if (!value.isString()) continue;
    auto parsed = QDateTime::fromString(value.toString(), Qt::ISODateWithMs);
    if (!parsed.isValid()) {
      parsed = QDateTime::fromString(value.toString(), Qt::ISODate);
    }
    if (parsed.isValid() && parsed.timeSpec() != Qt::LocalTime) {
      return parsed.toUTC();
    }
  }
  return {};
}

Result<dto::ContainerDto> parsePodmanOutput(const QByteArray& output) {
  QJsonParseError parseError;
  const auto document = QJsonDocument::fromJson(output, &parseError);
  if (parseError.error != QJsonParseError::NoError || !document.isArray()) {
    return tl::unexpected(makeError(ErrorCode::CliOutputMalformed,
                                    QStringLiteral("Podman JSON is malformed")));
  }
  const auto source = document.array();
  if (source.size() > kContainerCap) {
    return tl::unexpected(makeError(
        ErrorCode::ResponseTooLarge,
        QStringLiteral("Podman container count exceeds cap")));
  }

  QJsonArray containers;
  for (const auto& value : source) {
    if (!value.isObject()) {
      return tl::unexpected(makeError(
          ErrorCode::CliOutputMalformed,
          QStringLiteral("Podman container entry is not an object")));
    }
    const auto object = value.toObject();
    const auto fullId = object.value(QStringLiteral("Id")).toString().trimmed();
    const auto name = containerName(object);
    const auto image =
        object.value(QStringLiteral("Image")).toString().trimmed();
    const auto created = createdAt(object);
    if (fullId.size() < 12 || name.isEmpty() || image.isEmpty() ||
        !created.isValid()) {
      return tl::unexpected(makeError(
          ErrorCode::CliOutputMalformed,
          QStringLiteral("Podman container fields are incomplete")));
    }

    containers.append(QJsonObject{
        {QStringLiteral("id"), fullId.left(12)},
        {QStringLiteral("name"), name},
        {QStringLiteral("image"), image},
        {QStringLiteral("status"), containerStatus(object)},
        {QStringLiteral("ports"),
         formatPorts(object.value(QStringLiteral("Ports")))},
        {QStringLiteral("created"), created.toString(Qt::ISODateWithMs)}});
  }

  const auto normalized = QJsonObject{
      {QStringLiteral("containers"), containers},
      {QStringLiteral("sampledAt"),
       QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)}};
  const auto parsed = dto::ContainerDto::fromJson(normalized);
  if (!parsed) {
    return tl::unexpected(makeError(ErrorCode::CliOutputMalformed,
                                    parsed.error().debugContext));
  }
  return parsed.value();
}

Result<dto::ContainerDto> queryPodman() {
  const auto executable = QStandardPaths::findExecutable(QStringLiteral("podman"));
  if (executable.isEmpty()) return emptySample();

  QProcess process;
  process.setProgram(executable);
  process.setArguments({QStringLiteral("ps"), QStringLiteral("--all"),
                        QStringLiteral("--format"), QStringLiteral("json")});
  process.setProcessChannelMode(QProcess::SeparateChannels);
  process.start();
  if (!process.waitForStarted(kStartTimeoutMs)) return emptySample();
  if (!process.waitForFinished(kFinishTimeoutMs)) {
    process.kill();
    process.waitForFinished(1000);
    return emptySample();
  }
  const auto output = process.readAllStandardOutput();
  if (output.size() > kOutputCap) {
    return tl::unexpected(makeError(ErrorCode::ResponseTooLarge,
                                    QStringLiteral("Podman output exceeds cap")));
  }
  if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
    return emptySample();
  }
  return parsePodmanOutput(output);
}

}  // namespace

ContainerService::ContainerService(QObject* parent) : QObject(parent) {
  timer_.setInterval(kIntervalMs);
  timer_.setSingleShot(false);
  connect(&timer_, &QTimer::timeout, this, &ContainerService::sampleNow);
}

void ContainerService::start() {
  timer_.start();
  sampleNow();
}

void ContainerService::stop() { timer_.stop(); }

void ContainerService::refresh() { sampleNow(); }

void ContainerService::sampleNow() {
  if (sampling_) return;
  sampling_ = true;
  auto* watcher = new QFutureWatcher<Result<dto::ContainerDto>>(this);
  connect(watcher, &QFutureWatcher<Result<dto::ContainerDto>>::finished, this,
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
  watcher->setFuture(async::run(queryPodman));
}

}  // namespace aegis
