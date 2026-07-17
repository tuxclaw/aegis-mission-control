#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

#include "core/atomic_file.h"

namespace {

QByteArray readFile(const QString& path) {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) return {};
  return file.readAll();
}

bool writeFile(const QString& path, const QByteArray& contents) {
  QFile file(path);
  return file.open(QIODevice::WriteOnly) && file.write(contents) == contents.size();
}

}  // namespace

class AtomicFileTest : public QObject {
  Q_OBJECT

 private slots:
  void replacesThroughAtomicCommit();
  void failureLeavesOriginalIntact();
};

void AtomicFileTest::replacesThroughAtomicCommit() {
  QTemporaryDir temporary;
  QVERIFY(temporary.isValid());
  const auto target = temporary.filePath(QStringLiteral("events.json"));
  QVERIFY(writeFile(target, "old"));
  aegis::AtomicFile writer;
  const auto result = writer.write(target, QByteArrayLiteral("new-data"));
  QVERIFY(result.has_value());
  QCOMPARE(readFile(target), QByteArrayLiteral("new-data"));
  const auto leftovers = QDir(temporary.path()).entryList(
      {QStringLiteral("events.json.*")}, QDir::Files | QDir::Hidden);
  QVERIFY(leftovers.isEmpty());
}

void AtomicFileTest::failureLeavesOriginalIntact() {
  QTemporaryDir temporary;
  QVERIFY(temporary.isValid());
  const auto directory = temporary.filePath(QStringLiteral("readonly"));
  QVERIFY(QDir().mkpath(directory));
  const auto target = QDir(directory).filePath(QStringLiteral("events.json"));
  QVERIFY(writeFile(target, "original"));
  QVERIFY(QFile::setPermissions(
      directory, QFileDevice::ReadOwner | QFileDevice::ExeOwner));

  aegis::AtomicFile writer;
  const auto result = writer.write(target, QByteArrayLiteral("replacement"));
  QVERIFY(!result.has_value());
  QCOMPARE(result.error().code, aegis::ErrorCode::WriteFailed);
  QCOMPARE(readFile(target), QByteArrayLiteral("original"));

  QVERIFY(QFile::setPermissions(directory, QFileDevice::ReadOwner |
                                              QFileDevice::WriteOwner |
                                              QFileDevice::ExeOwner));
}

QTEST_MAIN(AtomicFileTest)
#include "tst_atomicfile.moc"
