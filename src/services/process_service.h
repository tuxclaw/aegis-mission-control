#pragma once

#include <memory>

#include <QObject>
#include <QTimer>

#include "core/result.h"
#include "dto/process_dto.h"

namespace aegis {

class ProcessService final : public QObject {
  Q_OBJECT

 public:
  // Creates a Linux /proc process sampler.
  explicit ProcessService(QObject* parent = nullptr);
  // Starts five-second periodic sampling and takes an immediate sample.
  void start();
  // Stops future periodic samples.
  void stop();
  // Requests an immediate sample.
  void refresh();

 signals:
  void sampled(dto::ProcessDto sample);
  void failed(AegisError error);

 private slots:
  void sampleNow();

 private:
  struct SamplerState;
  static Result<dto::ProcessDto> collect(
      const std::shared_ptr<SamplerState>& state);

  QTimer timer_;
  std::shared_ptr<SamplerState> state_;
  bool sampling_ = false;
};

}  // namespace aegis
