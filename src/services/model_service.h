#pragma once

#include <QFuture>
#include <QObject>
#include <QVector>

#include "core/result.h"
#include "dto/model_dto.h"

namespace aegis {

class OpenClawCli;

class ModelService : public QObject {
  Q_OBJECT

 public:
  // Creates a live model service over OpenClaw CLI.
  explicit ModelService(OpenClawCli* cli, QObject* parent = nullptr);
  // Fetches the live model inventory.
  [[nodiscard]] QFuture<Result<QVector<dto::ModelDto>>> list();
  // Validates against a fresh inventory and switches active model.
  [[nodiscard]] QFuture<Result<dto::ModelDto>> setActive(QString modelId);

 signals:
  void activeModelChanged(QString modelId);

 private:
  OpenClawCli* cli_;
};

}  // namespace aegis
