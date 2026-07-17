#pragma once

#include <QAbstractListModel>
#include <QVector>

#include "dto/memory_file_dto.h"

namespace aegis {

class MemoryFileModel : public QAbstractListModel {
  Q_OBJECT

 public:
  enum Role {
    NameRole = Qt::UserRole + 1,
    RelativePathRole,
    RootIdRole,
    SizeBytesRole,
    ModifiedAtRole,
  };
  Q_ENUM(Role)

  // Creates an empty memory-file model.
  explicit MemoryFileModel(QObject* parent = nullptr);
  // Returns the number of files.
  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  // Returns one file role value.
  QVariant data(const QModelIndex& index, int role) const override;
  // Returns stable QML role names.
  QHash<int, QByteArray> roleNames() const override;
  // Atomically replaces all files.
  void setItems(QVector<dto::MemoryFileDto> items);

 private:
  QVector<dto::MemoryFileDto> items_;
};

}  // namespace aegis
