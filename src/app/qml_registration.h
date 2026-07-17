#pragma once

#include <QObject>

class QQmlApplicationEngine;

namespace aegis {
class AppContext;

class ErrorCodeValues : public QObject {
  Q_OBJECT

 public:
  enum Value {
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
  Q_ENUM(Value)
};

class ConnectionStateValues : public QObject {
  Q_OBJECT

 public:
  enum Value { Disconnected, Connecting, Connected, Error };
  Q_ENUM(Value)
};

class QmlRegistration {
 public:
  // Registers DTO, error, and connection enums for named QML access.
  static void registerTypes();
  // Exposes every controller as its documented context property.
  static void registerContext(QQmlApplicationEngine* engine,
                              AppContext* context);
};

}  // namespace aegis
