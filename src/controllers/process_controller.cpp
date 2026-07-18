#include "controllers/process_controller.h"

#include "services/process_service.h"

namespace aegis {

ProcessController::ProcessController(ProcessService* service, QObject* parent)
    : QObject(parent), service_(service), processes_(this) {
  connect(service_, &ProcessService::sampled, this,
          [this](const dto::ProcessDto& sample) {
            processes_.setItems(sample.processes);
            setLoading(false);
          });
  connect(service_, &ProcessService::failed, this,
          [this](const AegisError& error) {
            setLoading(false);
            emit errorRaised(error.userMessage, error.retryable);
            emit toast(error.userMessage, 3);
          });
}

ProcessListModel* ProcessController::processes() { return &processes_; }

bool ProcessController::loading() const { return loading_; }

void ProcessController::refresh() {
  if (loading_) return;
  setLoading(true);
  service_->refresh();
}

void ProcessController::setLoading(bool loading) {
  if (loading_ == loading) return;
  loading_ = loading;
  emit loadingChanged();
}

}  // namespace aegis
