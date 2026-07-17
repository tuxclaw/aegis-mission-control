#pragma once

#include <QDateTime>
#include <QJsonObject>
#include <QString>
#include <QVector>
#include <QtMath>

#include "core/result.h"

namespace aegis::dto {

struct CpuVitals {
  double utilizationPct = 0.0;
  int coreCount = 0;
  QVector<double> perCorePct;
  double loadAvg1 = 0.0;
  double tempC = qQNaN();
};

struct GpuVitals {
  bool available = false;
  QString vendor;
  double utilizationPct = qQNaN();
  double memUsedBytes = 0.0;
  double memTotalBytes = 0.0;
  double tempC = qQNaN();
};

struct MemVitals {
  quint64 totalBytes = 0;
  quint64 usedBytes = 0;
  quint64 availableBytes = 0;
  quint64 swapTotalBytes = 0;
  quint64 swapUsedBytes = 0;
};

struct DiskVitals {
  QString mount;
  quint64 totalBytes = 0;
  quint64 usedBytes = 0;
};

struct NetVitals {
  QString iface;
  quint64 rxBytesPerSec = 0;
  quint64 txBytesPerSec = 0;
};

struct VitalsDto {
  CpuVitals cpu;
  GpuVitals gpu;
  MemVitals mem;
  QVector<DiskVitals> disks;
  QVector<NetVitals> nets;
  QDateTime sampledAt;

  // Parses and fully validates a complete system-vitals sample.
  static Result<VitalsDto> fromJson(const QJsonObject& object);

  // Serializes this sample into its stable JSON representation.
  [[nodiscard]] QJsonObject toJson() const;
};

}  // namespace aegis::dto
