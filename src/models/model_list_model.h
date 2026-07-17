#pragma once

#include <QAbstractListModel>
#include <QVector>

#include "dto/model_dto.h"

namespace aegis {

class ModelListModel : public QAbstractListModel {
  Q_OBJECT

 public:
  enum Role { IdRole = Qt::UserRole + 1, ProviderRole, LabelRole, IsActiveRole };
  Q_ENUM(Role)

  // Creates an empty model inventory.
  explicit ModelListModel(QObject* parent = nullptr);
  // Returns the number of models.
  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  // Returns one model role value.
  QVariant data(const QModelIndex& index, int role) const override;
  // Returns stable QML role names.
  QHash<int, QByteArray> roleNames() const override;
  // Atomically replaces all models.
  void setItems(QVector<dto::ModelDto> items);

 private:
  QVector<dto::ModelDto> items_;
};

}  // namespace aegis
