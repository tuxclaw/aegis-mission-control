#include "models/container_list_model.h"

#include <utility>

namespace aegis {

ContainerListModel::ContainerListModel(QObject* parent)
    : QAbstractListModel(parent) {}

int ContainerListModel::rowCount(const QModelIndex& parent) const {
  return parent.isValid() ? 0 : static_cast<int>(items_.size());
}

QVariant ContainerListModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid() || index.row() < 0 || index.row() >= items_.size()) {
    return {};
  }
  const auto& item = items_.at(index.row());
  switch (role) {
    case IdRole: return item.id;
    case NameRole: return item.name;
    case ImageRole: return item.image;
    case StatusRole: return item.status;
    case PortsRole: return item.ports;
    case CreatedRole: return item.created;
    default: return {};
  }
}

QHash<int, QByteArray> ContainerListModel::roleNames() const {
  return {{IdRole, "id"},         {NameRole, "name"},
          {ImageRole, "image"},   {StatusRole, "status"},
          {PortsRole, "ports"},   {CreatedRole, "created"}};
}

void ContainerListModel::setItems(QVector<dto::ContainerInfo> items) {
  beginResetModel();
  items_ = std::move(items);
  endResetModel();
}

QVector<dto::ContainerInfo> ContainerListModel::items() const {
  return items_;
}

}  // namespace aegis
