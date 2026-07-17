#include <QtTest>
#include <QJsonArray>
#include <QJsonDocument>
#include "services/openclaw_cli.h"

class OpenClawCliParseTest : public QObject {
  Q_OBJECT
 private slots:
  void parsesStructuredFixtures() {
    aegis::dto::AgentDto agent;
    agent.id = "helen";
    agent.displayName = "Helen";
    agent.model = "openai/example";
    agent.status = aegis::dto::AgentStatus::Active;
    agent.activeSessions = 1;
    agent.avatarSeed = "helen";
    auto agents = aegis::OpenClawCli::parseAgents(
        QJsonDocument(QJsonObject{{"agents", QJsonArray{agent.toJson()}}})
            .toJson(QJsonDocument::Compact));
    QVERIFY(agents);
    QCOMPARE(agents->first().id, QString("helen"));

    aegis::dto::CronJobDto job;
    job.id = "daily";
    job.name = "Daily";
    job.schedule = "0 6 * * *";
    job.state = aegis::dto::CronState::Enabled;
    job.command = "briefing";
    auto jobs = aegis::OpenClawCli::parseCron(
        QJsonDocument(QJsonObject{{"jobs", QJsonArray{job.toJson()}}})
            .toJson(QJsonDocument::Compact));
    QVERIFY(jobs);
    QCOMPARE(jobs->size(), 1);

    aegis::dto::ModelDto model{"openai/example", "openai", "Example", true};
    auto models = aegis::OpenClawCli::parseModels(
        QJsonDocument(QJsonObject{{"models", QJsonArray{model.toJson()}}})
            .toJson(QJsonDocument::Compact));
    QVERIFY(models);
    QVERIFY(models->first().isActive);
  }
  void rejectsMalformedFixtures() {
    QVERIFY(!aegis::OpenClawCli::parseAgents("not json"));
    QVERIFY(!aegis::OpenClawCli::parseCron("{}"));
    QVERIFY(!aegis::OpenClawCli::parseModels("[42]"));
    const auto invalid = QJsonDocument(QJsonArray{QJsonObject{
        {"id", "only-id"}, {"unexpected", true}}}).toJson();
    const auto result = aegis::OpenClawCli::parseAgents(invalid);
    QVERIFY(!result);
    QCOMPARE(result.error().code, aegis::ErrorCode::CliOutputMalformed);
  }
};
QTEST_MAIN(OpenClawCliParseTest)
#include "tst_openclawcli_parse.moc"
