#include <QtTest>
#include <QJsonArray>
#include <QJsonDocument>
#include "services/openclaw_cli.h"

class OpenClawCliParseTest : public QObject {
  Q_OBJECT
 private slots:
  void parsesStructuredFixtures() {
    const QJsonObject agent{{"agentDir", "/tmp/agents/helen"},
                            {"bindings", 0},
                            {"id", "helen"},
                            {"identityEmoji", "🦸‍♀️"},
                            {"identityName", "Helen"},
                            {"identitySource", "profile"},
                            {"isDefault", false},
                            {"model", "openai/gpt-example"},
                            {"status", "active"},
                            {"statusDetail", "running a session"},
                            {"lastSeen", "2026-07-17T08:00:00.000Z"},
                            {"activeSessions", 2},
                            {"workspace", "/tmp/workspace"}};
    auto agents = aegis::OpenClawCli::parseAgents(
        QJsonDocument(QJsonObject{{"agents", QJsonArray{agent}}})
            .toJson(QJsonDocument::Compact));
    QVERIFY(agents);
    QCOMPARE(agents->first().id, QString("helen"));
    QCOMPARE(agents->first().displayName, QString("Helen"));
    QCOMPARE(agents->first().status, aegis::dto::AgentStatus::Active);
    QCOMPARE(agents->first().statusDetail, QString("running a session"));
    QCOMPARE(agents->first().activeSessions, 2);

    auto configuredAgent = agent;
    configuredAgent.remove(QStringLiteral("status"));
    configuredAgent.remove(QStringLiteral("statusDetail"));
    configuredAgent.remove(QStringLiteral("lastSeen"));
    configuredAgent.remove(QStringLiteral("activeSessions"));
    auto configuredAgents = aegis::OpenClawCli::parseAgents(
        QJsonDocument(QJsonArray{configuredAgent})
            .toJson(QJsonDocument::Compact));
    QVERIFY(configuredAgents);
    QCOMPARE(configuredAgents->first().status,
             aegis::dto::AgentStatus::Idle);

    const QJsonObject job{
        {"agentId", "helen"},
        {"createdAtMs", 1784300000000.0},
        {"delivery", QJsonObject{{"mode", "announce"}}},
        {"description", "Daily briefing"},
        {"enabled", true},
        {"id", "daily"},
        {"lastDelivered", true},
        {"lastDeliveryStatus", "delivered"},
        {"lastFailureNotificationDeliveryStatus", "not-requested"},
        {"lastRunAtMs", 1784300564951.0},
        {"lastRunStatus", "ok"},
        {"name", "Daily"},
        {"nextRunAtMs", 1784314964947.0},
        {"payload", QJsonObject{{"kind", "agentTurn"},
                                 {"message", "briefing"},
                                 {"model", "openai/gpt-example"},
                                 {"timeoutSeconds", 300}}},
        {"schedule", QJsonObject{{"kind", "cron"},
                                  {"expr", "0 6 * * *"},
                                  {"tz", "America/Los_Angeles"},
                                  {"staggerMs", 300000}}},
        {"sessionTarget", "isolated"},
        {"state", QJsonObject{{"lastRunStatus", "ok"}}},
        {"status", "ok"},
        {"updatedAtMs", 1784300000000.0},
        {"wakeMode", "now"}};
    auto jobs = aegis::OpenClawCli::parseCron(
        QJsonDocument(QJsonObject{{"jobs", QJsonArray{job}}})
            .toJson(QJsonDocument::Compact));
    QVERIFY(jobs);
    QCOMPARE(jobs->size(), 1);
    QCOMPARE(jobs->first().schedule, QString("0 6 * * *"));
    QCOMPARE(jobs->first().state, aegis::dto::CronState::Enabled);
    QCOMPARE(jobs->first().lastResult, QString("success"));
    QCOMPARE(jobs->first().lastRun.toMSecsSinceEpoch(), 1784300564951LL);

    const QJsonObject model{{"available", true},
                            {"contextWindow", 1048576},
                            {"input", "text"},
                            {"key", "openai/example"},
                            {"local", false},
                            {"missing", false},
                            {"name", "Example"},
                            {"tags", QJsonArray{"default", "configured"}}};
    auto models = aegis::OpenClawCli::parseModels(
        QJsonDocument(QJsonObject{{"models", QJsonArray{model}}})
            .toJson(QJsonDocument::Compact));
    QVERIFY(models);
    QVERIFY(models->first().isActive);
    QCOMPARE(models->first().provider, QString("openai"));
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
