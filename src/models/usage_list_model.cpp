#include "models/usage_list_model.h"

#include <utility>

#include <QVariantList>
#include <QVariantMap>

namespace {

QVariantList windowsToVariant(const QList<ProviderWindow>& providerWindows) {
  QVariantList windows;
  windows.reserve(providerWindows.size());
  for (const auto& window : providerWindows) {
    windows.append(QVariantMap{
        {QStringLiteral("label"), window.label},
        {QStringLiteral("tokensUsed"), window.tokensUsed},
        {QStringLiteral("tokensLimit"), window.tokensLimit},
        {QStringLiteral("creditsUsed"), window.creditsUsed},
        {QStringLiteral("creditsLimit"), window.creditsLimit},
        {QStringLiteral("usedFraction"), window.usedFraction},
        {QStringLiteral("resetsAt"), window.resetsAt},
    });
  }
  return windows;
}

}  // namespace

UsageListModel::UsageListModel(QObject* parent) : QAbstractListModel(parent) {}

int UsageListModel::rowCount(const QModelIndex& parent) const {
  return parent.isValid() ? 0 : static_cast<int>(quotas_.size());
}

QVariant UsageListModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid() || index.row() < 0 ||
      static_cast<qsizetype>(index.row()) >= quotas_.size()) {
    return {};
  }

  const auto& quota = quotas_.at(index.row());
  switch (role) {
    case IdRole:
      return quota.id;
    case DisplayNameRole:
      return quota.displayName;
    case EnabledRole:
      return quota.enabled;
    case FetchedRole:
      return quota.fetched;
    case HasErrorRole:
      return quota.hasError;
    case ErrorMsgRole:
      return quota.errorMsg;
    case PrimaryFractionRole:
      return qBound(0.0, quota.primaryFraction(), 1.0);
    case PlanNameRole:
      return quota.planName;
    case WindowsRole:
      return windowsToVariant(quota.windows);
    case BalanceCashRole:
      return quota.balance.cash;
    case BalanceGiftRole:
      return quota.balance.gift;
    case BalanceTotalRole:
      return quota.balance.total;
    case BalanceCurrencyRole:
      return quota.balance.currency;
    case FetchedAtRole:
      return quota.fetchedAt;
    default:
      return {};
  }
}

QHash<int, QByteArray> UsageListModel::roleNames() const {
  return {{IdRole, "id"},
          {DisplayNameRole, "displayName"},
          {EnabledRole, "enabled"},
          {FetchedRole, "fetched"},
          {HasErrorRole, "hasError"},
          {ErrorMsgRole, "errorMsg"},
          {PrimaryFractionRole, "primaryFraction"},
          {PlanNameRole, "planName"},
          {WindowsRole, "windows"},
          {BalanceCashRole, "balanceCash"},
          {BalanceGiftRole, "balanceGift"},
          {BalanceTotalRole, "balanceTotal"},
          {BalanceCurrencyRole, "balanceCurrency"},
          {FetchedAtRole, "fetchedAt"}};
}

void UsageListModel::setQuotas(QList<ProviderQuota> quotas) {
  beginResetModel();
  quotas_ = std::move(quotas);
  endResetModel();
}
