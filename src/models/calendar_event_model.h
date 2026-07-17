#pragma once

#include <QAbstractListModel>
#include <QVector>

#include "dto/calendar_event_dto.h"

namespace aegis {

class CalendarEventModel : public QAbstractListModel {
  Q_OBJECT

 public:
  enum Role {
    IdRole = Qt::UserRole + 1,
    TitleRole,
    DescriptionRole,
    StartRole,
    EndRole,
    AllDayRole,
    LocationRole,
    ColorRole,
  };
  Q_ENUM(Role)

  // Creates an empty calendar-event model.
  explicit CalendarEventModel(QObject* parent = nullptr);
  // Returns the number of events.
  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  // Returns one event role value.
  QVariant data(const QModelIndex& index, int role) const override;
  // Returns stable QML role names.
  QHash<int, QByteArray> roleNames() const override;
  // Atomically replaces all events.
  void setItems(QVector<dto::CalendarEventDto> items);
  // Returns a copy of all events.
  [[nodiscard]] QVector<dto::CalendarEventDto> items() const;

 private:
  QVector<dto::CalendarEventDto> items_;
};

}  // namespace aegis
