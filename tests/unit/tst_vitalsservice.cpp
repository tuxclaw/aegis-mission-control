#include <sys/statvfs.h>

#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

#include "config/config_service.h"
#include "controllers/vitals_controller.h"
#include "services/vitals_service.h"

class VitalsServiceTest : public QObject {
  Q_OBJECT

 private slots:
  void samplesLinuxProcFilesAndUpdatesController();
};

void VitalsServiceTest::samplesLinuxProcFilesAndUpdatesController() {
  QTemporaryDir directory;
  QVERIFY(directory.isValid());
  aegis::ConfigService config(
      directory.filePath(QStringLiteral("aegis.ini")));
  aegis::VitalsService service(&config);
  aegis::VitalsController controller(&service);

  int sampleCount = 0;
  connect(&controller, &aegis::VitalsController::vitalsChanged, this,
          [&sampleCount] { ++sampleCount; });
  controller.start();
  QTRY_VERIFY_WITH_TIMEOUT(sampleCount >= 2, 5000);
  controller.stop();

  QVERIFY(controller.cpuPct() >= 0.0);
  QVERIFY(controller.cpuPct() <= 100.0);
  QVERIFY(controller.memPct() > 0.0);
  QVERIFY(controller.memTotal() != QStringLiteral("0.0 GiB"));
  QVERIFY(!controller.diskModel().isEmpty());
  QVERIFY(!controller.netModel().isEmpty());

  const auto disk = controller.diskModel().first().toMap();
  const auto mount = disk.value(QStringLiteral("mount")).toString();
  QVERIFY(!mount.isEmpty());
  struct statvfs diskInfo {};
  const auto encodedMount = QFile::encodeName(mount);
  QCOMPARE(statvfs(encodedMount.constData(), &diskInfo), 0);
  const auto expectedTotal = static_cast<quint64>(diskInfo.f_blocks) *
                             static_cast<quint64>(diskInfo.f_frsize);
  const auto expectedUsed = expectedTotal -
                            static_cast<quint64>(diskInfo.f_bavail) *
                                static_cast<quint64>(diskInfo.f_frsize);
  QCOMPARE(disk.value(QStringLiteral("totalBytes")).toULongLong(),
           expectedTotal);
  QCOMPARE(disk.value(QStringLiteral("usedBytes")).toULongLong(),
           expectedUsed);
}

QTEST_MAIN(VitalsServiceTest)
#include "tst_vitalsservice.moc"
