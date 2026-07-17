#include "dto/vitals_dto.h"

#include <cmath>
#include <limits>

#include <QDir>
#include <QJsonArray>
#include <QRegularExpression>
#include <QSet>

#include "dto/json_validation.h"

namespace aegis::dto {
namespace {

constexpr auto kMaxExactJsonInteger = 9007199254740991ULL;

QJsonValue nullableJsonNumber(double value) {
  return std::isfinite(value) ? QJsonValue(value)
                              : QJsonValue(QJsonValue::Null);
}

Result<CpuVitals> parseCpu(const QJsonObject& object) {
  const auto unknown = json::rejectUnknown(
      object, {QStringLiteral("utilizationPct"), QStringLiteral("coreCount"),
               QStringLiteral("perCorePct"), QStringLiteral("loadAvg1"),
               QStringLiteral("tempC")});
  if (!unknown) return tl::unexpected(unknown.error());
  const auto utilization = json::requiredNumber(
      object, QStringLiteral("utilizationPct"), 0.0, 100.0);
  if (!utilization) return tl::unexpected(utilization.error());
  const auto coreCount = json::requiredInt(
      object, QStringLiteral("coreCount"), 0, 4096);
  if (!coreCount) return tl::unexpected(coreCount.error());
  if (!object.value(QStringLiteral("perCorePct")).isArray()) {
    return tl::unexpected(makeError(ErrorCode::ValidationFailed,
                                    QStringLiteral("perCorePct must be an array")));
  }
  const auto array = object.value(QStringLiteral("perCorePct")).toArray();
  if (array.size() > 4096 ||
      (!array.isEmpty() && array.size() != coreCount.value())) {
    return tl::unexpected(makeError(ErrorCode::ValidationFailed,
                                    QStringLiteral("invalid per-core sample count")));
  }
  QVector<double> perCore;
  perCore.reserve(array.size());
  for (const auto& value : array) {
    if (!value.isDouble() || !std::isfinite(value.toDouble()) ||
        value.toDouble() < 0.0 || value.toDouble() > 100.0) {
      return tl::unexpected(makeError(ErrorCode::ValidationFailed,
                                      QStringLiteral("invalid per-core utilization")));
    }
    perCore.append(value.toDouble());
  }
  const auto load = json::requiredNumber(
      object, QStringLiteral("loadAvg1"), 0.0, 1000000.0);
  if (!load) return tl::unexpected(load.error());
  const auto temp = json::nullableNumber(
      object, QStringLiteral("tempC"), -273.15, 1000.0);
  if (!temp) return tl::unexpected(temp.error());
  return CpuVitals{utilization.value(), coreCount.value(), perCore,
                   load.value(), temp.value()};
}

Result<GpuVitals> parseGpu(const QJsonObject& object) {
  const auto unknown = json::rejectUnknown(
      object, {QStringLiteral("available"), QStringLiteral("vendor"),
               QStringLiteral("utilizationPct"), QStringLiteral("memUsedBytes"),
               QStringLiteral("memTotalBytes"), QStringLiteral("tempC")});
  if (!unknown) return tl::unexpected(unknown.error());
  const auto available = json::requiredBool(object, QStringLiteral("available"));
  if (!available) return tl::unexpected(available.error());
  const auto vendor = json::optionalString(
      object, QStringLiteral("vendor"), 16, true);
  if (!vendor) return tl::unexpected(vendor.error());
  static const QSet<QString> kVendors = {
      QString(), QStringLiteral("nvidia"), QStringLiteral("amd"),
      QStringLiteral("intel")};
  if (!kVendors.contains(vendor.value())) {
    return tl::unexpected(makeError(ErrorCode::ValidationFailed,
                                    QStringLiteral("unknown GPU vendor")));
  }
  const auto utilization = json::nullableNumber(
      object, QStringLiteral("utilizationPct"), 0.0, 100.0);
  if (!utilization) return tl::unexpected(utilization.error());
  const auto used = json::requiredNumber(
      object, QStringLiteral("memUsedBytes"), 0.0,
      static_cast<double>(kMaxExactJsonInteger));
  if (!used) return tl::unexpected(used.error());
  const auto total = json::requiredNumber(
      object, QStringLiteral("memTotalBytes"), 0.0,
      static_cast<double>(kMaxExactJsonInteger));
  if (!total) return tl::unexpected(total.error());
  if (used.value() > total.value()) {
    return tl::unexpected(makeError(ErrorCode::ValidationFailed,
                                    QStringLiteral("GPU memory used exceeds total")));
  }
  const auto temp = json::nullableNumber(
      object, QStringLiteral("tempC"), -273.15, 1000.0);
  if (!temp) return tl::unexpected(temp.error());
  if (!available.value() && std::isfinite(utilization.value())) {
    return tl::unexpected(makeError(ErrorCode::ValidationFailed,
                                    QStringLiteral("unavailable GPU has utilization")));
  }
  return GpuVitals{available.value(), vendor.value(), utilization.value(),
                   used.value(), total.value(), temp.value()};
}

Result<MemVitals> parseMemory(const QJsonObject& object) {
  const auto unknown = json::rejectUnknown(
      object, {QStringLiteral("totalBytes"), QStringLiteral("usedBytes"),
               QStringLiteral("availableBytes"),
               QStringLiteral("swapTotalBytes"),
               QStringLiteral("swapUsedBytes")});
  if (!unknown) return tl::unexpected(unknown.error());
  const auto total = json::requiredUnsigned(
      object, QStringLiteral("totalBytes"), kMaxExactJsonInteger);
  if (!total) return tl::unexpected(total.error());
  const auto used = json::requiredUnsigned(
      object, QStringLiteral("usedBytes"), kMaxExactJsonInteger);
  if (!used) return tl::unexpected(used.error());
  const auto available = json::requiredUnsigned(
      object, QStringLiteral("availableBytes"), kMaxExactJsonInteger);
  if (!available) return tl::unexpected(available.error());
  const auto swapTotal = json::requiredUnsigned(
      object, QStringLiteral("swapTotalBytes"), kMaxExactJsonInteger);
  if (!swapTotal) return tl::unexpected(swapTotal.error());
  const auto swapUsed = json::requiredUnsigned(
      object, QStringLiteral("swapUsedBytes"), kMaxExactJsonInteger);
  if (!swapUsed) return tl::unexpected(swapUsed.error());
  if (used.value() > total.value() || available.value() > total.value() ||
      swapUsed.value() > swapTotal.value()) {
    return tl::unexpected(makeError(ErrorCode::ValidationFailed,
                                    QStringLiteral("memory totals are inconsistent")));
  }
  return MemVitals{total.value(), used.value(), available.value(),
                   swapTotal.value(), swapUsed.value()};
}

Result<DiskVitals> parseDisk(const QJsonObject& object) {
  const auto unknown = json::rejectUnknown(
      object, {QStringLiteral("mount"), QStringLiteral("totalBytes"),
               QStringLiteral("usedBytes")});
  if (!unknown) return tl::unexpected(unknown.error());
  const auto mount = json::requiredString(
      object, QStringLiteral("mount"), 1, 4096);
  if (!mount) return tl::unexpected(mount.error());
  if (!QDir::isAbsolutePath(mount.value())) {
    return tl::unexpected(makeError(ErrorCode::ValidationFailed,
                                    QStringLiteral("disk mount must be absolute")));
  }
  const auto total = json::requiredUnsigned(
      object, QStringLiteral("totalBytes"), kMaxExactJsonInteger);
  if (!total) return tl::unexpected(total.error());
  const auto used = json::requiredUnsigned(
      object, QStringLiteral("usedBytes"), kMaxExactJsonInteger);
  if (!used) return tl::unexpected(used.error());
  if (used.value() > total.value()) {
    return tl::unexpected(makeError(ErrorCode::ValidationFailed,
                                    QStringLiteral("disk used exceeds total")));
  }
  return DiskVitals{mount.value(), total.value(), used.value()};
}

Result<NetVitals> parseNet(const QJsonObject& object) {
  const auto unknown = json::rejectUnknown(
      object, {QStringLiteral("iface"), QStringLiteral("rxBytesPerSec"),
               QStringLiteral("txBytesPerSec")});
  if (!unknown) return tl::unexpected(unknown.error());
  const auto iface = json::requiredString(
      object, QStringLiteral("iface"), 1, 64, true);
  if (!iface) return tl::unexpected(iface.error());
  static const QRegularExpression kInterfacePattern(
      QStringLiteral(R"(^[A-Za-z0-9_.:-]+$)"));
  if (!kInterfacePattern.match(iface.value()).hasMatch()) {
    return tl::unexpected(makeError(ErrorCode::ValidationFailed,
                                    QStringLiteral("invalid network interface")));
  }
  const auto rx = json::requiredUnsigned(
      object, QStringLiteral("rxBytesPerSec"), kMaxExactJsonInteger);
  if (!rx) return tl::unexpected(rx.error());
  const auto tx = json::requiredUnsigned(
      object, QStringLiteral("txBytesPerSec"), kMaxExactJsonInteger);
  if (!tx) return tl::unexpected(tx.error());
  return NetVitals{iface.value(), rx.value(), tx.value()};
}

QJsonObject cpuJson(const CpuVitals& cpu) {
  QJsonArray perCore;
  for (const auto value : cpu.perCorePct) perCore.append(value);
  return {{QStringLiteral("utilizationPct"), cpu.utilizationPct},
          {QStringLiteral("coreCount"), cpu.coreCount},
          {QStringLiteral("perCorePct"), perCore},
          {QStringLiteral("loadAvg1"), cpu.loadAvg1},
          {QStringLiteral("tempC"), nullableJsonNumber(cpu.tempC)}};
}

QJsonObject gpuJson(const GpuVitals& gpu) {
  return {{QStringLiteral("available"), gpu.available},
          {QStringLiteral("vendor"), gpu.vendor},
          {QStringLiteral("utilizationPct"), nullableJsonNumber(gpu.utilizationPct)},
          {QStringLiteral("memUsedBytes"), gpu.memUsedBytes},
          {QStringLiteral("memTotalBytes"), gpu.memTotalBytes},
          {QStringLiteral("tempC"), nullableJsonNumber(gpu.tempC)}};
}

QJsonObject memoryJson(const MemVitals& memory) {
  return {{QStringLiteral("totalBytes"), static_cast<double>(memory.totalBytes)},
          {QStringLiteral("usedBytes"), static_cast<double>(memory.usedBytes)},
          {QStringLiteral("availableBytes"),
           static_cast<double>(memory.availableBytes)},
          {QStringLiteral("swapTotalBytes"),
           static_cast<double>(memory.swapTotalBytes)},
          {QStringLiteral("swapUsedBytes"),
           static_cast<double>(memory.swapUsedBytes)}};
}

}  // namespace

Result<VitalsDto> VitalsDto::fromJson(const QJsonObject& object) {
  const auto unknown = json::rejectUnknown(
      object, {QStringLiteral("cpu"), QStringLiteral("gpu"),
               QStringLiteral("mem"), QStringLiteral("disks"),
               QStringLiteral("nets"), QStringLiteral("sampledAt")});
  if (!unknown) return tl::unexpected(unknown.error());
  for (const auto& key : {QStringLiteral("cpu"), QStringLiteral("gpu"),
                          QStringLiteral("mem")}) {
    if (!object.value(key).isObject()) {
      return tl::unexpected(makeError(
          ErrorCode::ValidationFailed,
          QStringLiteral("%1 must be an object").arg(key)));
    }
  }
  const auto cpu = parseCpu(object.value(QStringLiteral("cpu")).toObject());
  if (!cpu) return tl::unexpected(cpu.error());
  const auto gpu = parseGpu(object.value(QStringLiteral("gpu")).toObject());
  if (!gpu) return tl::unexpected(gpu.error());
  const auto mem = parseMemory(object.value(QStringLiteral("mem")).toObject());
  if (!mem) return tl::unexpected(mem.error());

  if (!object.value(QStringLiteral("disks")).isArray() ||
      !object.value(QStringLiteral("nets")).isArray()) {
    return tl::unexpected(makeError(ErrorCode::ValidationFailed,
                                    QStringLiteral("vitals lists must be arrays")));
  }
  const auto diskArray = object.value(QStringLiteral("disks")).toArray();
  const auto netArray = object.value(QStringLiteral("nets")).toArray();
  if (diskArray.size() > 4096 || netArray.size() > 4096) {
    return tl::unexpected(makeError(ErrorCode::ValidationFailed,
                                    QStringLiteral("vitals list exceeds cap")));
  }
  QVector<DiskVitals> disks;
  disks.reserve(diskArray.size());
  for (const auto& value : diskArray) {
    if (!value.isObject()) {
      return tl::unexpected(makeError(ErrorCode::ValidationFailed,
                                      QStringLiteral("disk entry must be object")));
    }
    const auto parsed = parseDisk(value.toObject());
    if (!parsed) return tl::unexpected(parsed.error());
    disks.append(parsed.value());
  }
  QVector<NetVitals> nets;
  nets.reserve(netArray.size());
  for (const auto& value : netArray) {
    if (!value.isObject()) {
      return tl::unexpected(makeError(ErrorCode::ValidationFailed,
                                      QStringLiteral("net entry must be object")));
    }
    const auto parsed = parseNet(value.toObject());
    if (!parsed) return tl::unexpected(parsed.error());
    nets.append(parsed.value());
  }
  const auto sampled = json::requiredDateTime(
      object, QStringLiteral("sampledAt"));
  if (!sampled) return tl::unexpected(sampled.error());
  return VitalsDto{cpu.value(), gpu.value(), mem.value(), disks, nets,
                   sampled.value()};
}

QJsonObject VitalsDto::toJson() const {
  QJsonArray diskArray;
  for (const auto& disk : disks) {
    diskArray.append(QJsonObject{
        {QStringLiteral("mount"), disk.mount},
        {QStringLiteral("totalBytes"), static_cast<double>(disk.totalBytes)},
        {QStringLiteral("usedBytes"), static_cast<double>(disk.usedBytes)}});
  }
  QJsonArray netArray;
  for (const auto& net : nets) {
    netArray.append(QJsonObject{
        {QStringLiteral("iface"), net.iface},
        {QStringLiteral("rxBytesPerSec"),
         static_cast<double>(net.rxBytesPerSec)},
        {QStringLiteral("txBytesPerSec"),
         static_cast<double>(net.txBytesPerSec)}});
  }
  return {{QStringLiteral("cpu"), cpuJson(cpu)},
          {QStringLiteral("gpu"), gpuJson(gpu)},
          {QStringLiteral("mem"), memoryJson(mem)},
          {QStringLiteral("disks"), diskArray},
          {QStringLiteral("nets"), netArray},
          {QStringLiteral("sampledAt"), json::dateTimeValue(sampledAt)}};
}

}  // namespace aegis::dto
