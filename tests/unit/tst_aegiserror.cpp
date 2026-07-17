#include <QtTest>

#include "core/aegis_error.h"

class AegisErrorTest : public QObject {
  Q_OBJECT

 private slots:
  void mappingsAreStableAndSafe();
  void retryPolicyIsCorrect();
};

void AegisErrorTest::mappingsAreStableAndSafe() {
  const QVector<aegis::ErrorCode> codes = {
      aegis::ErrorCode::Ok,
      aegis::ErrorCode::MissingToken,
      aegis::ErrorCode::ConfigInvalid,
      aegis::ErrorCode::NetworkTimeout,
      aegis::ErrorCode::NetworkUnreachable,
      aegis::ErrorCode::NetworkTls,
      aegis::ErrorCode::HttpStatus,
      aegis::ErrorCode::ResponseTooLarge,
      aegis::ErrorCode::ResponseMalformed,
      aegis::ErrorCode::ProcessSpawnFailed,
      aegis::ErrorCode::ProcessTimeout,
      aegis::ErrorCode::ProcessNonZeroExit,
      aegis::ErrorCode::CliOutputMalformed,
      aegis::ErrorCode::PathOutsideSandbox,
      aegis::ErrorCode::PathSymlinkRejected,
      aegis::ErrorCode::PathNotFound,
      aegis::ErrorCode::FileTooLarge,
      aegis::ErrorCode::WriteFailed,
      aegis::ErrorCode::GitOpenFailed,
      aegis::ErrorCode::GitOperationFailed,
      aegis::ErrorCode::GitAuthFailed,
      aegis::ErrorCode::GitConflict,
      aegis::ErrorCode::ValidationFailed,
      aegis::ErrorCode::Cancelled,
      aegis::ErrorCode::Unknown};
  const auto marker = QStringLiteral("/private/path Authorization: Bearer hidden");
  for (const auto code : codes) {
    const auto error = aegis::makeError(code, marker);
    QVERIFY(!error.stableCode().isEmpty());
    QCOMPARE(error.debugContext, marker);
    QVERIFY(!error.userMessage.contains(marker));
    QVERIFY(!error.userMessage.contains(QStringLiteral("Bearer")));
    if (code != aegis::ErrorCode::Ok) QVERIFY(!error.userMessage.isEmpty());
  }
}

void AegisErrorTest::retryPolicyIsCorrect() {
  QVERIFY(aegis::makeError(aegis::ErrorCode::NetworkTimeout).retryable);
  QVERIFY(aegis::makeError(aegis::ErrorCode::NetworkUnreachable).retryable);
  QVERIFY(!aegis::makeError(aegis::ErrorCode::MissingToken).retryable);
  QVERIFY(!aegis::makeError(aegis::ErrorCode::ValidationFailed).retryable);
}

QTEST_MAIN(AegisErrorTest)
#include "tst_aegiserror.moc"
