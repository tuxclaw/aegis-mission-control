#include "core/logging.h"

#include <cstdio>

#include <QMutex>
#include <QMutexLocker>
#include <QRegularExpression>
#include <QSet>

Q_LOGGING_CATEGORY(aegisAppLog, "aegis.app")
Q_LOGGING_CATEGORY(aegisConfigLog, "aegis.config")
Q_LOGGING_CATEGORY(aegisGatewayLog, "aegis.gateway")
Q_LOGGING_CATEGORY(aegisGitLog, "aegis.git")
Q_LOGGING_CATEGORY(aegisVitalsLog, "aegis.vitals")
Q_LOGGING_CATEGORY(aegisFilesystemLog, "aegis.filesystem")

namespace aegis::logging {
namespace {

QMutex& secretMutex() {
  static QMutex mutex;
  return mutex;
}

QSet<QString>& registeredSecrets() {
  static QSet<QString> secrets;
  return secrets;
}

void redactingHandler(QtMsgType type, const QMessageLogContext& context,
                      const QString& message) {
  const auto sanitized = redact(message);
  const auto category = QString::fromUtf8(context.category == nullptr
                                               ? "default"
                                               : context.category);
  const auto line = qFormatLogMessage(type, context,
                                      QStringLiteral("[%1] %2")
                                          .arg(category, sanitized));
  const auto bytes = line.toLocal8Bit();
  std::fwrite(bytes.constData(), 1, static_cast<std::size_t>(bytes.size()), stderr);
  std::fputc('\n', stderr);
  std::fflush(stderr);
  if (type == QtFatalMsg) {
    std::abort();
  }
}

}  // namespace

void installRedactingMessageHandler() {
  qInstallMessageHandler(redactingHandler);
}

void registerSecret(const QString& secret) {
  if (secret.size() < 4) {
    return;
  }
  QMutexLocker locker(&secretMutex());
  registeredSecrets().insert(secret);
}

QString redact(const QString& text) {
  auto sanitized = text;
  static const QRegularExpression bearer(
      QStringLiteral(R"((Bearer\s+)[A-Za-z0-9._~+/=-]+)"),
      QRegularExpression::CaseInsensitiveOption);
  static const QRegularExpression authorization(
      QStringLiteral(R"((Authorization\s*:\s*)[^\r\n]+)"),
      QRegularExpression::CaseInsensitiveOption);
  static const QRegularExpression assignment(
      QStringLiteral(
          R"(((?:api[_-]?key|token|secret|password)\s*[=:]\s*)[^\s,;]+)"),
      QRegularExpression::CaseInsensitiveOption);
  sanitized.replace(bearer, QStringLiteral("\\1[REDACTED]"));
  sanitized.replace(authorization, QStringLiteral("\\1[REDACTED]"));
  sanitized.replace(assignment, QStringLiteral("\\1[REDACTED]"));

  QMutexLocker locker(&secretMutex());
  for (const auto& secret : registeredSecrets()) {
    sanitized.replace(secret, QStringLiteral("[REDACTED]"));
  }
  return sanitized;
}

}  // namespace aegis::logging
