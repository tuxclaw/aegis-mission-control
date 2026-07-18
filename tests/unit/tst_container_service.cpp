#include <QtTest>

#include "dto/container_dto.h"

class ContainerServiceTest : public QObject {
  Q_OBJECT

 private slots:
  void parsesAndRoundTripsContainerSample();
  void rejectsInvalidContainerId();
};

void ContainerServiceTest::parsesAndRoundTripsContainerSample() {
  const auto object = QJsonObject{
      {QStringLiteral("containers"),
       QJsonArray{QJsonObject{
           {QStringLiteral("id"), QStringLiteral("a1b2c3d4e5f6")},
           {QStringLiteral("name"), QStringLiteral("ollama-rocm")},
           {QStringLiteral("image"),
            QStringLiteral("docker.io/ollama/ollama:rocm")},
           {QStringLiteral("status"), QStringLiteral("running")},
           {QStringLiteral("ports"),
            QStringLiteral("11434->11434/tcp")},
           {QStringLiteral("created"),
            QStringLiteral("2026-07-18T10:15:30.000Z")}}}},
      {QStringLiteral("sampledAt"),
       QStringLiteral("2026-07-18T11:00:00.000Z")}};

  const auto result = aegis::dto::ContainerDto::fromJson(object);
  QVERIFY(result.has_value());
  QCOMPARE(result->containers.size(), 1);
  QCOMPARE(result->containers.first().id, QStringLiteral("a1b2c3d4e5f6"));
  QCOMPARE(result->containers.first().name, QStringLiteral("ollama-rocm"));
  QCOMPARE(result->containers.first().status, QStringLiteral("running"));
  QCOMPARE(result->containers.first().ports,
           QStringLiteral("11434->11434/tcp"));

  const auto roundTrip =
      aegis::dto::ContainerDto::fromJson(result->toJson());
  QVERIFY(roundTrip.has_value());
  QCOMPARE(roundTrip->containers.size(), 1);
  QCOMPARE(roundTrip->sampledAt, result->sampledAt);
  QCOMPARE(roundTrip->containers.first().created,
           result->containers.first().created);
}

void ContainerServiceTest::rejectsInvalidContainerId() {
  const auto object = QJsonObject{
      {QStringLiteral("containers"),
       QJsonArray{QJsonObject{
           {QStringLiteral("id"), QStringLiteral("not-an-id")},
           {QStringLiteral("name"), QStringLiteral("example")},
           {QStringLiteral("image"), QStringLiteral("example:latest")},
           {QStringLiteral("status"), QStringLiteral("exited")},
           {QStringLiteral("ports"), QString()},
           {QStringLiteral("created"),
            QStringLiteral("2026-07-18T10:15:30.000Z")}}}},
      {QStringLiteral("sampledAt"),
       QStringLiteral("2026-07-18T11:00:00.000Z")}};

  const auto result = aegis::dto::ContainerDto::fromJson(object);
  QVERIFY(!result.has_value());
  QCOMPARE(result.error().code, aegis::ErrorCode::ValidationFailed);
}

QTEST_MAIN(ContainerServiceTest)
#include "tst_container_service.moc"
