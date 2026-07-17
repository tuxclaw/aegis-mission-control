#pragma once

#include <QObject>
#include <QVariantList>

#include "dto/vitals_dto.h"

namespace aegis {
class VitalsService;

class VitalsController final : public QObject {
  Q_OBJECT
  Q_PROPERTY(double cpuPct READ cpuPct NOTIFY vitalsChanged)
  Q_PROPERTY(double gpuPct READ gpuPct NOTIFY vitalsChanged)
  Q_PROPERTY(double memPct READ memPct NOTIFY vitalsChanged)
  Q_PROPERTY(double diskUsagePct READ diskUsagePct NOTIFY vitalsChanged)
  Q_PROPERTY(quint64 netRxBytesPerSec READ netRxBytesPerSec NOTIFY vitalsChanged)
  Q_PROPERTY(quint64 netTxBytesPerSec READ netTxBytesPerSec NOTIFY vitalsChanged)
  Q_PROPERTY(QString gpuVendor READ gpuVendor NOTIFY vitalsChanged)
  Q_PROPERTY(QString memUsed READ memUsed NOTIFY vitalsChanged)
  Q_PROPERTY(QString memTotal READ memTotal NOTIFY vitalsChanged)
  Q_PROPERTY(QVariantList diskModel READ diskModel NOTIFY vitalsChanged)
  Q_PROPERTY(QVariantList netModel READ netModel NOTIFY vitalsChanged)

 public:
  // Creates a presentation adapter for system samples.
  explicit VitalsController(VitalsService* service, QObject* parent = nullptr);
  [[nodiscard]] double cpuPct() const;
  [[nodiscard]] double gpuPct() const;
  [[nodiscard]] double memPct() const;
  [[nodiscard]] double diskUsagePct() const;
  [[nodiscard]] quint64 netRxBytesPerSec() const;
  [[nodiscard]] quint64 netTxBytesPerSec() const;
  [[nodiscard]] QString gpuVendor() const;
  [[nodiscard]] QString memUsed() const;
  [[nodiscard]] QString memTotal() const;
  [[nodiscard]] QVariantList diskModel() const;
  [[nodiscard]] QVariantList netModel() const;
  Q_INVOKABLE void start();
  Q_INVOKABLE void stop();

 signals:
  void vitalsChanged();
  void errorRaised(QString message, bool retryable);
  void toast(QString message, int level);

 private:
  VitalsService* service_;
  dto::VitalsDto sample_;
};
}  // namespace aegis
