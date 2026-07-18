#include "controllers/usage_controller.h"

#include <utility>

#include <QVariantMap>

#include "dto/provider_quota.h"
#include "services/provider_manager.h"

namespace {

QVariantMap windowToVariant(const ProviderWindow& window) {
  return {{QStringLiteral("label"), window.label},
          {QStringLiteral("tokensUsed"), window.tokensUsed},
          {QStringLiteral("tokensLimit"), window.tokensLimit},
          {QStringLiteral("creditsUsed"), window.creditsUsed},
          {QStringLiteral("creditsLimit"), window.creditsLimit},
          {QStringLiteral("usedFraction"), window.usedFraction},
          {QStringLiteral("resetsAt"), window.resetsAt}};
}

QVariantMap quotaToVariant(const ProviderQuota& quota) {
  QVariantList windows;
  windows.reserve(quota.windows.size());
  for (const auto& window : quota.windows) {
    windows.append(windowToVariant(window));
  }

  return {{QStringLiteral("id"), quota.id},
          {QStringLiteral("displayName"), quota.displayName},
          {QStringLiteral("enabled"), quota.enabled},
          {QStringLiteral("fetched"), quota.fetched},
          {QStringLiteral("hasError"), quota.hasError},
          {QStringLiteral("errorMsg"), quota.errorMsg},
          {QStringLiteral("primaryFraction"),
           qBound(0.0, quota.primaryFraction(), 1.0)},
          {QStringLiteral("planName"), quota.planName},
          {QStringLiteral("windows"), windows},
          {QStringLiteral("balanceCash"), quota.balance.cash},
          {QStringLiteral("balanceGift"), quota.balance.gift},
          {QStringLiteral("balanceTotal"), quota.balance.total},
          {QStringLiteral("balanceCurrency"), quota.balance.currency},
          {QStringLiteral("fetchedAt"), quota.fetchedAt}};
}

}  // namespace

UsageController::UsageController(ProviderManager* manager, QObject* parent)
    : QObject(parent), manager_(manager) {
  Q_ASSERT(manager_ != nullptr);
  connect(manager_, &ProviderManager::quotasReady, this,
          [this](const QList<ProviderQuota>& quotas) {
            QVariantList converted;
            converted.reserve(quotas.size());
            for (const auto& quota : quotas) {
              converted.append(quotaToVariant(quota));
            }

            providers_ = std::move(converted);
            emit providersChanged();

            if (!errorMessage_.isEmpty()) {
              errorMessage_.clear();
              emit errorChanged();
            }
            if (loading_) {
              loading_ = false;
              emit loadingChanged();
            }
          });
}

void UsageController::refresh() {
  if (loading_) {
    return;
  }
  loading_ = true;
  emit loadingChanged();
  manager_->refresh();
}

QVariantList UsageController::providers() const { return providers_; }

bool UsageController::loading() const { return loading_; }

QString UsageController::errorMessage() const { return errorMessage_; }
