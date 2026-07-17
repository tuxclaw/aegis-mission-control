#include "models/cron_job_model.h"

namespace aegis {

CronJobModel::CronJobModel(QObject* parent) : QAbstractListModel(parent) {}

int CronJobModel::rowCount(const QModelIndex& parent) const {
  return parent.isValid() ? 0 : static_cast<int>(items_.size());
}

QVariant CronJobModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid() || index.row() < 0 || index.row() >= items_.size()) {
    return {};
  }
  const auto& item = items_.at(index.row());
  switch (role) {
    case IdRole: return item.id;
    case NameRole: return item.name;
    case ScheduleRole: return item.schedule;
    case StateRole: return static_cast<int>(item.state);
    case CommandRole: return item.command;
    case LastRunRole: return item.lastRun;
    case NextRunRole: return item.nextRun;
    case LastResultRole: return item.lastResult;
    default: return {};
  }
}

QHash<int, QByteArray> CronJobModel::roleNames() const {
  return {{IdRole, "id"}, {NameRole, "name"},
          {ScheduleRole, "schedule"}, {StateRole, "state"},
          {CommandRole, "command"}, {LastRunRole, "lastRun"},
          {NextRunRole, "nextRun"}, {LastResultRole, "lastResult"}};
}

void CronJobModel::setItems(QVector<dto::CronJobDto> items) {
  beginResetModel();
  items_ = std::move(items);
  endResetModel();
}

QVector<dto::CronJobDto> CronJobModel::items() const { return items_; }

}  // namespace aegis
