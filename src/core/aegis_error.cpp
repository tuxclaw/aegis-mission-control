#include "core/aegis_error.h"

namespace aegis {

QString AegisError::stableCode() const {
  switch (code) {
    case ErrorCode::Ok: return QStringLiteral("ok");
    case ErrorCode::MissingToken: return QStringLiteral("missing_token");
    case ErrorCode::ConfigInvalid: return QStringLiteral("config_invalid");
    case ErrorCode::NetworkTimeout: return QStringLiteral("network_timeout");
    case ErrorCode::NetworkUnreachable: return QStringLiteral("network_unreachable");
    case ErrorCode::NetworkTls: return QStringLiteral("network_tls");
    case ErrorCode::HttpStatus: return QStringLiteral("http_status");
    case ErrorCode::ResponseTooLarge: return QStringLiteral("response_too_large");
    case ErrorCode::ResponseMalformed: return QStringLiteral("response_malformed");
    case ErrorCode::ProcessSpawnFailed: return QStringLiteral("process_spawn_failed");
    case ErrorCode::ProcessTimeout: return QStringLiteral("process_timeout");
    case ErrorCode::ProcessNonZeroExit: return QStringLiteral("process_nonzero_exit");
    case ErrorCode::CliOutputMalformed: return QStringLiteral("cli_output_malformed");
    case ErrorCode::PathOutsideSandbox: return QStringLiteral("path_outside_sandbox");
    case ErrorCode::PathSymlinkRejected: return QStringLiteral("path_symlink_rejected");
    case ErrorCode::PathNotFound: return QStringLiteral("path_not_found");
    case ErrorCode::FileTooLarge: return QStringLiteral("file_too_large");
    case ErrorCode::WriteFailed: return QStringLiteral("write_failed");
    case ErrorCode::GitOpenFailed: return QStringLiteral("git_open_failed");
    case ErrorCode::GitOperationFailed: return QStringLiteral("git_operation_failed");
    case ErrorCode::GitAuthFailed: return QStringLiteral("git_auth_failed");
    case ErrorCode::GitConflict: return QStringLiteral("git_conflict");
    case ErrorCode::ValidationFailed: return QStringLiteral("validation_failed");
    case ErrorCode::Cancelled: return QStringLiteral("cancelled");
    case ErrorCode::Unknown: return QStringLiteral("unknown");
  }
  return QStringLiteral("unknown");
}

QString safeMessageFor(ErrorCode code) {
  switch (code) {
    case ErrorCode::Ok: return QString();
    case ErrorCode::MissingToken: return QStringLiteral("A required credential is not configured.");
    case ErrorCode::ConfigInvalid: return QStringLiteral("A setting was invalid and was reset to its default.");
    case ErrorCode::NetworkTimeout: return QStringLiteral("The request timed out.");
    case ErrorCode::NetworkUnreachable: return QStringLiteral("The service could not be reached.");
    case ErrorCode::NetworkTls: return QStringLiteral("A secure connection could not be established.");
    case ErrorCode::HttpStatus: return QStringLiteral("The service returned an unsuccessful response.");
    case ErrorCode::ResponseTooLarge: return QStringLiteral("The response was too large to process safely.");
    case ErrorCode::ResponseMalformed: return QStringLiteral("The service returned malformed data.");
    case ErrorCode::ProcessSpawnFailed: return QStringLiteral("A required program could not be started.");
    case ErrorCode::ProcessTimeout: return QStringLiteral("The program did not finish in time.");
    case ErrorCode::ProcessNonZeroExit: return QStringLiteral("The program reported an error.");
    case ErrorCode::CliOutputMalformed: return QStringLiteral("The program returned malformed data.");
    case ErrorCode::PathOutsideSandbox: return QStringLiteral("Access to that path is not allowed.");
    case ErrorCode::PathSymlinkRejected: return QStringLiteral("A link points outside the allowed location.");
    case ErrorCode::PathNotFound: return QStringLiteral("The requested file was not found.");
    case ErrorCode::FileTooLarge: return QStringLiteral("The file is too large to preview.");
    case ErrorCode::WriteFailed: return QStringLiteral("The data could not be saved safely.");
    case ErrorCode::GitOpenFailed: return QStringLiteral("The configured repository could not be opened.");
    case ErrorCode::GitOperationFailed: return QStringLiteral("The repository operation failed.");
    case ErrorCode::GitAuthFailed: return QStringLiteral("Repository authentication failed.");
    case ErrorCode::GitConflict: return QStringLiteral("The repository operation requires manual conflict resolution.");
    case ErrorCode::ValidationFailed: return QStringLiteral("The supplied data was invalid.");
    case ErrorCode::Cancelled: return QStringLiteral("The operation was cancelled.");
    case ErrorCode::Unknown: return QStringLiteral("An unexpected error occurred.");
  }
  return QStringLiteral("An unexpected error occurred.");
}

bool isRetryable(ErrorCode code) {
  switch (code) {
    case ErrorCode::NetworkTimeout:
    case ErrorCode::NetworkUnreachable:
    case ErrorCode::HttpStatus:
    case ErrorCode::ProcessTimeout:
    case ErrorCode::ProcessNonZeroExit:
    case ErrorCode::GitOperationFailed:
    case ErrorCode::Unknown:
      return true;
    default:
      return false;
  }
}

AegisError makeError(ErrorCode code, const QString& debugContext) {
  return AegisError{code, safeMessageFor(code), debugContext, isRetryable(code)};
}

}  // namespace aegis
