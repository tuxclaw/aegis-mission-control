#pragma once

#include <QAbstractListModel>
#include <QVector>

#include "dto/git_status_dto.h"

namespace aegis {

class GitFileModel : public QAbstractListModel {
  Q_OBJECT

 public:
  enum Role {
    PathRole = Qt::UserRole + 1,
    IndexStateRole,
    WorktreeStateRole,
    StagedRole,
    GroupRole,
    FilePathRole,
  };
  Q_ENUM(Role)

  // Creates an empty git-file model.
  explicit GitFileModel(QObject* parent = nullptr);
  // Returns the number of status entries.
  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  // Returns one status-entry role value.
  QVariant data(const QModelIndex& index, int role) const override;
  // Returns stable QML role names.
  QHash<int, QByteArray> roleNames() const override;
  // Replaces and groups entries as staged, unstaged, then untracked.
  void setItems(QVector<dto::GitFileEntry> items);
  // Returns all staged paths.
  [[nodiscard]] QStringList stagedPaths() const;

 private:
  QVector<dto::GitFileEntry> items_;
};

}  // namespace aegis
