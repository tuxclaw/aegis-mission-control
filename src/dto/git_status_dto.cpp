#include "dto/git_status_dto.h"

#include <QDir>
#include <QJsonArray>

#include "dto/json_validation.h"

namespace aegis::dto {
namespace {

Result<GitFileState> parseFileState(const QJsonValue& value) {
  if (!value.isString()) {
    return tl::unexpected(makeError(ErrorCode::ValidationFailed,
                                    QStringLiteral("git state must be a string")));
  }
  const auto state = value.toString();
  if (state == QStringLiteral("new")) return GitFileState::New;
  if (state == QStringLiteral("modified")) return GitFileState::Modified;
  if (state == QStringLiteral("deleted")) return GitFileState::Deleted;
  if (state == QStringLiteral("renamed")) return GitFileState::Renamed;
  if (state == QStringLiteral("untracked")) return GitFileState::Untracked;
  if (state == QStringLiteral("conflicted")) return GitFileState::Conflicted;
  if (state == QStringLiteral("ignored")) return GitFileState::Ignored;
  return tl::unexpected(makeError(ErrorCode::ValidationFailed,
                                  QStringLiteral("unknown git file state")));
}

QString stateName(GitFileState state) {
  switch (state) {
    case GitFileState::New: return QStringLiteral("new");
    case GitFileState::Modified: return QStringLiteral("modified");
    case GitFileState::Deleted: return QStringLiteral("deleted");
    case GitFileState::Renamed: return QStringLiteral("renamed");
    case GitFileState::Untracked: return QStringLiteral("untracked");
    case GitFileState::Conflicted: return QStringLiteral("conflicted");
    case GitFileState::Ignored: return QStringLiteral("ignored");
  }
  return QStringLiteral("untracked");
}

Result<GitFileEntry> parseEntry(const QJsonObject& object) {
  const auto unknown = json::rejectUnknown(
      object, {QStringLiteral("path"), QStringLiteral("indexState"),
               QStringLiteral("worktreeState"), QStringLiteral("staged")});
  if (!unknown) return tl::unexpected(unknown.error());
  const auto path = json::requiredString(
      object, QStringLiteral("path"), 1, 4096);
  if (!path) return tl::unexpected(path.error());
  const auto cleaned = QDir::cleanPath(path.value());
  if (QDir::isAbsolutePath(path.value()) || path->contains(QChar::Null) ||
      cleaned == QStringLiteral("..") || cleaned.startsWith(QStringLiteral("../"))) {
    return tl::unexpected(makeError(ErrorCode::ValidationFailed,
                                    QStringLiteral("invalid repo-relative path")));
  }
  const auto index = parseFileState(object.value(QStringLiteral("indexState")));
  if (!index) return tl::unexpected(index.error());
  const auto worktree = parseFileState(
      object.value(QStringLiteral("worktreeState")));
  if (!worktree) return tl::unexpected(worktree.error());
  const auto staged = json::requiredBool(object, QStringLiteral("staged"));
  if (!staged) return tl::unexpected(staged.error());
  return GitFileEntry{cleaned, index.value(), worktree.value(), staged.value()};
}

QJsonObject entryJson(const GitFileEntry& entry) {
  return {{QStringLiteral("path"), entry.path},
          {QStringLiteral("indexState"), stateName(entry.indexState)},
          {QStringLiteral("worktreeState"), stateName(entry.worktreeState)},
          {QStringLiteral("staged"), entry.staged}};
}

}  // namespace

Result<GitStatusDto> GitStatusDto::fromJson(const QJsonObject& object) {
  const auto unknown = json::rejectUnknown(
      object, {QStringLiteral("repoPath"), QStringLiteral("branch"),
               QStringLiteral("upstream"), QStringLiteral("ahead"),
               QStringLiteral("behind"), QStringLiteral("clean"),
               QStringLiteral("detached"), QStringLiteral("entries"),
               QStringLiteral("lastCommitSummary"),
               QStringLiteral("lastCommitHash")});
  if (!unknown) return tl::unexpected(unknown.error());
  const auto repoPath = json::requiredString(
      object, QStringLiteral("repoPath"), 1, 4096);
  if (!repoPath) return tl::unexpected(repoPath.error());
  if (!QDir::isAbsolutePath(repoPath.value()) || repoPath->contains(QChar::Null)) {
    return tl::unexpected(makeError(ErrorCode::ValidationFailed,
                                    QStringLiteral("repoPath must be absolute")));
  }
  const auto branch = json::requiredString(
      object, QStringLiteral("branch"), 1, 512, true);
  if (!branch) return tl::unexpected(branch.error());
  const auto upstream = json::optionalString(
      object, QStringLiteral("upstream"), 512, true);
  if (!upstream) return tl::unexpected(upstream.error());
  const auto ahead = json::requiredInt(
      object, QStringLiteral("ahead"), 0, 100000000);
  if (!ahead) return tl::unexpected(ahead.error());
  const auto behind = json::requiredInt(
      object, QStringLiteral("behind"), 0, 100000000);
  if (!behind) return tl::unexpected(behind.error());
  const auto clean = json::requiredBool(object, QStringLiteral("clean"));
  if (!clean) return tl::unexpected(clean.error());
  const auto detached = json::requiredBool(object, QStringLiteral("detached"));
  if (!detached) return tl::unexpected(detached.error());
  if (!object.value(QStringLiteral("entries")).isArray()) {
    return tl::unexpected(makeError(ErrorCode::ValidationFailed,
                                    QStringLiteral("entries must be an array")));
  }
  const auto array = object.value(QStringLiteral("entries")).toArray();
  if (array.size() > 100000) {
    return tl::unexpected(makeError(ErrorCode::ValidationFailed,
                                    QStringLiteral("too many git entries")));
  }
  QVector<GitFileEntry> entries;
  entries.reserve(array.size());
  for (const auto& value : array) {
    if (!value.isObject()) {
      return tl::unexpected(makeError(ErrorCode::ValidationFailed,
                                      QStringLiteral("git entry must be an object")));
    }
    const auto entry = parseEntry(value.toObject());
    if (!entry) return tl::unexpected(entry.error());
    entries.append(entry.value());
  }
  if (clean.value() != entries.isEmpty()) {
    return tl::unexpected(makeError(ErrorCode::ValidationFailed,
                                    QStringLiteral("clean state conflicts with entries")));
  }
  const auto summary = json::optionalString(
      object, QStringLiteral("lastCommitSummary"), 1000, true);
  if (!summary) return tl::unexpected(summary.error());
  const auto hash = json::optionalString(
      object, QStringLiteral("lastCommitHash"), 64, true);
  if (!hash) return tl::unexpected(hash.error());
  return GitStatusDto{repoPath.value(), branch.value(), upstream.value(),
                      ahead.value(), behind.value(), clean.value(),
                      detached.value(), entries, summary.value(), hash.value()};
}

QJsonObject GitStatusDto::toJson() const {
  QJsonArray array;
  for (const auto& entry : entries) {
    array.append(entryJson(entry));
  }
  return {{QStringLiteral("repoPath"), repoPath},
          {QStringLiteral("branch"), branch},
          {QStringLiteral("upstream"), upstream},
          {QStringLiteral("ahead"), ahead},
          {QStringLiteral("behind"), behind},
          {QStringLiteral("clean"), clean},
          {QStringLiteral("detached"), detached},
          {QStringLiteral("entries"), array},
          {QStringLiteral("lastCommitSummary"), lastCommitSummary},
          {QStringLiteral("lastCommitHash"), lastCommitHash}};
}

}  // namespace aegis::dto
