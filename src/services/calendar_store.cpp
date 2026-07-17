#include "services/calendar_store.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QMutexLocker>
#include <QUuid>

#include "config/config_service.h"
#include "core/async.h"

namespace aegis {
namespace {

constexpr qsizetype kMaxEvents = 10000;

Result<void> validateEvent(const dto::CalendarEventDto& event) {
  return dto::CalendarEventDto::fromJson(event.toJson()).transform(
      [](const auto&) {});
}

}  // namespace

CalendarStore::CalendarStore(ConfigService* config, QObject* parent)
    : QObject(parent), config_(config) {}

QFuture<Result<QVector<dto::CalendarEventDto>>> CalendarStore::load() {
  return async::run([this] {
    const auto path = storePath();
    if (!path) return Result<QVector<dto::CalendarEventDto>>(
        tl::unexpected(path.error()));
    QMutexLocker locker(&mutex_);
    return loadLocked(path.value());
  });
}

QFuture<Result<dto::CalendarEventDto>> CalendarStore::create(
    dto::CalendarEventDto draft) {
  return async::run([this, draft = std::move(draft)]() mutable
                        -> Result<dto::CalendarEventDto> {
    const auto path = storePath();
    if (!path) return tl::unexpected(path.error());
    QMutexLocker locker(&mutex_);
    auto events = loadLocked(path.value());
    if (!events) return tl::unexpected(events.error());
    if (events->size() >= kMaxEvents) {
      return tl::unexpected(makeError(ErrorCode::ValidationFailed,
                                      QStringLiteral("calendar event cap reached")));
    }
    const auto now = QDateTime::currentDateTimeUtc();
    draft.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    draft.createdAt = now;
    draft.updatedAt = now;
    draft.start = draft.start.toUTC();
    draft.end = draft.end.toUTC();
    const auto valid = validateEvent(draft);
    if (!valid) return tl::unexpected(valid.error());
    events->append(draft);
    const auto written = writeLocked(path.value(), events.value());
    if (!written) return tl::unexpected(written.error());
    QMetaObject::invokeMethod(this, &CalendarStore::changed,
                              Qt::QueuedConnection);
    return draft;
  });
}

QFuture<Result<dto::CalendarEventDto>> CalendarStore::update(
    dto::CalendarEventDto event) {
  return async::run([this, event = std::move(event)]() mutable
                        -> Result<dto::CalendarEventDto> {
    const auto path = storePath();
    if (!path) return tl::unexpected(path.error());
    QMutexLocker locker(&mutex_);
    auto events = loadLocked(path.value());
    if (!events) return tl::unexpected(events.error());
    auto found = std::find_if(events->begin(), events->end(),
                              [&event](const auto& candidate) {
                                return candidate.id == event.id;
                              });
    if (found == events->end()) {
      return tl::unexpected(makeError(ErrorCode::PathNotFound,
                                      QStringLiteral("calendar event not found")));
    }
    event.createdAt = found->createdAt;
    event.updatedAt = QDateTime::currentDateTimeUtc();
    event.start = event.start.toUTC();
    event.end = event.end.toUTC();
    const auto valid = validateEvent(event);
    if (!valid) return tl::unexpected(valid.error());
    *found = event;
    const auto written = writeLocked(path.value(), events.value());
    if (!written) return tl::unexpected(written.error());
    QMetaObject::invokeMethod(this, &CalendarStore::changed,
                              Qt::QueuedConnection);
    return event;
  });
}

QFuture<Result<void>> CalendarStore::remove(QString id) {
  return async::run([this, id = std::move(id)]() -> Result<void> {
    const QUuid uuid(id);
    if (uuid.isNull() || uuid.toString(QUuid::WithoutBraces) != id) {
      return tl::unexpected(makeError(ErrorCode::ValidationFailed,
                                      QStringLiteral("invalid calendar id")));
    }
    const auto path = storePath();
    if (!path) return tl::unexpected(path.error());
    QMutexLocker locker(&mutex_);
    auto events = loadLocked(path.value());
    if (!events) return tl::unexpected(events.error());
    const auto originalSize = events->size();
    events->erase(std::remove_if(events->begin(), events->end(),
                                 [&id](const auto& event) {
                                   return event.id == id;
                                 }),
                  events->end());
    if (events->size() == originalSize) {
      return tl::unexpected(makeError(ErrorCode::PathNotFound,
                                      QStringLiteral("calendar event not found")));
    }
    const auto written = writeLocked(path.value(), events.value());
    if (!written) return tl::unexpected(written.error());
    QMetaObject::invokeMethod(this, &CalendarStore::changed,
                              Qt::QueuedConnection);
    return {};
  });
}

Result<QString> CalendarStore::storePath() const {
  const auto root = config_->dataRoot();
  if (!root) return tl::unexpected(root.error());
  const auto directory = QDir(root.value()).filePath(QStringLiteral("calendar"));
  if (!QDir().mkpath(directory)) {
    return tl::unexpected(makeError(ErrorCode::WriteFailed,
                                    QStringLiteral("calendar directory failed")));
  }
  return QDir(directory).filePath(QStringLiteral("events.json"));
}

Result<QVector<dto::CalendarEventDto>> CalendarStore::loadLocked(
    const QString& path) const {
  const QFileInfo info(path);
  if (!info.exists()) return QVector<dto::CalendarEventDto>();
  if (!info.isFile() || info.size() > 32 * 1024 * 1024) {
    return tl::unexpected(makeError(ErrorCode::ResponseTooLarge,
                                    QStringLiteral("calendar store invalid")));
  }
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) {
    return tl::unexpected(makeError(ErrorCode::PathNotFound,
                                    QStringLiteral("calendar store unreadable")));
  }
  QJsonParseError parseError;
  const auto document = QJsonDocument::fromJson(file.readAll(), &parseError);
  if (parseError.error != QJsonParseError::NoError || !document.isObject() ||
      !document.object().value(QStringLiteral("events")).isArray()) {
    return tl::unexpected(makeError(ErrorCode::ResponseMalformed,
                                    QStringLiteral("calendar JSON invalid")));
  }
  const auto array = document.object().value(QStringLiteral("events")).toArray();
  if (array.size() > kMaxEvents) {
    return tl::unexpected(makeError(ErrorCode::ValidationFailed,
                                    QStringLiteral("calendar event cap exceeded")));
  }
  QVector<dto::CalendarEventDto> events;
  events.reserve(array.size());
  for (const auto& value : array) {
    if (!value.isObject()) {
      return tl::unexpected(makeError(ErrorCode::ResponseMalformed,
                                      QStringLiteral("calendar entry invalid")));
    }
    const auto event = dto::CalendarEventDto::fromJson(value.toObject());
    if (!event) return tl::unexpected(event.error());
    events.append(event.value());
  }
  return events;
}

Result<void> CalendarStore::writeLocked(
    const QString& path, const QVector<dto::CalendarEventDto>& events) {
  QJsonArray array;
  for (const auto& event : events) array.append(event.toJson());
  return atomicFile_.write(
      path, QJsonDocument(QJsonObject{{QStringLiteral("version"), 1},
                                     {QStringLiteral("events"), array}})
                .toJson(QJsonDocument::Compact));
}

}  // namespace aegis
