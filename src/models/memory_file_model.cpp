#include "models/memory_file_model.h"

namespace aegis {

MemoryFileModel::MemoryFileModel(QObject* parent) : QAbstractListModel(parent) {}

int MemoryFileModel::rowCount(const QModelIndex& parent) const {
  return parent.isValid() ? 0 : static_cast<int>(items_.size());
}

QVariant MemoryFileModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid() || index.row() < 0 || index.row() >= items_.size()) {
    return {};
  }
  const auto& item = items_.at(index.row());
  switch (role) {
    case NameRole: return item.name;
    case RelativePathRole: return item.relativePath;
    case RootIdRole: return item.rootId;
    case SizeBytesRole: return QVariant::fromValue(item.sizeBytes);
    case ModifiedAtRole: return item.modifiedAt;
    default: return {};
  }
}

QHash<int, QByteArray> MemoryFileModel::roleNames() const {
  return {{NameRole, "name"}, {RelativePathRole, "relativePath"},
          {RootIdRole, "rootId"}, {SizeBytesRole, "sizeBytes"},
          {ModifiedAtRole, "modifiedAt"}};
}

void MemoryFileModel::setItems(QVector<dto::MemoryFileDto> items) {
  beginResetModel();
  items_ = std::move(items);
  endResetModel();
}

}  // namespace aegis
