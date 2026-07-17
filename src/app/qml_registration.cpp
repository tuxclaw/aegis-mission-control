#include "app/qml_registration.h"

#include <qqml.h>

#include "dto/enums.h"

namespace aegis {

void QmlRegistration::registerTypes() {
  for (const auto* name : {"AgentStatus", "CronState", "GitFileState",
                           "CreativeBackend"}) {
    qmlRegisterUncreatableMetaObject(dto::staticMetaObject, "Aegis", 1, 0,
                                     name,
                                     QStringLiteral("Enums are read-only"));
  }
  qmlRegisterUncreatableType<ErrorCodeValues>(
      "Aegis", 1, 0, "ErrorCode", QStringLiteral("Enums are read-only"));
  qmlRegisterUncreatableType<ConnectionStateValues>(
      "Aegis", 1, 0, "ConnectionState",
      QStringLiteral("Enums are read-only"));
}

}  // namespace aegis
