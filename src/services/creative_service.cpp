#include "services/creative_service.h"

#include <QFutureWatcher>
#include <QDir>
#include <QJsonDocument>
#include <QNetworkRequest>
#include <QUuid>

#include "config/config_service.h"
#include "core/http_client.h"
#include "services/gateway_service.h"

namespace aegis {

CreativeService::CreativeService(ConfigService* config, HttpClient* http,
                                 GatewayService* gateway, QObject* parent)
    : QObject(parent), config_(config), http_(http), gateway_(gateway) {
  connect(gateway_, &GatewayService::streamChunk, this,
          [this](const QString& gatewayId, const QByteArray& bytes) {
            const auto requestId = gatewayRequestIds_.key(gatewayId);
            if (requestId.isEmpty() || !results_.contains(requestId)) return;
            const auto delta = QString::fromUtf8(bytes);
            auto& result = results_[requestId];
            result.text.append(delta);
            result.outputBytes += static_cast<quint64>(bytes.size());
            emit chunk(requestId, delta);
          });
  connect(gateway_, &GatewayService::streamFinished, this,
          [this](const QString& gatewayId, const AegisError& error) {
            const auto requestId = gatewayRequestIds_.key(gatewayId);
            if (requestId.isEmpty() || !results_.contains(requestId)) return;
            gatewayRequestIds_.remove(requestId);
            if (error.code == ErrorCode::Cancelled) {
              finishCancelled(requestId);
            } else if (error.code != ErrorCode::Ok) {
              results_.remove(requestId);
              emit failed(requestId, error);
            } else {
              auto result = results_.take(requestId);
              result.done = true;
              result.finishReason = QStringLiteral("stop");
              emit finished(requestId, result);
            }
          });
}

QString CreativeService::generate(dto::CreativeRequest request) {
  const auto requestId = QUuid::createUuid().toString(QUuid::WithoutBraces);
  const auto valid = dto::CreativeRequest::fromJson(request.toJson());
  if (!valid) {
    QMetaObject::invokeMethod(
        this, [this, requestId, error = valid.error()] {
          emit failed(requestId, error);
        }, Qt::QueuedConnection);
    return requestId;
  }
  request = valid.value();
  results_.insert(requestId,
                  dto::CreativeResult{requestId, {}, false, {}, 0});
  if (request.backend == dto::CreativeBackend::GatewayCreative) {
    const auto gatewayId = gateway_->beginStream(
        QStringLiteral("creative/generate"), request.toJson());
    gatewayRequestIds_.insert(requestId, gatewayId);
  } else {
    generateOllama(requestId, request);
  }
  return requestId;
}

void CreativeService::cancel(QString requestId) {
  if (!results_.contains(requestId)) return;
  if (gatewayRequestIds_.contains(requestId)) {
    gateway_->cancelStream(gatewayRequestIds_.value(requestId));
  } else {
    finishCancelled(requestId);
  }
}

void CreativeService::generateOllama(
    const QString& requestId, const dto::CreativeRequest& request) {
  const auto base = config_->ollamaBaseUrl();
  if (!base) {
    results_.remove(requestId);
    emit failed(requestId, base.error());
    return;
  }
  auto url = base.value();
  url.setPath(QDir::cleanPath(url.path() + QStringLiteral("/api/generate")));
  QNetworkRequest networkRequest(url);
  networkRequest.setHeader(QNetworkRequest::ContentTypeHeader,
                           QStringLiteral("application/json"));
  const QJsonObject body{{QStringLiteral("model"), request.model},
                         {QStringLiteral("prompt"), request.prompt},
                         {QStringLiteral("stream"), true},
                         {QStringLiteral("options"),
                          QJsonObject{{QStringLiteral("temperature"),
                                       request.temperature},
                                      {QStringLiteral("num_predict"),
                                       request.maxTokens}}}};
  HttpRequestOptions options;
  options.totalTimeoutMs = 300000;
  options.maxResponseBytes = 16ULL * 1024ULL * 1024ULL;
  auto* watcher = new QFutureWatcher<Result<QByteArray>>(this);
  connect(watcher, &QFutureWatcher<Result<QByteArray>>::finished, this,
          [this, watcher, requestId] {
            const auto response = watcher->result();
            watcher->deleteLater();
            if (!results_.contains(requestId)) return;
            if (!response) {
              results_.remove(requestId);
              emit failed(requestId, response.error());
              return;
            }
            auto& result = results_[requestId];
            for (const auto& line : response->split('\n')) {
              if (line.trimmed().isEmpty()) continue;
              QJsonParseError parseError;
              const auto document = QJsonDocument::fromJson(line, &parseError);
              if (parseError.error != QJsonParseError::NoError ||
                  !document.isObject()) {
                results_.remove(requestId);
                emit failed(requestId, makeError(
                    ErrorCode::ResponseMalformed,
                    QStringLiteral("Ollama stream line invalid")));
                return;
              }
              const auto delta = document.object()
                                     .value(QStringLiteral("response"))
                                     .toString();
              if (!delta.isEmpty()) {
                result.text.append(delta);
                result.outputBytes += static_cast<quint64>(delta.toUtf8().size());
                emit chunk(requestId, delta);
              }
              if (document.object().value(QStringLiteral("done")).toBool()) {
                result.finishReason = document.object()
                                          .value(QStringLiteral("done_reason"))
                                          .toString(QStringLiteral("stop"));
              }
            }
            auto finalResult = results_.take(requestId);
            finalResult.done = true;
            if (finalResult.finishReason.isEmpty()) {
              finalResult.finishReason = QStringLiteral("stop");
            }
            emit finished(requestId, finalResult);
          });
  watcher->setFuture(http_->request(
      HttpMethod::Post, networkRequest,
      QJsonDocument(body).toJson(QJsonDocument::Compact), options));
}

void CreativeService::finishCancelled(const QString& requestId) {
  if (!results_.contains(requestId)) return;
  gatewayRequestIds_.remove(requestId);
  auto result = results_.take(requestId);
  result.done = true;
  result.finishReason = QStringLiteral("cancelled");
  emit finished(requestId, result);
}

}  // namespace aegis
