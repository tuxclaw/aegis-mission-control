#pragma once

#include <chrono>
#include <memory>

#include <QObject>
#include <QTimer>

#include "core/result.h"
#include "dto/vitals_dto.h"

namespace aegis {

class ConfigService;

class VitalsService : public QObject {
  Q_OBJECT

 public:
  // Creates an off-thread Linux system sampler.
  explicit VitalsService(ConfigService* config, QObject* parent = nullptr);
  // Starts periodic sampling at a validated interval.
  void start(std::chrono::milliseconds interval);
  // Stops future sampling.
  void stop();

 signals:
  void sampled(dto::VitalsDto sample);
  void failed(AegisError error);

 private slots:
  void sampleNow();

 private:
  struct SamplerState;

  ConfigService* config_;
  QTimer timer_;
  std::shared_ptr<SamplerState> state_;
  bool sampling_ = false;
};

}  // namespace aegis
