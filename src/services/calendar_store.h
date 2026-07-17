#pragma once

#include <QFuture>
#include <QMutex>
#include <QObject>
#include <QVector>

#include "core/atomic_file.h"
#include "core/result.h"
#include "dto/calendar_event_dto.h"

namespace aegis {

class ConfigService;

class CalendarStore : public QObject {
  Q_OBJECT

 public:
  // Creates a validated atomic calendar store.
  explicit CalendarStore(ConfigService* config, QObject* parent = nullptr);
  // Loads and validates every persisted event.
  [[nodiscard]] QFuture<Result<QVector<dto::CalendarEventDto>>> load();
  // Validates, assigns identity and timestamps, then persists a draft.
  [[nodiscard]] QFuture<Result<dto::CalendarEventDto>> create(
      dto::CalendarEventDto draft);
  // Validates and atomically persists an existing event.
  [[nodiscard]] QFuture<Result<dto::CalendarEventDto>> update(
      dto::CalendarEventDto event);
  // Removes one validated event id atomically.
  [[nodiscard]] QFuture<Result<void>> remove(QString id);

 signals:
  void changed();

 private:
  Result<QString> storePath() const;
  Result<QVector<dto::CalendarEventDto>> loadLocked(const QString& path) const;
  Result<void> writeLocked(const QString& path,
                           const QVector<dto::CalendarEventDto>& events);

  ConfigService* config_;
  mutable QMutex mutex_;
  AtomicFile atomicFile_;
};

}  // namespace aegis
