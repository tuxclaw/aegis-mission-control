#include "services/gateway_service.h"

#include <QFutureWatcher>
#include <QDir>
#include <QJsonDocument>
#include <QNetworkRequest>
#include <QUuid>

#include "config/config_service.h"
#include "config/secret_store.h"
#include "core/async.h"
#include "core/http_client.h"

namespace aegis {

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
              setConnectionState(result ? ConnectionState::Connected
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
  auto* watcher = new QFutureWatcher<Result<QJsonObject>>(this);
  connect(watcher, &QFutureWatcher<Result<QJsonObject>>::finished, this,
          [this, watcher, requestId] {
            const auto result = watcher->result();
            watcher->deleteLater();
            if (cancelledStreams_.remove(requestId)) {
              emit streamFinished(requestId,
                                  makeError(ErrorCode::Cancelled));
              return;
            }
            if (!result) {
              emit streamFinished(requestId, result.error());
              return;
            }
            emit streamChunk(
                requestId,
                QJsonDocument(result.value()).toJson(QJsonDocument::Compact));
            emit streamFinished(requestId, makeError(ErrorCode::Ok));
          });
  watcher->setFuture(postJson(path, body, std::chrono::minutes(5)));
  return requestId;
}

void GatewayService::cancelStream(const QString& requestId) {
  if (!requestId.isEmpty()) cancelledStreams_.insert(requestId);
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
