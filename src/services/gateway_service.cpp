#include "services/gateway_service.h"

#include <QFutureWatcher>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QTimer>
#include <QUuid>

#include "config/config_service.h"
#include "config/secret_store.h"
#include "core/async.h"
#include "core/http_client.h"

namespace aegis {
namespace {

constexpr quint64 kStreamCap = 16ULL * 1024ULL * 1024ULL;

AegisError streamNetworkError(QNetworkReply::NetworkError error) {
  if (error == QNetworkReply::TimeoutError) {
    return makeError(ErrorCode::NetworkTimeout,
                     QStringLiteral("gateway stream timed out"));
  }
  if (error == QNetworkReply::SslHandshakeFailedError) {
    return makeError(ErrorCode::NetworkTls,
                     QStringLiteral("gateway TLS handshake failed"));
  }
  if (error == QNetworkReply::OperationCanceledError) {
    return makeError(ErrorCode::Cancelled);
  }
  return makeError(ErrorCode::NetworkUnreachable,
                   QStringLiteral("gateway stream network failure"));
}

QString textDelta(const QJsonObject& object) {
  const auto directKeys = {QStringLiteral("delta"),
                           QStringLiteral("textDelta"),
                           QStringLiteral("response"),
                           QStringLiteral("text")};
  for (const auto& key : directKeys) {
    const auto value = object.value(key);
    if (value.isString()) return value.toString();
  }
  const auto choices = object.value(QStringLiteral("choices"));
  if (!choices.isArray() || choices.toArray().isEmpty()) return {};
  const auto choice = choices.toArray().first();
  if (!choice.isObject()) return {};
  const auto delta = choice.toObject().value(QStringLiteral("delta"));
  if (!delta.isObject()) return {};
  return delta.toObject().value(QStringLiteral("content")).toString();
}

}  // namespace

GatewayService::GatewayService(SecretStore* secrets, ConfigService* config,
                               HttpClient* http, QObject* parent)
    : QObject(parent), secrets_(secrets), config_(config), http_(http) {}

QFuture<Result<QJsonObject>> GatewayService::get(
    const QString& path, const QUrlQuery& query,
    std::chrono::milliseconds deadline) {
  setConnectionState(ConnectionState::Connecting);
  return async::flatten(this, secrets_->read(QStringLiteral("gateway.token"))
      .then(this, [this, path, query, deadline](const Result<QString>& token)
                       -> QFuture<Result<QJsonObject>> {
        if (!token) {
          setConnectionState(ConnectionState::Error);
          return async::run([error = token.error()] {
            return Result<QJsonObject>(tl::unexpected(
                error.code == ErrorCode::MissingToken
                    ? error
                    : makeError(ErrorCode::MissingToken,
                                QStringLiteral("gateway credential unavailable"))));
          });
        }
        const auto request = authorizedRequest(path, query, token.value());
        if (!request) {
          setConnectionState(ConnectionState::Error);
          return async::run([error = request.error()] {
            return Result<QJsonObject>(tl::unexpected(error));
          });
        }
        HttpRequestOptions options;
        const auto connectTimeout = config_->gatewayConnectTimeoutMs();
        const auto cap = config_->gatewayMaxResponseBytes();
        if (!connectTimeout || !cap || deadline.count() <= 0) {
          const auto error = !connectTimeout
                                 ? connectTimeout.error()
                                 : (!cap ? cap.error()
                                         : makeError(ErrorCode::ConfigInvalid));
          return async::run([error] {
            return Result<QJsonObject>(tl::unexpected(error));
          });
        }
        options.connectTimeoutMs = connectTimeout.value();
        options.totalTimeoutMs = static_cast<int>(std::min<qint64>(
            deadline.count(), std::numeric_limits<int>::max()));
        options.maxResponseBytes = cap.value();
        return http_->getJson(request.value(), options).then(
            this, [this](const Result<QJsonObject>& result) {
              const bool gatewayReachable =
                  result || result.error().code == ErrorCode::ResponseMalformed;
              setConnectionState(gatewayReachable ? ConnectionState::Connected
                                                   : ConnectionState::Error);
              return result;
            });
      }));
}

QFuture<Result<QJsonObject>> GatewayService::postJson(
    const QString& path, const QJsonObject& body,
    std::chrono::milliseconds deadline) {
  setConnectionState(ConnectionState::Connecting);
  return async::flatten(this, secrets_->read(QStringLiteral("gateway.token"))
      .then(this, [this, path, body, deadline](const Result<QString>& token)
                       -> QFuture<Result<QJsonObject>> {
        if (!token) {
          setConnectionState(ConnectionState::Error);
          return async::run([error = token.error()] {
            return Result<QJsonObject>(tl::unexpected(
                error.code == ErrorCode::MissingToken
                    ? error
                    : makeError(ErrorCode::MissingToken,
                                QStringLiteral("gateway credential unavailable"))));
          });
        }
        const auto request = authorizedRequest(path, {}, token.value());
        if (!request) {
          return async::run([error = request.error()] {
            return Result<QJsonObject>(tl::unexpected(error));
          });
        }
        const auto connectTimeout = config_->gatewayConnectTimeoutMs();
        const auto cap = config_->gatewayMaxResponseBytes();
        if (!connectTimeout || !cap || deadline.count() <= 0) {
          const auto error = !connectTimeout
                                 ? connectTimeout.error()
                                 : (!cap ? cap.error()
                                         : makeError(ErrorCode::ConfigInvalid));
          return async::run([error] {
            return Result<QJsonObject>(tl::unexpected(error));
          });
        }
        HttpRequestOptions options;
        options.connectTimeoutMs = connectTimeout.value();
        options.totalTimeoutMs = static_cast<int>(std::min<qint64>(
            deadline.count(), std::numeric_limits<int>::max()));
        options.maxResponseBytes = cap.value();
        return http_->request(HttpMethod::Post, request.value(),
                              QJsonDocument(body).toJson(QJsonDocument::Compact),
                              options)
            .then(this, [this](const Result<QByteArray>& result)
                            -> Result<QJsonObject> {
              if (!result) {
                setConnectionState(ConnectionState::Error);
                return tl::unexpected(result.error());
              }
              QJsonParseError error;
              const auto document = QJsonDocument::fromJson(result.value(), &error);
              if (error.error != QJsonParseError::NoError ||
                  !document.isObject()) {
                setConnectionState(ConnectionState::Error);
                return tl::unexpected(makeError(
                    ErrorCode::ResponseMalformed,
                    QStringLiteral("gateway JSON response invalid")));
              }
              setConnectionState(ConnectionState::Connected);
              return document.object();
            });
      }));
}

QString GatewayService::beginStream(const QString& path,
                                    const QJsonObject& body) {
  const auto requestId = QUuid::createUuid().toString(QUuid::WithoutBraces);
  setConnectionState(ConnectionState::Connecting);
  secrets_->read(QStringLiteral("gateway.token"))
      .then(this, [this, requestId, path, body](const Result<QString>& token) {
        if (cancelledStreams_.remove(requestId)) {
          emit streamFinished(requestId, makeError(ErrorCode::Cancelled));
          return;
        }
        if (!token) {
          setConnectionState(ConnectionState::Error);
          emit streamFinished(
              requestId,
              token.error().code == ErrorCode::MissingToken
                  ? token.error()
                  : makeError(ErrorCode::MissingToken,
                              QStringLiteral("gateway credential unavailable")));
          return;
        }
        const auto request = authorizedRequest(path, {}, token.value());
        const auto connectTimeout = config_->gatewayConnectTimeoutMs();
        if (!request || !connectTimeout) {
          setConnectionState(ConnectionState::Error);
          emit streamFinished(requestId,
                              !request ? request.error()
                                       : connectTimeout.error());
          return;
        }
        startStreamReply(requestId, request.value(), body,
                         connectTimeout.value());
      });
  return requestId;
}

void GatewayService::cancelStream(const QString& requestId) {
  if (requestId.isEmpty()) return;
  cancelledStreams_.insert(requestId);
  const auto reply = streamReplies_.value(requestId);
  if (reply) reply->abort();
}

void GatewayService::startStreamReply(const QString& requestId,
                                      QNetworkRequest request,
                                      const QJsonObject& body,
                                      int connectTimeoutMs) {
  auto streamBody = body;
  streamBody.insert(QStringLiteral("stream"), true);
  auto* reply = http_->postStream(
      request, QJsonDocument(streamBody).toJson(QJsonDocument::Compact));
  if (reply == nullptr) {
    setConnectionState(ConnectionState::Error);
    emit streamFinished(requestId, makeError(ErrorCode::ValidationFailed,
                                             QStringLiteral("invalid stream request")));
    return;
  }
  streamReplies_.insert(requestId, reply);
  streamLineBuffers_.insert(requestId, {});
  streamReceivedBytes_.insert(requestId, 0);

  auto* connectTimer = new QTimer(reply);
  connectTimer->setSingleShot(true);
  connectTimer->start(connectTimeoutMs);
  auto* idleTimer = new QTimer(reply);
  idleTimer->setSingleShot(true);
  auto* totalTimer = new QTimer(reply);
  totalTimer->setSingleShot(true);
  totalTimer->start(300000);

  const auto abortWith = [this, requestId, reply](AegisError error) {
    if (!streamForcedErrors_.contains(requestId)) {
      streamForcedErrors_.insert(requestId, std::move(error));
      reply->abort();
    }
  };
  connect(connectTimer, &QTimer::timeout, this, [abortWith] {
    abortWith(makeError(ErrorCode::NetworkTimeout,
                        QStringLiteral("gateway stream connect timeout")));
  });
  connect(idleTimer, &QTimer::timeout, this, [abortWith] {
    abortWith(makeError(ErrorCode::NetworkTimeout,
                        QStringLiteral("gateway stream idle timeout")));
  });
  connect(totalTimer, &QTimer::timeout, this, [abortWith] {
    abortWith(makeError(ErrorCode::NetworkTimeout,
                        QStringLiteral("gateway stream total timeout")));
  });
  connect(reply, &QNetworkReply::metaDataChanged, this,
          [reply, connectTimer, abortWith] {
            connectTimer->stop();
            const auto length =
                reply->header(QNetworkRequest::ContentLengthHeader);
            if (length.isValid() && length.toULongLong() > kStreamCap) {
              abortWith(makeError(
                  ErrorCode::ResponseTooLarge,
                  QStringLiteral("gateway stream content length exceeded cap")));
            }
          });
  connect(reply, &QIODevice::readyRead, this,
          [this, requestId, reply, connectTimer, idleTimer, abortWith] {
            connectTimer->stop();
            idleTimer->start(60000);
            const auto consumed = consumeStreamBytes(requestId, reply->readAll());
            if (!consumed) abortWith(consumed.error());
          });
  connect(reply, &QNetworkReply::finished, this,
          [this, requestId, reply, connectTimer, idleTimer, totalTimer] {
            connectTimer->stop();
            idleTimer->stop();
            totalTimer->stop();
            if (reply->bytesAvailable() > 0 &&
                !streamForcedErrors_.contains(requestId)) {
              const auto consumed = consumeStreamBytes(requestId, reply->readAll());
              if (!consumed) streamForcedErrors_.insert(requestId, consumed.error());
            }
            completeStream(requestId);
            reply->deleteLater();
          });
}

Result<void> GatewayService::consumeStreamBytes(const QString& requestId,
                                                const QByteArray& bytes) {
  const auto received = streamReceivedBytes_.value(requestId);
  const auto incoming = static_cast<quint64>(bytes.size());
  if (incoming > kStreamCap || received > kStreamCap - incoming) {
    return tl::unexpected(makeError(
        ErrorCode::ResponseTooLarge,
        QStringLiteral("gateway streamed response exceeded cap")));
  }
  streamReceivedBytes_[requestId] = received + incoming;
  auto& buffer = streamLineBuffers_[requestId];
  buffer.append(bytes);
  qsizetype newline = 0;
  while ((newline = buffer.indexOf('\n')) >= 0) {
    const auto line = buffer.left(newline);
    buffer.remove(0, newline + 1);
    const auto processed = processStreamLine(requestId, line);
    if (!processed) return processed;
  }
  return {};
}

Result<void> GatewayService::processStreamLine(const QString& requestId,
                                               const QByteArray& line) {
  auto frame = line.trimmed();
  if (frame.isEmpty()) return {};
  if (frame.startsWith("data:")) frame = frame.mid(5).trimmed();
  if (frame == QByteArrayLiteral("[DONE]")) return {};
  QJsonParseError parseError;
  const auto document = QJsonDocument::fromJson(frame, &parseError);
  if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
    return tl::unexpected(makeError(
        ErrorCode::ResponseMalformed,
        QStringLiteral("gateway stream frame is not a JSON object")));
  }
  const auto object = document.object();
  if (object.value(QStringLiteral("error")).isString()) {
    return tl::unexpected(makeError(ErrorCode::HttpStatus,
                                    QStringLiteral("gateway stream error frame")));
  }
  const auto delta = textDelta(object);
  const auto done = object.value(QStringLiteral("done")).toBool(false);
  if (delta.isEmpty() && !done) {
    return tl::unexpected(makeError(
        ErrorCode::ResponseMalformed,
        QStringLiteral("gateway stream frame has no text delta")));
  }
  if (!delta.isEmpty()) emit streamChunk(requestId, delta.toUtf8());
  return {};
}

void GatewayService::completeStream(const QString& requestId) {
  const auto reply = streamReplies_.take(requestId);
  auto remaining = streamLineBuffers_.take(requestId);
  streamReceivedBytes_.remove(requestId);

  if (cancelledStreams_.remove(requestId)) {
    streamForcedErrors_.remove(requestId);
    emit streamFinished(requestId, makeError(ErrorCode::Cancelled));
    return;
  }
  if (!remaining.trimmed().isEmpty() &&
      !streamForcedErrors_.contains(requestId)) {
    const auto processed = processStreamLine(requestId, remaining);
    if (!processed) streamForcedErrors_.insert(requestId, processed.error());
  }
  if (streamForcedErrors_.contains(requestId)) {
    setConnectionState(ConnectionState::Error);
    emit streamFinished(requestId, streamForcedErrors_.take(requestId));
    return;
  }
  if (reply && reply->error() != QNetworkReply::NoError) {
    setConnectionState(ConnectionState::Error);
    emit streamFinished(requestId, streamNetworkError(reply->error()));
    return;
  }
  const auto status = reply
                          ? reply->attribute(QNetworkRequest::HttpStatusCodeAttribute)
                                .toInt()
                          : 0;
  if (status < 200 || status >= 300) {
    setConnectionState(ConnectionState::Error);
    emit streamFinished(
        requestId,
        makeError(ErrorCode::HttpStatus,
                  QStringLiteral("gateway stream HTTP status %1").arg(status)));
    return;
  }
  setConnectionState(ConnectionState::Connected);
  emit streamFinished(requestId, makeError(ErrorCode::Ok));
}

ConnectionState GatewayService::connectionState() const {
  return connectionState_;
}

Result<QNetworkRequest> GatewayService::authorizedRequest(
    const QString& path, const QUrlQuery& query, const QString& token) const {
  if (token.isEmpty()) {
    return tl::unexpected(makeError(ErrorCode::MissingToken));
  }
  if (path.contains(QChar::Null) || path.contains(QStringLiteral("://")) ||
      path.size() > 4096) {
    return tl::unexpected(makeError(ErrorCode::ValidationFailed,
                                    QStringLiteral("invalid gateway path")));
  }
  const auto base = config_->gatewayBaseUrl();
  if (!base) return tl::unexpected(base.error());
  auto url = base.value();
  auto basePath = url.path();
  if (!basePath.endsWith(QLatin1Char('/'))) basePath.append(QLatin1Char('/'));
  auto relative = path;
  while (relative.startsWith(QLatin1Char('/'))) relative.remove(0, 1);
  url.setPath(QDir::cleanPath(basePath + relative));
  url.setQuery(query);
  QNetworkRequest request(url);
  request.setRawHeader(QByteArrayLiteral("Authorization"),
                       QByteArrayLiteral("Bearer ") + token.toUtf8());
  request.setHeader(QNetworkRequest::ContentTypeHeader,
                    QStringLiteral("application/json"));
  return request;
}

void GatewayService::setConnectionState(ConnectionState state) {
  if (connectionState_ == state) return;
  connectionState_ = state;
  emit connectionStateChanged(state);
}

}  // namespace aegis
