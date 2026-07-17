#include "models/package_list_model.h"

namespace aegis {

PackageListModel::PackageListModel(QObject* parent) : QAbstractListModel(parent) {}

int PackageListModel::rowCount(const QModelIndex& parent) const {
  return parent.isValid() ? 0 : static_cast<int>(items_.size());
}

QVariant PackageListModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid() || index.row() < 0 || index.row() >= items_.size()) {
    return {};
  }
  const auto& item = items_.at(index.row());
  switch (role) {
    case NameRole: return item.name;
    case VersionRole: return item.version;
    case SourceRole: return item.source;
    case SummaryRole: return item.summary;
    default: return {};
  }
}

QHash<int, QByteArray> PackageListModel::roleNames() const {
  return {{NameRole, "name"}, {VersionRole, "version"},
          {SourceRole, "source"}, {SummaryRole, "summary"}};
}

void PackageListModel::setItems(QVector<dto::PackageDto> items) {
  beginResetModel();
  items_ = std::move(items);
  endResetModel();
}

}  // namespace aegis
