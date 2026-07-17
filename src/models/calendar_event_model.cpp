#include "models/calendar_event_model.h"

namespace aegis {

CalendarEventModel::CalendarEventModel(QObject* parent)
    : QAbstractListModel(parent) {}

int CalendarEventModel::rowCount(const QModelIndex& parent) const {
  return parent.isValid() ? 0 : static_cast<int>(items_.size());
}

QVariant CalendarEventModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid() || index.row() < 0 || index.row() >= items_.size()) {
    return {};
  }
  const auto& item = items_.at(index.row());
  switch (role) {
    case IdRole: return item.id;
    case TitleRole: return item.title;
    case DescriptionRole: return item.description;
    case StartRole: return item.start;
    case EndRole: return item.end;
    case AllDayRole: return item.allDay;
    case LocationRole: return item.location;
    case ColorRole: return item.color;
    default: return {};
  }
}

QHash<int, QByteArray> CalendarEventModel::roleNames() const {
  return {{IdRole, "id"}, {TitleRole, "title"},
          {DescriptionRole, "description"}, {StartRole, "start"},
          {EndRole, "end"}, {AllDayRole, "allDay"},
          {LocationRole, "location"}, {ColorRole, "color"}};
}

void CalendarEventModel::setItems(QVector<dto::CalendarEventDto> items) {
  beginResetModel();
  items_ = std::move(items);
  endResetModel();
}

QVector<dto::CalendarEventDto> CalendarEventModel::items() const {
  return items_;
}

}  // namespace aegis
