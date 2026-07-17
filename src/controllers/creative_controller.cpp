#include "controllers/creative_controller.h"

#include <utility>

#include "services/creative_service.h"

namespace aegis {

CreativeController::CreativeController(CreativeService* service,
                                       QObject* parent)
    : QObject(parent), service_(service) {
  connect(service_, &CreativeService::chunk, this,
          [this](const QString& requestId, const QString& delta) {
            if (!outputBuffers_.contains(requestId)) return;
            outputBuffers_[requestId].append(delta);
            if (requestId != requestId_) return;
            output_ = outputBuffers_.value(requestId);
            outputBytes_ = static_cast<quint64>(output_.toUtf8().size());
            emit outputChanged();
            emit resultChanged();
            emit chunk(requestId, delta);
          });
  connect(service_, &CreativeService::finished, this,
          [this](const QString& requestId, const dto::CreativeResult& result) {
            if (!outputBuffers_.contains(requestId)) return;
            const auto finalText = outputBuffers_.take(requestId);
            if (requestId != requestId_) return;
            busy_ = false;
            output_ = finalText;
            finishReason_ = result.finishReason;
            outputBytes_ = static_cast<quint64>(finalText.toUtf8().size());
            emit busyChanged();
            emit outputChanged();
            emit resultChanged();
            emit finished(requestId, finalText, result.finishReason);
          });
  connect(service_, &CreativeService::failed, this,
          [this](const QString& requestId, const AegisError& error) {
            outputBuffers_.remove(requestId);
            if (requestId != requestId_) return;
            busy_ = false;
            finishReason_ = QStringLiteral("error");
            emit busyChanged();
            emit resultChanged();
            emit failed(requestId, error.userMessage);
            emit errorRaised(error.userMessage, error.retryable);
            emit toast(error.userMessage, 3);
          });
}

bool CreativeController::busy() const { return busy_; }

QString CreativeController::output() const { return output_; }

QString CreativeController::finishReason() const { return finishReason_; }

quint64 CreativeController::outputBytes() const { return outputBytes_; }

QString CreativeController::generate(int backend, QString model,
                                     QString prompt, double temperature,
                                     int maxTokens) {
  if (busy_) cancel();
  output_.clear();
  finishReason_.clear();
  outputBytes_ = 0;
  busy_ = true;
  emit outputChanged();
  emit resultChanged();
  emit busyChanged();
  requestId_ = service_->generate(
      {static_cast<dto::CreativeBackend>(backend), std::move(model),
       std::move(prompt), temperature, maxTokens});
  outputBuffers_.insert(requestId_, {});
  return requestId_;
}

void CreativeController::cancel() {
  if (!requestId_.isEmpty()) service_->cancel(requestId_);
}

}  // namespace aegis
