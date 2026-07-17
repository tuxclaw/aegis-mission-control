#include "services/creative_service.h"

#include <utility>

#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUuid>

#include "config/config_service.h"
#include "core/http_client.h"
#include "services/gateway_service.h"

namespace aegis {
namespace {

constexpr quint64 kStreamCap = 16ULL * 1024ULL * 1024ULL;

AegisError networkError(QNetworkReply::NetworkError error) {
  if (error == QNetworkReply::TimeoutError) {
    return makeError(ErrorCode::NetworkTimeout,
                     QStringLiteral("Ollama stream timed out"));
  }
  if (error == QNetworkReply::SslHandshakeFailedError) {
    return makeError(ErrorCode::NetworkTls,
                     QStringLiteral("Ollama TLS handshake failed"));
  }
  if (error == QNetworkReply::OperationCanceledError) {
    return makeError(ErrorCode::Cancelled);
  }
  return makeError(ErrorCode::NetworkUnreachable,
                   QStringLiteral("Ollama network request failed"));
}

}  // namespace

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
  QMetaObject::invokeMethod(
      this,
      [this, requestId, request = std::move(request)] {
        if (!results_.contains(requestId)) return;
        if (cancelledRequests_.remove(requestId)) {
          finishCancelled(requestId);
          return;
        }
        if (request.backend == dto::CreativeBackend::GatewayCreative) {
          const auto gatewayId = gateway_->beginStream(
              QStringLiteral("creative/generate"), request.toJson());
          gatewayRequestIds_.insert(requestId, gatewayId);
        } else {
          generateOllama(requestId, request);
        }
      },
      Qt::QueuedConnection);
  return requestId;
}

void CreativeService::cancel(QString requestId) {
  if (!results_.contains(requestId)) return;
  if (gatewayRequestIds_.contains(requestId)) {
    gateway_->cancelStream(gatewayRequestIds_.value(requestId));
    return;
  }
  cancelledRequests_.insert(requestId);
  const auto reply = ollamaReplies_.value(requestId);
  if (reply) reply->abort();
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
  auto* reply = http_->postStream(
      networkRequest, QJsonDocument(body).toJson(QJsonDocument::Compact));
  if (reply == nullptr) {
    results_.remove(requestId);
    emit failed(requestId, makeError(ErrorCode::ValidationFailed,
                                     QStringLiteral("invalid Ollama request")));
    return;
  }

  ollamaReplies_.insert(requestId, reply);
  ollamaLineBuffers_.insert(requestId, {});
  ollamaReceivedBytes_.insert(requestId, 0);

  auto* connectTimer = new QTimer(reply);
  connectTimer->setSingleShot(true);
  connectTimer->start(5000);
  auto* idleTimer = new QTimer(reply);
  idleTimer->setSingleShot(true);
  auto* totalTimer = new QTimer(reply);
  totalTimer->setSingleShot(true);
  totalTimer->start(300000);

  const auto abortWith = [this, requestId, reply](AegisError error) {
    if (!ollamaForcedErrors_.contains(requestId)) {
      ollamaForcedErrors_.insert(requestId, std::move(error));
      reply->abort();
    }
  };
  connect(connectTimer, &QTimer::timeout, this, [abortWith] {
    abortWith(makeError(ErrorCode::NetworkTimeout,
                        QStringLiteral("Ollama connect timeout")));
  });
  connect(idleTimer, &QTimer::timeout, this, [abortWith] {
    abortWith(makeError(ErrorCode::NetworkTimeout,
                        QStringLiteral("Ollama idle timeout")));
  });
  connect(totalTimer, &QTimer::timeout, this, [abortWith] {
    abortWith(makeError(ErrorCode::NetworkTimeout,
                        QStringLiteral("Ollama total timeout")));
  });
  connect(reply, &QNetworkReply::metaDataChanged, this,
          [this, requestId, reply, connectTimer, abortWith] {
            connectTimer->stop();
            const auto length =
                reply->header(QNetworkRequest::ContentLengthHeader);
            if (length.isValid() && length.toULongLong() > kStreamCap) {
              abortWith(makeError(
                  ErrorCode::ResponseTooLarge,
                  QStringLiteral("Ollama content length exceeded cap")));
            }
          });
  connect(reply, &QIODevice::readyRead, this,
          [this, requestId, reply, connectTimer, idleTimer, abortWith] {
            connectTimer->stop();
            idleTimer->start(60000);
            const auto consumed = consumeOllamaBytes(requestId, reply->readAll());
            if (!consumed) abortWith(consumed.error());
          });
  connect(reply, &QNetworkReply::finished, this,
          [this, requestId, reply, connectTimer, idleTimer, totalTimer] {
            connectTimer->stop();
            idleTimer->stop();
            totalTimer->stop();
            if (reply->bytesAvailable() > 0 &&
                !ollamaForcedErrors_.contains(requestId)) {
              const auto consumed = consumeOllamaBytes(requestId, reply->readAll());
              if (!consumed) ollamaForcedErrors_.insert(requestId, consumed.error());
            }
            completeOllama(requestId);
            reply->deleteLater();
          });
}

Result<void> CreativeService::consumeOllamaBytes(const QString& requestId,
                                                 const QByteArray& bytes) {
  if (!results_.contains(requestId)) return {};
  const auto received = ollamaReceivedBytes_.value(requestId);
  const auto incoming = static_cast<quint64>(bytes.size());
  if (incoming > kStreamCap || received > kStreamCap - incoming) {
    return tl::unexpected(makeError(
        ErrorCode::ResponseTooLarge,
        QStringLiteral("Ollama streamed response exceeded cap")));
  }
  ollamaReceivedBytes_[requestId] = received + incoming;
  auto& buffer = ollamaLineBuffers_[requestId];
  buffer.append(bytes);
  qsizetype newline = 0;
  while ((newline = buffer.indexOf('\n')) >= 0) {
    const auto line = buffer.left(newline);
    buffer.remove(0, newline + 1);
    const auto processed = processOllamaLine(requestId, line);
    if (!processed) return processed;
  }
  return {};
}

Result<void> CreativeService::processOllamaLine(const QString& requestId,
                                                const QByteArray& line) {
  if (line.trimmed().isEmpty()) return {};
  QJsonParseError parseError;
  const auto document = QJsonDocument::fromJson(line, &parseError);
  if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
    return tl::unexpected(makeError(ErrorCode::ResponseMalformed,
                                    QStringLiteral("Ollama stream line invalid")));
  }
  const auto object = document.object();
  const auto response = object.value(QStringLiteral("response"));
  const auto done = object.value(QStringLiteral("done"));
  if ((!response.isUndefined() && !response.isString()) ||
      (!done.isUndefined() && !done.isBool())) {
    return tl::unexpected(makeError(
        ErrorCode::ResponseMalformed,
        QStringLiteral("Ollama stream frame has invalid fields")));
  }
  const auto delta = response.toString();
  if (!delta.isEmpty() && results_.contains(requestId)) {
    auto& result = results_[requestId];
    result.text.append(delta);
    result.outputBytes += static_cast<quint64>(delta.toUtf8().size());
    emit chunk(requestId, delta);
  }
  if (done.toBool() && results_.contains(requestId)) {
    results_[requestId].finishReason =
        object.value(QStringLiteral("done_reason"))
            .toString(QStringLiteral("stop"));
  }
  return {};
}

void CreativeService::completeOllama(const QString& requestId) {
  const auto reply = ollamaReplies_.take(requestId);
  auto remaining = ollamaLineBuffers_.take(requestId);
  ollamaReceivedBytes_.remove(requestId);

  if (cancelledRequests_.remove(requestId)) {
    ollamaForcedErrors_.remove(requestId);
    finishCancelled(requestId);
    return;
  }
  if (!results_.contains(requestId)) {
    ollamaForcedErrors_.remove(requestId);
    return;
  }
  if (!remaining.trimmed().isEmpty() && !ollamaForcedErrors_.contains(requestId)) {
    const auto processed = processOllamaLine(requestId, remaining);
    if (!processed) ollamaForcedErrors_.insert(requestId, processed.error());
  }

  if (ollamaForcedErrors_.contains(requestId)) {
    const auto error = ollamaForcedErrors_.take(requestId);
    results_.remove(requestId);
    emit failed(requestId, error);
    return;
  }
  if (reply && reply->error() != QNetworkReply::NoError) {
    results_.remove(requestId);
    emit failed(requestId, networkError(reply->error()));
    return;
  }
  const auto status = reply
                          ? reply->attribute(QNetworkRequest::HttpStatusCodeAttribute)
                                .toInt()
                          : 0;
  if (status < 200 || status >= 300) {
    results_.remove(requestId);
    emit failed(requestId,
                makeError(ErrorCode::HttpStatus,
                          QStringLiteral("Ollama HTTP status %1").arg(status)));
    return;
  }
  auto finalResult = results_.take(requestId);
  finalResult.done = true;
  if (finalResult.finishReason.isEmpty()) {
    finalResult.finishReason = QStringLiteral("stop");
  }
  emit finished(requestId, finalResult);
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
