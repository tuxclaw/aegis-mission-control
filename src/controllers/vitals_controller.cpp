#include "controllers/vitals_controller.h"

#include <chrono>

#include "services/vitals_service.h"

namespace aegis {
VitalsController::VitalsController(VitalsService* service, QObject* parent)
    : QObject(parent), service_(service) {
  connect(service_, &VitalsService::sampled, this,
          [this](const dto::VitalsDto& sample) {
            sample_ = sample;
            emit vitalsChanged();
          });
  connect(service_, &VitalsService::failed, this,
          [this](const AegisError& error) {
            emit errorRaised(error.userMessage, error.retryable);
            emit toast(error.userMessage, 3);
          });
}

double VitalsController::cpuPct() const { return sample_.cpu.utilizationPct; }

double VitalsController::gpuPct() const { return sample_.gpu.utilizationPct; }

double VitalsController::memPct() const {
  return sample_.mem.totalBytes
             ? 100.0 * static_cast<double>(sample_.mem.usedBytes) /
                   static_cast<double>(sample_.mem.totalBytes)
             : 0.0;
}

double VitalsController::diskUsagePct() const {
  return sample_.disks.isEmpty() || !sample_.disks.first().totalBytes
             ? 0.0
             : 100.0 * static_cast<double>(sample_.disks.first().usedBytes) /
                   static_cast<double>(sample_.disks.first().totalBytes);
}

quint64 VitalsController::netRxBytesPerSec() const {
  quint64 bytesPerSecond = 0;
  for (const auto& net : sample_.nets) {
    bytesPerSecond += net.rxBytesPerSec;
  }
  return bytesPerSecond;
}

quint64 VitalsController::netTxBytesPerSec() const {
  quint64 bytesPerSecond = 0;
  for (const auto& net : sample_.nets) {
    bytesPerSecond += net.txBytesPerSec;
  }
  return bytesPerSecond;
}

QString VitalsController::gpuVendor() const { return sample_.gpu.vendor; }

QString VitalsController::memUsed() const {
  return QString::number(static_cast<double>(sample_.mem.usedBytes) /
                             (1024.0 * 1024.0 * 1024.0),
                         'f', 1) +
         QStringLiteral(" GiB");
}

QString VitalsController::memTotal() const {
  return QString::number(static_cast<double>(sample_.mem.totalBytes) /
                             (1024.0 * 1024.0 * 1024.0),
                         'f', 1) +
         QStringLiteral(" GiB");
}

QVariantList VitalsController::diskModel() const {
  QVariantList disks;
  for (const auto& disk : sample_.disks) {
    disks.append(QVariantMap{
        {QStringLiteral("mount"), disk.mount},
        {QStringLiteral("totalBytes"), QVariant::fromValue(disk.totalBytes)},
        {QStringLiteral("usedBytes"), QVariant::fromValue(disk.usedBytes)}});
  }
  return disks;
}

QVariantList VitalsController::netModel() const {
  QVariantList nets;
  for (const auto& net : sample_.nets) {
    nets.append(QVariantMap{
        {QStringLiteral("iface"), net.iface},
        {QStringLiteral("rxBytesPerSec"),
         QVariant::fromValue(net.rxBytesPerSec)},
        {QStringLiteral("txBytesPerSec"),
         QVariant::fromValue(net.txBytesPerSec)}});
  }
  return nets;
}

void VitalsController::start() {
  service_->start(std::chrono::milliseconds(0));
}

void VitalsController::stop() { service_->stop(); }

}  // namespace aegis
