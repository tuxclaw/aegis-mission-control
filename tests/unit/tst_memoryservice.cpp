#include <QtTest>
#include <QFile>
#include <QTemporaryDir>
#include <QVariantMap>
#include "config/config_service.h"
#include "controllers/memory_controller.h"
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
  void defaultsToOpenClawWorkspaceMemory() {
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    aegis::ConfigService config(temporary.filePath("settings.ini"));
    const auto roots = config.memoryRoots();
    QVERIFY(roots);
    QCOMPARE(roots->value(QStringLiteral("workspace")),
             QDir::home().absoluteFilePath(
                 QStringLiteral(".openclaw/workspace/memory")));
  }
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
    const auto files = awaitMemory(service.list("allowed"));
    QVERIFY(files);
    QCOMPARE(files->size(), 1);
    QCOMPARE(files->first().relativePath, QStringLiteral("note.md"));
    QVERIFY(awaitMemory(service.read("allowed", "note.md")));
    auto unknown = awaitMemory(service.read("other", "note.md"));
    QVERIFY(!unknown);
    QCOMPARE(unknown.error().code, aegis::ErrorCode::PathOutsideSandbox);
    QVERIFY(!awaitMemory(service.read("allowed", "../note.md")));
    auto nonMarkdown = awaitMemory(service.read("allowed", "plain.txt"));
    QVERIFY(!nonMarkdown);
    QCOMPARE(nonMarkdown.error().code, aegis::ErrorCode::ValidationFailed);
  }
  void controllerPublishesListedFiles() {
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    const auto root = temporary.filePath("allowed");
    QVERIFY(QDir().mkpath(root));
    QVERIFY(writeTestFile(QDir(root).filePath("note.md"), "safe"));
    aegis::ConfigService config(temporary.filePath("settings.ini"));
    QVERIFY(config.setValue("memory.roots", QVariantMap{{"allowed", root}}));
    aegis::MemoryService service(&config);
    aegis::MemoryController controller(&service);
    QSignalSpy filesChanged(&controller,
                            &aegis::MemoryController::filesChanged);
    QTRY_COMPARE_WITH_TIMEOUT(filesChanged.count(), 1, 2000);
    QCOMPARE(controller.files()->rowCount(), 1);
  }
  void controllerReportsListErrors() {
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    const auto missingRoot = temporary.filePath("missing");
    aegis::ConfigService config(temporary.filePath("settings.ini"));
    QVERIFY(config.setValue("memory.roots",
                            QVariantMap{{"missing", missingRoot}}));
    aegis::MemoryService service(&config);
    aegis::MemoryController controller(&service);
    QSignalSpy errorRaised(&controller,
                           &aegis::MemoryController::errorRaised);
    QTRY_COMPARE_WITH_TIMEOUT(errorRaised.count(), 1, 2000);
    QCOMPARE(controller.files()->rowCount(), 0);
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
