#include "controllers/container_controller.h"

#include "services/container_service.h"

namespace aegis {

ContainerController::ContainerController(ContainerService* service,
                                         QObject* parent)
    : QObject(parent), service_(service), containers_(this) {
  connect(service_, &ContainerService::sampled, this,
          [this](const dto::ContainerDto& sample) {
            containers_.setItems(sample.containers);
            setLoading(false);
            if (!available_) {
              available_ = true;
              emit availableChanged();
            }
          });
  connect(service_, &ContainerService::failed, this,
          [this](const AegisError& error) {
            setLoading(false);
            if (!available_) {
              available_ = true;
              emit availableChanged();
            }
            emit errorRaised(error.userMessage, error.retryable);
            emit toast(error.userMessage, 3);
          });
}

ContainerListModel* ContainerController::containers() { return &containers_; }

bool ContainerController::loading() const { return loading_; }

bool ContainerController::available() const { return available_; }

void ContainerController::refresh() {
  if (loading_) return;
  setLoading(true);
  service_->refresh();
}

void ContainerController::setLoading(bool loading) {
  if (loading_ == loading) return;
  loading_ = loading;
  emit loadingChanged();
}

}  // namespace aegis
