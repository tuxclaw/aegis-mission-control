#include "models/git_file_model.h"

#include <algorithm>

namespace aegis {
namespace {

int groupRank(const dto::GitFileEntry& entry) {
  if (entry.staged) return 0;
  if (entry.worktreeState == dto::GitFileState::Untracked) return 2;
  return 1;
}

QString groupName(const dto::GitFileEntry& entry) {
  if (entry.staged) return QStringLiteral("staged");
  if (entry.worktreeState == dto::GitFileState::Untracked) {
    return QStringLiteral("untracked");
  }
  return QStringLiteral("unstaged");
}

}  // namespace

GitFileModel::GitFileModel(QObject* parent) : QAbstractListModel(parent) {}

int GitFileModel::rowCount(const QModelIndex& parent) const {
  return parent.isValid() ? 0 : static_cast<int>(items_.size());
}

QVariant GitFileModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid() || index.row() < 0 || index.row() >= items_.size()) {
    return {};
  }
  const auto& item = items_.at(index.row());
  switch (role) {
    case PathRole: return item.path;
    case IndexStateRole: return static_cast<int>(item.indexState);
    case WorktreeStateRole: return static_cast<int>(item.worktreeState);
    case StagedRole: return item.staged;
    case GroupRole: return groupName(item);
    case FilePathRole: return item.path;
    default: return {};
  }
}

QHash<int, QByteArray> GitFileModel::roleNames() const {
  return {{PathRole, "path"}, {IndexStateRole, "indexState"},
          {WorktreeStateRole, "worktreeState"}, {StagedRole, "staged"},
          {GroupRole, "group"}, {FilePathRole, "filePath"}};
}

void GitFileModel::setItems(QVector<dto::GitFileEntry> items) {
  std::stable_sort(items.begin(), items.end(), [](const auto& left,
                                                  const auto& right) {
    const auto leftRank = groupRank(left);
    const auto rightRank = groupRank(right);
    return leftRank == rightRank ? left.path < right.path : leftRank < rightRank;
  });
  beginResetModel();
  items_ = std::move(items);
  endResetModel();
}

QStringList GitFileModel::stagedPaths() const {
  QStringList result;
  for (const auto& item : items_) {
    if (item.staged) result.append(item.path);
  }
  return result;
}

}  // namespace aegis
