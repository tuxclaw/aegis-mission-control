#pragma once

#include <QJsonObject>
#include <QString>
#include <QVector>

#include "core/result.h"
#include "dto/enums.h"

namespace aegis::dto {

struct GitFileEntry {
  QString path;
  GitFileState indexState = GitFileState::Untracked;
  GitFileState worktreeState = GitFileState::Untracked;
  bool staged = false;
};

struct GitStatusDto {
  QString repoPath;
  QString branch;
  QString upstream;
  int ahead = 0;
  int behind = 0;
  bool clean = true;
  bool detached = false;
  QVector<GitFileEntry> entries;
  QString lastCommitSummary;
  QString lastCommitHash;

  // Parses and fully validates repository status and every file entry.
  static Result<GitStatusDto> fromJson(const QJsonObject& object);

  // Serializes this repository status into its stable JSON representation.
  [[nodiscard]] QJsonObject toJson() const;
};

}  // namespace aegis::dto
