#pragma once

#include <chrono>

#include <QFuture>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QObject>
#include <QSet>
#include <QUrlQuery>

#include "core/result.h"

namespace aegis {

class ConfigService;
class HttpClient;
class SecretStore;

enum class ConnectionState { Disconnected, Connecting, Connected, Error };

class GatewayService : public QObject {
  Q_OBJECT

 public:
  // Creates the sole gateway authorization boundary.
  explicit GatewayService(SecretStore* secrets, ConfigService* config,
                          HttpClient* http, QObject* parent = nullptr);
  // Sends an authorized JSON GET, failing closed without a token.
  [[nodiscard]] QFuture<Result<QJsonObject>> get(
      const QString& path, const QUrlQuery& query = {},
      std::chrono::milliseconds deadline = std::chrono::milliseconds(30000));
  // Sends an authorized JSON POST, failing closed without a token.
  [[nodiscard]] QFuture<Result<QJsonObject>> postJson(
      const QString& path, const QJsonObject& body,
      std::chrono::milliseconds deadline = std::chrono::milliseconds(30000));
  // Starts a cancellable authorized creative response stream.
  [[nodiscard]] QString beginStream(const QString& path,
                                    const QJsonObject& body);
  // Cancels a stream by correlation id.
  void cancelStream(const QString& requestId);
  // Returns the last observed gateway connection state.
  [[nodiscard]] ConnectionState connectionState() const;

 signals:
  void connectionStateChanged(ConnectionState state);
  void streamChunk(QString requestId, QByteArray chunk);
  void streamFinished(QString requestId, AegisError errorOrOk);

 private:
  Result<QNetworkRequest> authorizedRequest(const QString& path,
                                             const QUrlQuery& query,
                                             const QString& token) const;
  void setConnectionState(ConnectionState state);

  SecretStore* secrets_;
  ConfigService* config_;
  HttpClient* http_;
  ConnectionState connectionState_ = ConnectionState::Disconnected;
  QSet<QString> cancelledStreams_;
};

}  // namespace aegis

Q_DECLARE_METATYPE(aegis::ConnectionState)
