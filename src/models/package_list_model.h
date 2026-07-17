#pragma once

#include <QAbstractListModel>
#include <QVector>

#include "dto/package_dto.h"

namespace aegis {

class PackageListModel : public QAbstractListModel {
  Q_OBJECT

 public:
  enum Role { NameRole = Qt::UserRole + 1, VersionRole, SourceRole, SummaryRole };
  Q_ENUM(Role)

  // Creates an empty package inventory.
  explicit PackageListModel(QObject* parent = nullptr);
  // Returns the number of packages.
  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  // Returns one package role value.
  QVariant data(const QModelIndex& index, int role) const override;
  // Returns stable QML role names.
  QHash<int, QByteArray> roleNames() const override;
  // Atomically replaces all packages.
  void setItems(QVector<dto::PackageDto> items);

 private:
  QVector<dto::PackageDto> items_;
};

}  // namespace aegis
