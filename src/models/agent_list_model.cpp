#include "models/agent_list_model.h"

namespace aegis {

AgentListModel::AgentListModel(QObject* parent) : QAbstractListModel(parent) {}

int AgentListModel::rowCount(const QModelIndex& parent) const {
  return parent.isValid() ? 0 : static_cast<int>(items_.size());
}

QVariant AgentListModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid() || index.row() < 0 || index.row() >= items_.size()) {
    return {};
  }
  const auto& item = items_.at(index.row());
  switch (role) {
    case IdRole: return item.id;
    case DisplayNameRole: return item.displayName;
    case ModelRole: return item.model;
    case StatusRole: return static_cast<int>(item.status);
    case StatusDetailRole: return item.statusDetail;
    case LastSeenRole: return item.lastSeen;
    case ActiveSessionsRole: return item.activeSessions;
    case AvatarSeedRole: return item.avatarSeed;
    default: return {};
  }
}

QHash<int, QByteArray> AgentListModel::roleNames() const {
  return {{IdRole, "id"}, {DisplayNameRole, "displayName"},
          {ModelRole, "model"}, {StatusRole, "status"},
          {StatusDetailRole, "statusDetail"}, {LastSeenRole, "lastSeen"},
          {ActiveSessionsRole, "activeSessions"},
          {AvatarSeedRole, "avatarSeed"}};
}

void AgentListModel::setItems(QVector<dto::AgentDto> items) {
  beginResetModel();
  items_ = std::move(items);
  endResetModel();
}

QVector<dto::AgentDto> AgentListModel::items() const { return items_; }

}  // namespace aegis
