#pragma once

#include <QAbstractListModel>
#include <QList>

#include "dto/provider_quota.h"

class UsageListModel final : public QAbstractListModel {
  Q_OBJECT

 public:
  enum Role {
    IdRole = Qt::UserRole + 1,
    DisplayNameRole,
    EnabledRole,
    FetchedRole,
    HasErrorRole,
    ErrorMsgRole,
    PrimaryFractionRole,
    PlanNameRole,
    WindowsRole,
    BalanceCashRole,
    BalanceGiftRole,
    BalanceTotalRole,
    BalanceCurrencyRole,
    FetchedAtRole,
  };
  Q_ENUM(Role)

  explicit UsageListModel(QObject* parent = nullptr);

  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  QVariant data(const QModelIndex& index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;

  void setQuotas(QList<ProviderQuota> quotas);

 private:
  QList<ProviderQuota> quotas_;
};
