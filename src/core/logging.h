#pragma once

#include <QLoggingCategory>
#include <QString>

Q_DECLARE_LOGGING_CATEGORY(aegisAppLog)
Q_DECLARE_LOGGING_CATEGORY(aegisConfigLog)
Q_DECLARE_LOGGING_CATEGORY(aegisGatewayLog)
Q_DECLARE_LOGGING_CATEGORY(aegisGitLog)
Q_DECLARE_LOGGING_CATEGORY(aegisVitalsLog)
Q_DECLARE_LOGGING_CATEGORY(aegisFilesystemLog)

namespace aegis::logging {

// Installs the process-wide message handler that redacts known secret patterns.
void installRedactingMessageHandler();

// Registers an exact secret value for redaction without logging it.
void registerSecret(const QString& secret);

// Returns text with credentials and secret-pattern values removed.
[[nodiscard]] QString redact(const QString& text);

}  // namespace aegis::logging
