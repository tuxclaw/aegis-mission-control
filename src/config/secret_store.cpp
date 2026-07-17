#include "config/secret_store.h"

#include <memory>

#include <QPromise>
#include <QRegularExpression>

#include "core/logging.h"

#if defined(AEGIS_HAS_QTKEYCHAIN)
#include <qt6keychain/keychain.h>
#endif

namespace aegis {
namespace {

template <typename T>
QFuture<Result<T>> readyResult(Result<T> result) {
  QPromise<Result<T>> promise;
  promise.start();
  promise.addResult(std::move(result));
  promise.finish();
  return promise.future();
}

Result<void> validateKey(const QString& key) {
  static const QRegularExpression kPattern(
      QStringLiteral(R"(^[A-Za-z0-9][A-Za-z0-9._-]{0,127}$)"));
  if (!kPattern.match(key).hasMatch()) {
    return tl::unexpected(makeError(ErrorCode::ValidationFailed,
                                    QStringLiteral("invalid secret key")));
  }
  return {};
}

#if !defined(AEGIS_HAS_QTKEYCHAIN)
AegisError unavailableError() {
  return makeError(ErrorCode::ConfigInvalid,
                   QStringLiteral("OS keychain backend is unavailable"));
}
#endif

#if defined(AEGIS_HAS_QTKEYCHAIN)
AegisError keychainError(QKeychain::Error error, const QString& operation) {
  if (error == QKeychain::EntryNotFound) {
    return makeError(ErrorCode::MissingToken,
                     QStringLiteral("keychain entry not found"));
  }
  return makeError(ErrorCode::ConfigInvalid,
                   QStringLiteral("keychain %1 failed with code %2")
                       .arg(operation)
                       .arg(static_cast<int>(error)));
}
#endif

}  // namespace

SecretStore::SecretStore(QObject* parent) : QObject(parent) {}

QFuture<Result<QString>> SecretStore::read(const QString& key) {
  const auto valid = validateKey(key);
  if (!valid) return readyResult<QString>(tl::unexpected(valid.error()));
#if defined(AEGIS_HAS_QTKEYCHAIN)
  auto promise = std::make_shared<QPromise<Result<QString>>>();
  promise->start();
  auto* job = new QKeychain::ReadPasswordJob(
      QString::fromLatin1(kServiceName), this);
  job->setKey(key);
  connect(job, &QKeychain::Job::finished, this, [promise, job]() {
    if (job->error() != QKeychain::NoError) {
      promise->addResult(tl::unexpected(keychainError(job->error(),
                                                      QStringLiteral("read"))));
    } else if (job->textData().isEmpty()) {
      promise->addResult(tl::unexpected(makeError(
          ErrorCode::MissingToken, QStringLiteral("keychain value is empty"))));
    } else {
      logging::registerSecret(job->textData());
      promise->addResult(job->textData());
    }
    promise->finish();
  });
  job->start();
  return promise->future();
#else
  return readyResult<QString>(tl::unexpected(unavailableError()));
#endif
}

QFuture<Result<void>> SecretStore::write(const QString& key,
                                          const QString& value) {
  const auto valid = validateKey(key);
  if (!valid) return readyResult<void>(tl::unexpected(valid.error()));
  if (value.isEmpty() || value.contains(QChar::Null) || value.size() > 65536) {
    return readyResult<void>(tl::unexpected(makeError(
        ErrorCode::ValidationFailed, QStringLiteral("invalid secret value"))));
  }
#if defined(AEGIS_HAS_QTKEYCHAIN)
  logging::registerSecret(value);
  auto promise = std::make_shared<QPromise<Result<void>>>();
  promise->start();
  auto* job = new QKeychain::WritePasswordJob(
      QString::fromLatin1(kServiceName), this);
  job->setKey(key);
  job->setTextData(value);
  connect(job, &QKeychain::Job::finished, this, [promise, job]() {
    if (job->error() != QKeychain::NoError) {
      promise->addResult(tl::unexpected(keychainError(job->error(),
                                                      QStringLiteral("write"))));
    } else {
      promise->addResult(Result<void>());
    }
    promise->finish();
  });
  job->start();
  return promise->future();
#else
  return readyResult<void>(tl::unexpected(unavailableError()));
#endif
}

QFuture<Result<void>> SecretStore::erase(const QString& key) {
  const auto valid = validateKey(key);
  if (!valid) return readyResult<void>(tl::unexpected(valid.error()));
#if defined(AEGIS_HAS_QTKEYCHAIN)
  auto promise = std::make_shared<QPromise<Result<void>>>();
  promise->start();
  auto* job = new QKeychain::DeletePasswordJob(
      QString::fromLatin1(kServiceName), this);
  job->setKey(key);
  connect(job, &QKeychain::Job::finished, this, [promise, job]() {
    if (job->error() != QKeychain::NoError &&
        job->error() != QKeychain::EntryNotFound) {
      promise->addResult(tl::unexpected(keychainError(job->error(),
                                                      QStringLiteral("erase"))));
    } else {
      promise->addResult(Result<void>());
    }
    promise->finish();
  });
  job->start();
  return promise->future();
#else
  return readyResult<void>(tl::unexpected(unavailableError()));
#endif
}

QFuture<Result<bool>> SecretStore::has(const QString& key) {
  return read(key).then(this, [](const Result<QString>& result) -> Result<bool> {
    if (result) return !result->isEmpty();
    if (result.error().code == ErrorCode::MissingToken) return false;
    return tl::unexpected(result.error());
  });
}

}  // namespace aegis
