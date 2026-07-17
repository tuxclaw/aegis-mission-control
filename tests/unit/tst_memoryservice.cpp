#include <QtTest>
#include <QFile>
#include <QTemporaryDir>
#include <QVariantMap>
#include "config/config_service.h"
#include "services/memory_service.h"

template <typename T>
T awaitMemory(QFuture<T> future) {
  future.waitForFinished();
  return future.result();
}
static bool writeTestFile(const QString& path, const QByteArray& data) {
  QFile file(path);
  return file.open(QIODevice::WriteOnly) && file.write(data) == data.size();
}

class MemoryServiceTest : public QObject {
  Q_OBJECT
 private slots:
  void enforcesSandboxAndMarkdownPolicy() {
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    const auto root = temporary.filePath("allowed");
    QVERIFY(QDir().mkpath(root));
    QVERIFY(writeTestFile(QDir(root).filePath("note.md"), "safe"));
    QVERIFY(writeTestFile(QDir(root).filePath("plain.txt"), "unsafe"));
    aegis::ConfigService config(temporary.filePath("settings.ini"));
    QVERIFY(config.setValue("memory.roots", QVariantMap{{"allowed", root}}));
    aegis::MemoryService service(&config);
    QCOMPARE(service.rootIds(), QStringList{"allowed"});
    QVERIFY(awaitMemory(service.read("allowed", "note.md")));
    auto unknown = awaitMemory(service.read("other", "note.md"));
    QVERIFY(!unknown);
    QCOMPARE(unknown.error().code, aegis::ErrorCode::PathOutsideSandbox);
    QVERIFY(!awaitMemory(service.read("allowed", "../note.md")));
    auto nonMarkdown = awaitMemory(service.read("allowed", "plain.txt"));
    QVERIFY(!nonMarkdown);
    QCOMPARE(nonMarkdown.error().code, aegis::ErrorCode::ValidationFailed);
  }
  void enforcesSizeCapAndRejectsEscapingSymlink() {
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    const auto root = temporary.filePath("allowed");
    QVERIFY(QDir().mkpath(root));
    QVERIFY(writeTestFile(QDir(root).filePath("large.md"), QByteArray(2048, 'x')));
    const auto outside = temporary.filePath("outside.md");
    QVERIFY(writeTestFile(outside, "outside"));
    QVERIFY(QFile::link(outside, QDir(root).filePath("escape.md")));
    aegis::ConfigService config(temporary.filePath("settings.ini"));
    QVERIFY(config.setValue("memory.roots", QVariantMap{{"allowed", root}}));
    QVERIFY(config.setValue("memory.maxFileBytes", 1024));
    aegis::MemoryService service(&config);
    auto large = awaitMemory(service.read("allowed", "large.md"));
    QVERIFY(!large);
    QCOMPARE(large.error().code, aegis::ErrorCode::FileTooLarge);
    auto link = awaitMemory(service.read("allowed", "escape.md"));
    QVERIFY(!link);
    QCOMPARE(link.error().code, aegis::ErrorCode::PathSymlinkRejected);
  }
};
QTEST_MAIN(MemoryServiceTest)
#include "tst_memoryservice.moc"
