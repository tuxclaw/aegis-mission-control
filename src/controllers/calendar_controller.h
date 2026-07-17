#pragma once

#include <QObject>

#include "models/calendar_event_model.h"

namespace aegis {
class CalendarStore;

class CalendarController final : public QObject {
  Q_OBJECT
  Q_PROPERTY(CalendarEventModel* events READ events CONSTANT)
  Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)

 public:
  // Creates a calendar controller over validated atomic persistence.
  explicit CalendarController(CalendarStore* store, QObject* parent = nullptr);
  [[nodiscard]] CalendarEventModel* events();
  [[nodiscard]] bool loading() const;
  Q_INVOKABLE void refresh();
  Q_INVOKABLE void createEvent(QString title, QDateTime start, QDateTime end,
                               bool allDay, QString location, QString color,
                               QString description);
  Q_INVOKABLE void updateEvent(QString id, QString title, QDateTime start,
                               QDateTime end, bool allDay, QString location,
                               QString color, QString description);
  Q_INVOKABLE void deleteEvent(QString id);
  Q_INVOKABLE void confirmDeleteEvent();

 signals:
  void loadingChanged();
  void errorRaised(QString message, bool retryable);
  void toast(QString message, int level);
  void confirmRequested(QString action, QString detail);

 private:
  void finishError(const AegisError& error);
  CalendarStore* store_;
  CalendarEventModel events_;
  bool loading_ = false;
  QString pendingDeleteId_;
};
}  // namespace aegis
