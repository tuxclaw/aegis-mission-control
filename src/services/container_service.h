#pragma once

#include <QObject>
#include <QTimer>

#include "core/result.h"
#include "dto/container_dto.h"

namespace aegis {

class ContainerService final : public QObject {
  Q_OBJECT

 public:
  // Creates a Podman-backed container inventory sampler.
  explicit ContainerService(QObject* parent = nullptr);
  // Starts ten-second periodic sampling and takes an immediate sample.
  void start();
  // Stops future periodic samples.
  void stop();
  // Requests an immediate sample.
  void refresh();

 signals:
  void sampled(dto::ContainerDto sample);
  void failed(AegisError error);

 private slots:
  void sampleNow();

 private:
  QTimer timer_;
  bool sampling_ = false;
};

}  // namespace aegis
