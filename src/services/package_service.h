#pragma once

#include <QFuture>
#include <QObject>
#include <QVector>

#include "core/result.h"
#include "dto/package_dto.h"

namespace aegis {

class ConfigService;

class PackageService : public QObject {
  Q_OBJECT

 public:
  // Creates a read-only package inventory service.
  explicit PackageService(ConfigService* config, QObject* parent = nullptr);
  // Runs the configured read-only query and parses bounded output.
  [[nodiscard]] QFuture<Result<QVector<dto::PackageDto>>> list();

 private:
  ConfigService* config_;
};

}  // namespace aegis
