#pragma once

#include <QAbstractListModel>
#include <QVector>

#include "dto/process_dto.h"

namespace aegis {

class ProcessListModel final : public QAbstractListModel {
  Q_OBJECT

 public:
  enum Role {
    PidRole = Qt::UserRole + 1,
    NameRole,
    UserRole,
    CpuPctRole,
    MemPctRole,
    CommandRole,
  };
  Q_ENUM(Role)

  explicit ProcessListModel(QObject* parent = nullptr);
  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  QVariant data(const QModelIndex& index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;
  void setItems(QVector<dto::ProcessInfo> items);
  [[nodiscard]] QVector<dto::ProcessInfo> items() const;

 private:
  QVector<dto::ProcessInfo> items_;
};

}  // namespace aegis
