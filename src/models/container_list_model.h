#pragma once

#include <QAbstractListModel>
#include <QVector>

#include "dto/container_dto.h"

namespace aegis {

class ContainerListModel final : public QAbstractListModel {
  Q_OBJECT

 public:
  enum Role {
    IdRole = Qt::UserRole + 1,
    NameRole,
    ImageRole,
    StatusRole,
    PortsRole,
    CreatedRole,
  };
  Q_ENUM(Role)

  explicit ContainerListModel(QObject* parent = nullptr);
  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  QVariant data(const QModelIndex& index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;
  void setItems(QVector<dto::ContainerInfo> items);
  [[nodiscard]] QVector<dto::ContainerInfo> items() const;

 private:
  QVector<dto::ContainerInfo> items_;
};

}  // namespace aegis
