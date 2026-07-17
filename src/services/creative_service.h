#pragma once

#include <QHash>
#include <QObject>
#include <QPointer>
#include <QSet>
#include <QString>

#include "core/result.h"
#include "dto/creative_dto.h"

class QNetworkReply;

namespace aegis {

class ConfigService;
class GatewayService;
class HttpClient;

class CreativeService : public QObject {
  Q_OBJECT

 public:
  // Creates cancellable Ollama and gateway creative backends.
  explicit CreativeService(ConfigService* config, HttpClient* http,
                           GatewayService* gateway,
                           QObject* parent = nullptr);
  // Validates and starts a generation request.
  [[nodiscard]] QString generate(dto::CreativeRequest request);
  // Cancels one active request.
  void cancel(QString requestId);

 signals:
  void chunk(QString requestId, QString textDelta);
  void finished(QString requestId, dto::CreativeResult result);
  void failed(QString requestId, AegisError error);

 private:
  void generateOllama(const QString& requestId,
                      const dto::CreativeRequest& request);
  Result<void> consumeOllamaBytes(const QString& requestId,
                                  const QByteArray& bytes);
  Result<void> processOllamaLine(const QString& requestId,
                                 const QByteArray& line);
  void completeOllama(const QString& requestId);
  void finishCancelled(const QString& requestId);

  ConfigService* config_;
  HttpClient* http_;
  GatewayService* gateway_;
  QHash<QString, QString> gatewayRequestIds_;
  QHash<QString, QPointer<QNetworkReply>> ollamaReplies_;
  QHash<QString, QByteArray> ollamaLineBuffers_;
  QHash<QString, quint64> ollamaReceivedBytes_;
  QHash<QString, AegisError> ollamaForcedErrors_;
  QSet<QString> cancelledRequests_;
  QHash<QString, dto::CreativeResult> results_;
};

}  // namespace aegis
