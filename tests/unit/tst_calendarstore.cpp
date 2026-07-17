#include <QtTest>
#include <QFile>
#include <QTemporaryDir>
#include "config/config_service.h"
#include "services/calendar_store.h"

template <typename T>
T awaitResult(QFuture<T> future) {
  future.waitForFinished();
  return future.result();
}

class CalendarStoreTest : public QObject {
  Q_OBJECT
 private slots:
  void rejectsInvalidDraft() {
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    aegis::ConfigService config(temporary.filePath("settings.ini"));
    QVERIFY(config.setValue("data.root", temporary.path()));
    aegis::CalendarStore store(&config);
    aegis::dto::CalendarEventDto draft;
    draft.title = "Invalid";
    draft.start = QDateTime::currentDateTimeUtc();
    draft.end = draft.start.addSecs(-1);
    draft.color = "cyan";
    const auto result = awaitResult(store.create(draft));
    QVERIFY(!result);
    QCOMPARE(result.error().code, aegis::ErrorCode::ValidationFailed);
  }
  void reportsParseErrorWithoutOverwrite() {
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    aegis::ConfigService config(temporary.filePath("settings.ini"));
    QVERIFY(config.setValue("data.root", temporary.path()));
    QDir().mkpath(temporary.filePath("calendar"));
    QFile file(temporary.filePath("calendar/events.json"));
    QVERIFY(file.open(QIODevice::WriteOnly));
    const QByteArray corrupt("{not-json");
    QCOMPARE(file.write(corrupt), corrupt.size());
    file.close();
    aegis::CalendarStore store(&config);
    const auto result = awaitResult(store.load());
    QVERIFY(!result);
    QCOMPARE(result.error().code, aegis::ErrorCode::ResponseMalformed);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.readAll(), corrupt);
  }
  void roundTripCrud() {
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    aegis::ConfigService config(temporary.filePath("settings.ini"));
    QVERIFY(config.setValue("data.root", temporary.path()));
    aegis::CalendarStore store(&config);
    const auto start = QDateTime::currentDateTimeUtc().addSecs(3600);
    aegis::dto::CalendarEventDto draft;
    draft.title = "Mission review";
    draft.description = "AEGIS";
    draft.start = start;
    draft.end = start.addSecs(1800);
    draft.location = "Control room";
    draft.color = "cyan";
    auto created = awaitResult(store.create(draft));
    QVERIFY(created);
    auto loaded = awaitResult(store.load());
    QVERIFY(loaded);
    QCOMPARE(loaded->size(), 1);
    auto event = loaded->first();
    event.title = "Updated review";
    auto updated = awaitResult(store.update(event));
    QVERIFY(updated);
    QCOMPARE(updated->title, QString("Updated review"));
    QVERIFY(awaitResult(store.remove(updated->id)));
    loaded = awaitResult(store.load());
    QVERIFY(loaded);
    QVERIFY(loaded->isEmpty());
  }
};
QTEST_MAIN(CalendarStoreTest)
#include "tst_calendarstore.moc"
