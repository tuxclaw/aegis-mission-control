#pragma once

#include <QFuture>
#include <QObject>
#include <QString>

#include "core/result.h"

namespace aegis {

class SecretStore : public QObject {
 Q_OBJECT

 public:
  // Creates a keychain wrapper for the dev.tux.aegis service.
  explicit SecretStore(QObject* parent = nullptr);

  // Reads a secret from the OS keychain and fails closed when unavailable.
  [[nodiscard]] QFuture<Result<QString>> read(const QString& key);

  // Writes a non-empty secret into the OS keychain.
  [[nodiscard]] QFuture<Result<void>> write(const QString& key,
                                             const QString& value);

  // Erases a secret from the OS keychain; absence is treated as success.
  [[nodiscard]] QFuture<Result<void>> erase(const QString& key);

  // Reports whether the keychain contains a non-empty value for a key.
  [[nodiscard]] QFuture<Result<bool>> has(const QString& key);

 private:
  static constexpr auto kServiceName = "dev.tux.aegis";
};

}  // namespace aegis
