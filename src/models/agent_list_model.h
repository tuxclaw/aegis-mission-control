#pragma once

#include <QAbstractListModel>
#include <QVector>

#include "dto/agent_dto.h"

namespace aegis {

class AgentListModel : public QAbstractListModel {
  Q_OBJECT

 public:
  enum Role {
    IdRole = Qt::UserRole + 1,
    DisplayNameRole,
    ModelRole,
    StatusRole,
    StatusDetailRole,
    LastSeenRole,
    ActiveSessionsRole,
    AvatarSeedRole,
  };
  Q_ENUM(Role)

  // Creates an empty agent list model.
  explicit AgentListModel(QObject* parent = nullptr);
  // Returns the number of visible agents.
  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  // Returns one agent role value.
  QVariant data(const QModelIndex& index, int role) const override;
  // Returns stable QML role names.
  QHash<int, QByteArray> roleNames() const override;
  // Atomically replaces the model contents.
  void setItems(QVector<dto::AgentDto> items);
  // Returns a copy of the current contents.
  [[nodiscard]] QVector<dto::AgentDto> items() const;

 private:
  QVector<dto::AgentDto> items_;
};

}  // namespace aegis
