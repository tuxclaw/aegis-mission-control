#pragma once

#include <functional>
#include <memory>
#include <utility>

#include <QFuture>
#include <QFutureWatcher>
#include <QObject>
#include <QPromise>
#include <QThread>
#include <QThreadPool>
#include <QtConcurrentRun>

namespace aegis::async {

// Configures the global pool to retain at least four service workers.
inline void configureGlobalThreadPool() {
  const auto ideal = QThread::idealThreadCount();
  QThreadPool::globalInstance()->setMaxThreadCount(qMax(4, ideal));
}

// Runs a callable on the global service thread pool.
template <typename Function>
auto run(Function&& function) {
  return QtConcurrent::run(QThreadPool::globalInstance(),
                           std::forward<Function>(function));
}

// Flattens a future-of-future without blocking either the GUI or worker thread.
template <typename T>
QFuture<T> flatten(QObject* context, QFuture<QFuture<T>> nested) {
  auto promise = std::make_shared<QPromise<T>>();
  promise->start();
  auto result = promise->future();
  auto* outer = new QFutureWatcher<QFuture<T>>(context);
  QObject::connect(outer, &QFutureWatcher<QFuture<T>>::finished, context,
                   [context, outer, promise] {
    auto innerFuture = outer->result();
    outer->deleteLater();
    auto* inner = new QFutureWatcher<T>(context);
    QObject::connect(inner, &QFutureWatcher<T>::finished, context,
                     [inner, promise] {
      promise->addResult(inner->result());
      promise->finish();
      inner->deleteLater();
    });
    inner->setFuture(std::move(innerFuture));
  });
  outer->setFuture(std::move(nested));
  return result;
}

// Observes a future and invokes the callback on the context object's thread.
template <typename T, typename Callback>
std::shared_ptr<QFutureWatcher<T>> watch(QObject* context, QFuture<T> future,
                                         Callback&& callback) {
  auto watcher = std::make_shared<QFutureWatcher<T>>();
  QObject::connect(
      watcher.get(), &QFutureWatcher<T>::finished, context,
      [watcher, callback = std::forward<Callback>(callback)]() mutable {
        callback(watcher->result());
      });
  watcher->setFuture(std::move(future));
  return watcher;
}

}  // namespace aegis::async
