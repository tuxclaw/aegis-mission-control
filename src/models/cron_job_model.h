#pragma once

#include <QAbstractListModel>
#include <QVector>

#include "dto/cron_job_dto.h"

namespace aegis {

class CronJobModel : public QAbstractListModel {
  Q_OBJECT

 public:
  enum Role {
    IdRole = Qt::UserRole + 1,
    NameRole,
    ScheduleRole,
    StateRole,
    CommandRole,
    LastRunRole,
    NextRunRole,
    LastResultRole,
  };
  Q_ENUM(Role)

  // Creates an empty cron-job model.
  explicit CronJobModel(QObject* parent = nullptr);
  // Returns the number of jobs.
  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  // Returns one job role value.
  QVariant data(const QModelIndex& index, int role) const override;
  // Returns stable QML role names.
  QHash<int, QByteArray> roleNames() const override;
  // Atomically replaces all jobs.
  void setItems(QVector<dto::CronJobDto> items);
  // Returns a copy of all jobs.
  [[nodiscard]] QVector<dto::CronJobDto> items() const;

 private:
  QVector<dto::CronJobDto> items_;
};

}  // namespace aegis
