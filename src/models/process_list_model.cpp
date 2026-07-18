#include "models/process_list_model.h"

#include <utility>

namespace aegis {

ProcessListModel::ProcessListModel(QObject* parent)
    : QAbstractListModel(parent) {}

int ProcessListModel::rowCount(const QModelIndex& parent) const {
  return parent.isValid() ? 0 : static_cast<int>(items_.size());
}

QVariant ProcessListModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid() || index.row() < 0 || index.row() >= items_.size()) {
    return {};
  }
  const auto& item = items_.at(index.row());
  switch (role) {
    case PidRole: return item.pid;
    case NameRole: return item.name;
    case UserRole: return item.user;
    case CpuPctRole: return item.cpuPct;
    case MemPctRole: return item.memPct;
    case CommandRole: return item.command;
    default: return {};
  }
}

QHash<int, QByteArray> ProcessListModel::roleNames() const {
  return {{PidRole, "pid"},       {NameRole, "name"},
          {UserRole, "user"},     {CpuPctRole, "cpuPct"},
          {MemPctRole, "memPct"}, {CommandRole, "command"}};
}

void ProcessListModel::setItems(QVector<dto::ProcessInfo> items) {
  beginResetModel();
  items_ = std::move(items);
  endResetModel();
}

QVector<dto::ProcessInfo> ProcessListModel::items() const { return items_; }

}  // namespace aegis
