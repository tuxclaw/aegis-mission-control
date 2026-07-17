#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

#include "core/path_guard.h"

namespace {

bool writeFile(const QString& path, const QByteArray& contents) {
  QFile file(path);
  return file.open(QIODevice::WriteOnly) && file.write(contents) == contents.size();
}

}  // namespace

class PathGuardTest : public QObject {
  Q_OBJECT

 private slots:
  void rejectsTraversal();
  void rejectsAbsolutePath();
  void rejectsSymlinkEscape();
  void rejectsNonMarkdown();
  void rejectsOversize();
  void rejectsNulByte();
  void acceptsValidContainedFile();
};

void PathGuardTest::rejectsTraversal() {
  QTemporaryDir temporary;
  QVERIFY(temporary.isValid());
  const auto root = temporary.filePath(QStringLiteral("root"));
  QVERIFY(QDir().mkpath(root));
  QVERIFY(writeFile(temporary.filePath(QStringLiteral("outside.md")), "outside"));
  const auto guard = aegis::PathGuard::create(root, 1024);
  QVERIFY(guard.has_value());
  const auto result = guard->resolve(QStringLiteral("../outside.md"));
  QVERIFY(!result.has_value());
  QCOMPARE(result.error().code, aegis::ErrorCode::PathOutsideSandbox);
}

void PathGuardTest::rejectsAbsolutePath() {
  QTemporaryDir temporary;
  QVERIFY(temporary.isValid());
  QVERIFY(writeFile(temporary.filePath(QStringLiteral("safe.md")), "safe"));
  const auto guard = aegis::PathGuard::create(temporary.path(), 1024);
  QVERIFY(guard.has_value());
  const auto result = guard->resolve(temporary.filePath(QStringLiteral("safe.md")));
  QVERIFY(!result.has_value());
  QCOMPARE(result.error().code, aegis::ErrorCode::PathOutsideSandbox);
}

void PathGuardTest::rejectsSymlinkEscape() {
  QTemporaryDir temporary;
  QVERIFY(temporary.isValid());
  const auto root = temporary.filePath(QStringLiteral("root"));
  QVERIFY(QDir().mkpath(root));
  const auto outside = temporary.filePath(QStringLiteral("outside.md"));
  QVERIFY(writeFile(outside, "outside"));
  const auto link = QDir(root).filePath(QStringLiteral("escape.md"));
  QVERIFY(QFile::link(outside, link));
  const auto guard = aegis::PathGuard::create(root, 1024);
  QVERIFY(guard.has_value());
  const auto result = guard->resolve(QStringLiteral("escape.md"));
  QVERIFY(!result.has_value());
  QCOMPARE(result.error().code, aegis::ErrorCode::PathSymlinkRejected);
}

void PathGuardTest::rejectsNonMarkdown() {
  QTemporaryDir temporary;
  QVERIFY(temporary.isValid());
  QVERIFY(writeFile(temporary.filePath(QStringLiteral("notes.txt")), "notes"));
  const auto guard = aegis::PathGuard::create(temporary.path(), 1024);
  QVERIFY(guard.has_value());
  const auto result = guard->resolve(QStringLiteral("notes.txt"));
  QVERIFY(!result.has_value());
  QCOMPARE(result.error().code, aegis::ErrorCode::ValidationFailed);
}

void PathGuardTest::rejectsOversize() {
  QTemporaryDir temporary;
  QVERIFY(temporary.isValid());
  QVERIFY(writeFile(temporary.filePath(QStringLiteral("large.md")), "12345"));
  const auto guard = aegis::PathGuard::create(temporary.path(), 4);
  QVERIFY(guard.has_value());
  const auto result = guard->resolve(QStringLiteral("large.md"));
  QVERIFY(!result.has_value());
  QCOMPARE(result.error().code, aegis::ErrorCode::FileTooLarge);
}

void PathGuardTest::rejectsNulByte() {
  QTemporaryDir temporary;
  QVERIFY(temporary.isValid());
  const auto guard = aegis::PathGuard::create(temporary.path(), 1024);
  QVERIFY(guard.has_value());
  const auto result = guard->resolve(QStringLiteral("safe.md") + QChar::Null);
  QVERIFY(!result.has_value());
  QCOMPARE(result.error().code, aegis::ErrorCode::ValidationFailed);
}

void PathGuardTest::acceptsValidContainedFile() {
  QTemporaryDir temporary;
  QVERIFY(temporary.isValid());
  const auto nested = temporary.filePath(QStringLiteral("nested"));
  QVERIFY(QDir().mkpath(nested));
  const auto path = QDir(nested).filePath(QStringLiteral("safe.MD"));
  QVERIFY(writeFile(path, "safe"));
  const auto guard = aegis::PathGuard::create(temporary.path(), 1024);
  QVERIFY(guard.has_value());
  const auto result = guard->resolve(QStringLiteral("nested/safe.MD"));
  QVERIFY(result.has_value());
  QCOMPARE(result.value(), QFileInfo(path).canonicalFilePath());
}

QTEST_MAIN(PathGuardTest)
#include "tst_pathguard.moc"
