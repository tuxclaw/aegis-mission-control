#include <QtTest>

#include "dto/process_dto.h"

class ProcessServiceTest : public QObject {
  Q_OBJECT

 private slots:
  void parsesAndRoundTripsProcessSample();
  void rejectsOutOfRangeMemoryUsage();
};

void ProcessServiceTest::parsesAndRoundTripsProcessSample() {
  const auto object = QJsonObject{
      {QStringLiteral("processes"),
       QJsonArray{QJsonObject{
           {QStringLiteral("pid"), 4242},
           {QStringLiteral("name"), QStringLiteral("openclaw")},
           {QStringLiteral("user"), QStringLiteral("tux")},
           {QStringLiteral("cpuPct"), 137.5},
           {QStringLiteral("memPct"), 2.25},
           {QStringLiteral("command"),
            QStringLiteral("openclaw gateway --foreground")}}}},
      {QStringLiteral("sampledAt"),
       QStringLiteral("2026-07-18T11:00:00.000Z")}};

  const auto result = aegis::dto::ProcessDto::fromJson(object);
  QVERIFY(result.has_value());
  QCOMPARE(result->processes.size(), 1);
  QCOMPARE(result->processes.first().pid, 4242);
  QCOMPARE(result->processes.first().name, QStringLiteral("openclaw"));
  QCOMPARE(result->processes.first().user, QStringLiteral("tux"));
  QCOMPARE(result->processes.first().cpuPct, 137.5);
  QCOMPARE(result->processes.first().memPct, 2.25);

  const auto roundTrip = aegis::dto::ProcessDto::fromJson(result->toJson());
  QVERIFY(roundTrip.has_value());
  QCOMPARE(roundTrip->processes.size(), 1);
  QCOMPARE(roundTrip->sampledAt, result->sampledAt);
  QCOMPARE(roundTrip->processes.first().command,
           result->processes.first().command);
}

void ProcessServiceTest::rejectsOutOfRangeMemoryUsage() {
  const auto object = QJsonObject{
      {QStringLiteral("processes"),
       QJsonArray{QJsonObject{{QStringLiteral("pid"), 1},
                              {QStringLiteral("name"), QStringLiteral("init")},
                              {QStringLiteral("user"), QStringLiteral("root")},
                              {QStringLiteral("cpuPct"), 0.0},
                              {QStringLiteral("memPct"), 101.0},
                              {QStringLiteral("command"),
                               QStringLiteral("/sbin/init")}}}},
      {QStringLiteral("sampledAt"),
       QStringLiteral("2026-07-18T11:00:00.000Z")}};

  const auto result = aegis::dto::ProcessDto::fromJson(object);
  QVERIFY(!result.has_value());
  QCOMPARE(result.error().code, aegis::ErrorCode::ValidationFailed);
}

QTEST_MAIN(ProcessServiceTest)
#include "tst_process_service.moc"
