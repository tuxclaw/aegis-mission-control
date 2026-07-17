#pragma once

#include <QByteArray>
#include <QFuture>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QObject>

#include "core/result.h"

namespace aegis {

enum class HttpMethod { Get, Post, Put, Delete };

struct HttpRequestOptions {
  int connectTimeoutMs = 5000;
  int totalTimeoutMs = 30000;
  quint64 maxResponseBytes = 8ULL * 1024ULL * 1024ULL;
  int maxRetries = 1;
  int retryDelayMs = 100;
};

class HttpClient : public QObject {
 Q_OBJECT

 public:
  // Creates a reusable network client on the caller's Qt thread.
  explicit HttpClient(QObject* parent = nullptr);

  // Sends a GET request with timeout, size-cap, status, and retry handling.
  [[nodiscard]] QFuture<Result<QByteArray>> get(
      const QNetworkRequest& request,
      const HttpRequestOptions& options = HttpRequestOptions{});

  // Sends a JSON request body with the supplied method and safety limits.
  [[nodiscard]] QFuture<Result<QByteArray>> request(
      HttpMethod method, const QNetworkRequest& request, const QByteArray& body,
      const HttpRequestOptions& options = HttpRequestOptions{});

  // Sends a GET request and requires a JSON object response.
  [[nodiscard]] QFuture<Result<QJsonObject>> getJson(
      const QNetworkRequest& request,
      const HttpRequestOptions& options = HttpRequestOptions{});

 private:
  QNetworkAccessManager manager_;
};

}  // namespace aegis
