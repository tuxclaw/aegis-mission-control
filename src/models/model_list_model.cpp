#include "models/model_list_model.h"

namespace aegis {

ModelListModel::ModelListModel(QObject* parent) : QAbstractListModel(parent) {}

int ModelListModel::rowCount(const QModelIndex& parent) const {
  return parent.isValid() ? 0 : static_cast<int>(items_.size());
}

QVariant ModelListModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid() || index.row() < 0 || index.row() >= items_.size()) {
    return {};
  }
  const auto& item = items_.at(index.row());
  switch (role) {
    case IdRole: return item.id;
    case ProviderRole: return item.provider;
    case LabelRole: return item.label;
    case IsActiveRole: return item.isActive;
    default: return {};
  }
}

QHash<int, QByteArray> ModelListModel::roleNames() const {
  return {{IdRole, "id"}, {ProviderRole, "provider"},
          {LabelRole, "label"}, {IsActiveRole, "isActive"}};
}

void ModelListModel::setItems(QVector<dto::ModelDto> items) {
  beginResetModel();
  items_ = std::move(items);
  endResetModel();
}

}  // namespace aegis
