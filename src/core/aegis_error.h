#pragma once

#include <QString>

namespace aegis {

enum class ErrorCode {
  Ok = 0,
  MissingToken,
  ConfigInvalid,
  NetworkTimeout,
  NetworkUnreachable,
  NetworkTls,
  HttpStatus,
  ResponseTooLarge,
  ResponseMalformed,
  ProcessSpawnFailed,
  ProcessTimeout,
  ProcessNonZeroExit,
  CliOutputMalformed,
  PathOutsideSandbox,
  PathSymlinkRejected,
  PathNotFound,
  FileTooLarge,
  WriteFailed,
  GitOpenFailed,
  GitOperationFailed,
  GitAuthFailed,
  GitConflict,
  ValidationFailed,
  Cancelled,
  Unknown,
};

struct AegisError {
  ErrorCode code = ErrorCode::Unknown;
  QString userMessage;
  QString debugContext;
  bool retryable = false;

  // Returns the stable machine-readable identifier for this error.
  [[nodiscard]] QString stableCode() const;
};

// Creates an error with a stable safe message and retry policy.
[[nodiscard]] AegisError makeError(ErrorCode code,
                                   const QString& debugContext = {});

// Returns the safe user-facing message associated with an error code.
[[nodiscard]] QString safeMessageFor(ErrorCode code);

// Returns whether an operation with this error can normally be retried.
[[nodiscard]] bool isRetryable(ErrorCode code);

}  // namespace aegis
