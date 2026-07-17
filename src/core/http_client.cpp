#include "core/http_client.h"

#include <memory>
#include <optional>
#include <utility>

#include <QJsonDocument>
#include <QJsonParseError>
#include <QNetworkReply>
#include <QPointer>
#include <QPromise>
#include <QTimer>

namespace aegis {
namespace {

using ByteResult = Result<QByteArray>;

QFuture<ByteResult> readyError(const AegisError& error) {
  QPromise<ByteResult> promise;
  promise.start();
  promise.addResult(tl::unexpected(error));
  promise.finish();
  return promise.future();
}

AegisError mapNetworkError(QNetworkReply::NetworkError error,
                           const QString& detail) {
  switch (error) {
    case QNetworkReply::TimeoutError:
      return makeError(ErrorCode::NetworkTimeout, detail);
    case QNetworkReply::SslHandshakeFailedError:
      return makeError(ErrorCode::NetworkTls, detail);
    case QNetworkReply::ConnectionRefusedError:
    case QNetworkReply::RemoteHostClosedError:
    case QNetworkReply::HostNotFoundError:
    case QNetworkReply::TemporaryNetworkFailureError:
    case QNetworkReply::NetworkSessionFailedError:
      return makeError(ErrorCode::NetworkUnreachable, detail);
    case QNetworkReply::OperationCanceledError:
      return makeError(ErrorCode::Cancelled, detail);
    default:
      return makeError(ErrorCode::NetworkUnreachable, detail);
  }
}

class RequestOperation : public std::enable_shared_from_this<RequestOperation> {
 public:
  RequestOperation(QNetworkAccessManager* manager, HttpMethod method,
                   QNetworkRequest request, QByteArray body,
                   HttpRequestOptions options)
      : manager_(manager),
        method_(method),
        request_(std::move(request)),
        body_(std::move(body)),
        options_(options),
        promise_(std::make_unique<QPromise<ByteResult>>()) {
    promise_->start();
  }

  QFuture<ByteResult> future() { return promise_->future(); }

  void start() { startAttempt(); }

 private:
  void startAttempt() {
    if (manager_.isNull()) {
      complete(tl::unexpected(makeError(
          ErrorCode::NetworkUnreachable,
          QStringLiteral("network manager was destroyed"))));
      return;
    }

    ++attemptNumber_;
    buffer_.clear();
    forcedError_.reset();

    switch (method_) {
      case HttpMethod::Get:
        reply_ = manager_->get(request_);
        break;
      case HttpMethod::Post:
        reply_ = manager_->post(request_, body_);
        break;
      case HttpMethod::Put:
        reply_ = manager_->put(request_, body_);
        break;
      case HttpMethod::Delete:
        reply_ = manager_->sendCustomRequest(request_, QByteArrayLiteral("DELETE"),
                                             body_);
        break;
    }

    connectTimer_ = std::make_unique<QTimer>();
    connectTimer_->setSingleShot(true);
    totalTimer_ = std::make_unique<QTimer>();
    totalTimer_->setSingleShot(true);

    const auto self = shared_from_this();
    QObject::connect(connectTimer_.get(), &QTimer::timeout, reply_, [self]() {
      self->abortWith(makeError(ErrorCode::NetworkTimeout,
                                QStringLiteral("connect timeout")));
    });
    QObject::connect(totalTimer_.get(), &QTimer::timeout, reply_, [self]() {
      self->abortWith(makeError(ErrorCode::NetworkTimeout,
                                QStringLiteral("total timeout")));
    });
    QObject::connect(reply_, &QNetworkReply::metaDataChanged, reply_, [self]() {
      self->connectTimer_->stop();
      const auto length = self->reply_->header(QNetworkRequest::ContentLengthHeader);
      if (length.isValid() && length.toULongLong() > self->options_.maxResponseBytes) {
        self->abortWith(makeError(ErrorCode::ResponseTooLarge,
                                  QStringLiteral("content length exceeded cap")));
      }
    });
    QObject::connect(reply_, &QIODevice::readyRead, reply_, [self]() {
      self->connectTimer_->stop();
      self->consumeAvailable();
    });
    QObject::connect(reply_, &QNetworkReply::finished, reply_, [self]() {
      self->handleFinished();
    });

    connectTimer_->start(options_.connectTimeoutMs);
    totalTimer_->start(options_.totalTimeoutMs);
  }

  void consumeAvailable() {
    if (reply_.isNull() || forcedError_.has_value()) {
      return;
    }
    const auto chunk = reply_->readAll();
    const auto currentSize = static_cast<quint64>(buffer_.size());
    const auto chunkSize = static_cast<quint64>(chunk.size());
    if (chunkSize > options_.maxResponseBytes ||
        currentSize > options_.maxResponseBytes - chunkSize) {
      abortWith(makeError(ErrorCode::ResponseTooLarge,
                          QStringLiteral("streamed response exceeded cap")));
      return;
    }
    buffer_.append(chunk);
  }

  void abortWith(AegisError error) {
    if (forcedError_.has_value() || reply_.isNull()) {
      return;
    }
    forcedError_ = std::move(error);
    reply_->abort();
  }

  void handleFinished() {
    connectTimer_->stop();
    totalTimer_->stop();
    consumeAvailable();

    ByteResult result = buffer_;
    int status = 0;
    if (!reply_.isNull()) {
      status = reply_->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    }

    if (forcedError_.has_value()) {
      result = tl::unexpected(*forcedError_);
    } else if (status > 0 && (status < 200 || status >= 300)) {
      result = tl::unexpected(makeError(
          ErrorCode::HttpStatus,
          QStringLiteral("HTTP status %1").arg(status)));
    } else if (!reply_.isNull() && reply_->error() != QNetworkReply::NoError) {
      result = tl::unexpected(mapNetworkError(
          reply_->error(), QStringLiteral("network reply error %1")
                               .arg(static_cast<int>(reply_->error()))));
    }

    const auto retryableStatus = status >= 500 && status <= 599;
    const auto canRetry = !result && attemptNumber_ <= options_.maxRetries &&
                          (result.error().code == ErrorCode::NetworkTimeout ||
                           result.error().code == ErrorCode::NetworkUnreachable ||
                           retryableStatus);

    if (!reply_.isNull()) {
      reply_->deleteLater();
      reply_.clear();
    }
    connectTimer_.reset();
    totalTimer_.reset();

    if (canRetry && !manager_.isNull()) {
      const auto self = shared_from_this();
      QTimer::singleShot(options_.retryDelayMs, manager_,
                         [self]() { self->startAttempt(); });
      return;
    }
    complete(std::move(result));
  }

  void complete(ByteResult result) {
    if (completed_) {
      return;
    }
    completed_ = true;
    promise_->addResult(std::move(result));
    promise_->finish();
  }

  QPointer<QNetworkAccessManager> manager_;
  HttpMethod method_;
  QNetworkRequest request_;
  QByteArray body_;
  HttpRequestOptions options_;
  std::unique_ptr<QPromise<ByteResult>> promise_;
  QPointer<QNetworkReply> reply_;
  std::unique_ptr<QTimer> connectTimer_;
  std::unique_ptr<QTimer> totalTimer_;
  QByteArray buffer_;
  std::optional<AegisError> forcedError_;
  int attemptNumber_ = 0;
  bool completed_ = false;
};

bool validOptions(const HttpRequestOptions& options) {
  return options.connectTimeoutMs > 0 && options.totalTimeoutMs > 0 &&
         options.maxResponseBytes > 0 && options.maxRetries >= 0 &&
         options.retryDelayMs >= 0;
}

}  // namespace

HttpClient::HttpClient(QObject* parent) : QObject(parent), manager_(this) {}

QFuture<Result<QByteArray>> HttpClient::get(
    const QNetworkRequest& request, const HttpRequestOptions& options) {
  return this->request(HttpMethod::Get, request, {}, options);
}

QFuture<Result<QByteArray>> HttpClient::request(
    HttpMethod method, const QNetworkRequest& request, const QByteArray& body,
    const HttpRequestOptions& options) {
  if (!request.url().isValid() || request.url().scheme().isEmpty()) {
    return readyError(makeError(ErrorCode::ValidationFailed,
                                QStringLiteral("invalid request URL")));
  }
  if (!validOptions(options)) {
    return readyError(makeError(ErrorCode::ConfigInvalid,
                                QStringLiteral("invalid HTTP safety options")));
  }
  auto operation = std::make_shared<RequestOperation>(
      &manager_, method, request, body, options);
  auto future = operation->future();
  operation->start();
  return future;
}

QFuture<Result<QJsonObject>> HttpClient::getJson(
    const QNetworkRequest& request, const HttpRequestOptions& options) {
  return get(request, options).then(this, [](const Result<QByteArray>& result) {
    if (!result) {
      return Result<QJsonObject>(tl::unexpected(result.error()));
    }
    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(result.value(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
      return Result<QJsonObject>(tl::unexpected(makeError(
          ErrorCode::ResponseMalformed,
          QStringLiteral("JSON object parse failed"))));
    }
    return Result<QJsonObject>(document.object());
  });
}

}  // namespace aegis
