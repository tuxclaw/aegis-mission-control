#include "services/git_service.h"

#include <algorithm>
#include <atomic>

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>

#include "config/config_service.h"
#include "config/secret_store.h"
#include "core/async.h"

#if defined(AEGIS_HAS_LIBGIT2)
#include <git2.h>
#endif

namespace aegis {
namespace {

Result<QString> validatedRepoPath(ConfigService* config) {
  const auto configured = config->gitRepoPath();
  if (!configured) return tl::unexpected(configured.error());
  if (configured->isEmpty()) {
    return tl::unexpected(makeError(ErrorCode::GitOpenFailed,
                                    QStringLiteral("repository not configured")));
  }
  const QFileInfo info(configured.value());
  const auto canonical = info.canonicalFilePath();
  if (canonical.isEmpty() || !info.isDir()) {
    return tl::unexpected(makeError(ErrorCode::GitOpenFailed,
                                    QStringLiteral("repository unavailable")));
  }
  return canonical;
}

Result<void> validatePaths(const QStringList& paths) {
  if (paths.isEmpty() || paths.size() > 10000) {
    return tl::unexpected(makeError(ErrorCode::ValidationFailed,
                                    QStringLiteral("explicit path set is empty")));
  }
  for (const auto& path : paths) {
    const auto clean = QDir::cleanPath(path);
    if (path.isEmpty() || path.size() > 4096 || path.contains(QChar::Null) ||
        QDir::isAbsolutePath(path) || clean == QStringLiteral("..") ||
        clean.startsWith(QStringLiteral("../"))) {
      return tl::unexpected(makeError(ErrorCode::ValidationFailed,
                                      QStringLiteral("invalid repository path")));
    }
  }
  return {};
}

#if defined(AEGIS_HAS_LIBGIT2)

std::atomic_bool libgit2Initialized{false};

struct RepositoryDeleter {
  void operator()(git_repository* repository) const {
    git_repository_free(repository);
  }
};

using Repository = std::unique_ptr<git_repository, RepositoryDeleter>;

Result<Repository> openRepository(const QString& path) {
  static const auto initialized = [] {
    const auto result = git_libgit2_init();
    if (result >= 0) libgit2Initialized.store(true);
    return result;
  }();
  if (initialized < 0) {
    return tl::unexpected(makeError(ErrorCode::GitOpenFailed,
                                    QStringLiteral("libgit2 initialization failed")));
  }
  git_repository* raw = nullptr;
  if (git_repository_open_ext(&raw, path.toUtf8().constData(),
                              GIT_REPOSITORY_OPEN_NO_SEARCH, nullptr) < 0 ||
      raw == nullptr || git_repository_is_bare(raw) != 0 ||
      git_repository_workdir(raw) == nullptr) {
    if (raw != nullptr) git_repository_free(raw);
    return tl::unexpected(makeError(ErrorCode::GitOpenFailed,
                                    QStringLiteral("configured path is not a worktree")));
  }
  return Repository(raw);
}

dto::GitFileState stateFromStatus(unsigned int status, bool index) {
  const auto created = index ? GIT_STATUS_INDEX_NEW : GIT_STATUS_WT_NEW;
  const auto modified = index ? GIT_STATUS_INDEX_MODIFIED : GIT_STATUS_WT_MODIFIED;
  const auto deleted = index ? GIT_STATUS_INDEX_DELETED : GIT_STATUS_WT_DELETED;
  const auto renamed = index ? GIT_STATUS_INDEX_RENAMED : GIT_STATUS_WT_RENAMED;
  if ((status & GIT_STATUS_CONFLICTED) != 0) return dto::GitFileState::Conflicted;
  if ((status & created) != 0) return dto::GitFileState::New;
  if ((status & modified) != 0) return dto::GitFileState::Modified;
  if ((status & deleted) != 0) return dto::GitFileState::Deleted;
  if ((status & renamed) != 0) return dto::GitFileState::Renamed;
  if ((status & GIT_STATUS_IGNORED) != 0) return dto::GitFileState::Ignored;
  return dto::GitFileState::Untracked;
}

Result<dto::GitStatusDto> repositoryStatus(const QString& path) {
  auto repository = openRepository(path);
  if (!repository) return tl::unexpected(repository.error());
  dto::GitStatusDto result;
  result.repoPath = path;
  git_reference* head = nullptr;
  if (git_repository_head(&head, repository->get()) == 0 && head != nullptr) {
    result.detached = git_repository_head_detached(repository->get()) == 1;
    const auto shorthand = git_reference_shorthand(head);
    result.branch = result.detached ? QStringLiteral("detached HEAD")
                                    : QString::fromUtf8(shorthand ? shorthand : "");
    const auto target = git_reference_target(head);
    if (target != nullptr) {
      char oidBuffer[GIT_OID_HEXSZ + 1] = {};
      git_oid_tostr(oidBuffer, sizeof(oidBuffer), target);
      result.lastCommitHash = QString::fromLatin1(oidBuffer).left(12);
      git_commit* commit = nullptr;
      if (git_commit_lookup(&commit, repository->get(), target) == 0) {
        const auto summary = git_commit_summary(commit);
        result.lastCommitSummary = QString::fromUtf8(summary ? summary : "");
        git_commit_free(commit);
      }
    }
    git_reference* upstream = nullptr;
    if (!result.detached && git_branch_upstream(&upstream, head) == 0) {
      const auto upstreamName = git_reference_shorthand(upstream);
      result.upstream = QString::fromUtf8(upstreamName ? upstreamName : "");
      const auto localTarget = git_reference_target(head);
      const auto upstreamTarget = git_reference_target(upstream);
      size_t ahead = 0;
      size_t behind = 0;
      if (localTarget != nullptr && upstreamTarget != nullptr &&
          git_graph_ahead_behind(&ahead, &behind, repository->get(), localTarget,
                                 upstreamTarget) == 0) {
        result.ahead = static_cast<int>(std::min<size_t>(ahead, INT_MAX));
        result.behind = static_cast<int>(std::min<size_t>(behind, INT_MAX));
      }
      git_reference_free(upstream);
    }
    git_reference_free(head);
  } else {
    result.branch = QStringLiteral("unborn");
  }
  git_status_options options = GIT_STATUS_OPTIONS_INIT;
  options.show = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;
  options.flags = GIT_STATUS_OPT_INCLUDE_UNTRACKED |
                  GIT_STATUS_OPT_RECURSE_UNTRACKED_DIRS |
                  GIT_STATUS_OPT_RENAMES_HEAD_TO_INDEX |
                  GIT_STATUS_OPT_RENAMES_INDEX_TO_WORKDIR;
  git_status_list* list = nullptr;
  if (git_status_list_new(&list, repository->get(), &options) < 0) {
    return tl::unexpected(makeError(ErrorCode::GitOperationFailed,
                                    QStringLiteral("repository status failed")));
  }
  const auto count = git_status_list_entrycount(list);
  result.entries.reserve(static_cast<qsizetype>(count));
  for (size_t index = 0; index < count; ++index) {
    const auto* entry = git_status_byindex(list, index);
    if (entry == nullptr) continue;
    const char* rawPath = nullptr;
    if (entry->index_to_workdir != nullptr) {
      rawPath = entry->index_to_workdir->new_file.path;
    } else if (entry->head_to_index != nullptr) {
      rawPath = entry->head_to_index->new_file.path;
    }
    if (rawPath == nullptr) continue;
    const auto stagedMask = GIT_STATUS_INDEX_NEW | GIT_STATUS_INDEX_MODIFIED |
                            GIT_STATUS_INDEX_DELETED | GIT_STATUS_INDEX_RENAMED |
                            GIT_STATUS_INDEX_TYPECHANGE;
    result.entries.append({QString::fromUtf8(rawPath),
                           stateFromStatus(entry->status, true),
                           stateFromStatus(entry->status, false),
                           (entry->status & stagedMask) != 0});
  }
  git_status_list_free(list);
  result.clean = result.entries.isEmpty();
  return result;
}

int credentialCallback(git_credential** output, const char*,
                       const char* usernameFromUrl, unsigned int allowedTypes,
                       void* payload) {
  const auto* secret = static_cast<const QString*>(payload);
  const auto username = usernameFromUrl != nullptr ? usernameFromUrl : "git";
  if ((allowedTypes & GIT_CREDENTIAL_SSH_KEY) != 0 && secret != nullptr &&
      QFileInfo(*secret).isFile()) {
    return git_credential_ssh_key_new(output, username, nullptr,
                                      secret->toUtf8().constData(), nullptr);
  }
  if ((allowedTypes & GIT_CREDENTIAL_USERPASS_PLAINTEXT) != 0 &&
      secret != nullptr && !secret->isEmpty()) {
    return git_credential_userpass_plaintext_new(
        output, username, secret->toUtf8().constData());
  }
  if ((allowedTypes & GIT_CREDENTIAL_SSH_KEY) != 0) {
    return git_credential_ssh_key_from_agent(output, username);
  }
  return GIT_EAUTH;
}

Result<void> fetchRemote(git_repository* repository, const QString& remoteName,
                         const QString& credential) {
  git_remote* remote = nullptr;
  if (git_remote_lookup(&remote, repository, remoteName.toUtf8().constData()) < 0) {
    return tl::unexpected(makeError(ErrorCode::GitOperationFailed,
                                    QStringLiteral("configured remote not found")));
  }
  git_fetch_options options = GIT_FETCH_OPTIONS_INIT;
  options.callbacks.credentials = credentialCallback;
  options.callbacks.payload = const_cast<QString*>(&credential);
  const auto code = git_remote_fetch(remote, nullptr, &options, nullptr);
  git_remote_free(remote);
  if (code == GIT_EAUTH) {
    return tl::unexpected(makeError(ErrorCode::GitAuthFailed));
  }
  if (code < 0) {
    return tl::unexpected(makeError(ErrorCode::GitOperationFailed,
                                    QStringLiteral("remote fetch failed")));
  }
  return {};
}

#endif

template <typename T>
Result<T> libgitUnavailable() {
  return tl::unexpected(makeError(ErrorCode::GitOperationFailed,
                                  QStringLiteral("libgit2 unavailable")));
}

}  // namespace

GitService::GitService(ConfigService* config, SecretStore* secrets,
                       QObject* parent)
    : QObject(parent), config_(config), secrets_(secrets) {}

void GitService::shutdownLibrary() {
#if defined(AEGIS_HAS_LIBGIT2)
  if (libgit2Initialized.exchange(false)) git_libgit2_shutdown();
#endif
}

QFuture<Result<dto::GitStatusDto>> GitService::status() {
  const auto path = validatedRepoPath(config_);
  return async::run([path]() -> Result<dto::GitStatusDto> {
    if (!path) return tl::unexpected(path.error());
#if defined(AEGIS_HAS_LIBGIT2)
    return repositoryStatus(path.value());
#else
    return libgitUnavailable<dto::GitStatusDto>();
#endif
  });
}

QFuture<Result<void>> GitService::stage(QStringList explicitPaths) {
  const auto path = validatedRepoPath(config_);
  const auto valid = validatePaths(explicitPaths);
  return async::run([this, path, valid, explicitPaths = std::move(explicitPaths)]
                        () -> Result<void> {
    if (!path) return tl::unexpected(path.error());
    if (!valid) return tl::unexpected(valid.error());
#if defined(AEGIS_HAS_LIBGIT2)
    auto repository = openRepository(path.value());
    if (!repository) return tl::unexpected(repository.error());
    git_index* index = nullptr;
    if (git_repository_index(&index, repository->get()) < 0) {
      return tl::unexpected(makeError(ErrorCode::GitOperationFailed));
    }
    for (const auto& item : explicitPaths) {
      if (git_index_add_bypath(index, item.toUtf8().constData()) < 0) {
        git_index_free(index);
        return tl::unexpected(makeError(ErrorCode::GitOperationFailed,
                                        QStringLiteral("explicit stage failed")));
      }
    }
    const auto writeCode = git_index_write(index);
    git_index_free(index);
    if (writeCode < 0) return tl::unexpected(makeError(ErrorCode::GitOperationFailed));
    QMetaObject::invokeMethod(this, &GitService::repoChanged,
                              Qt::QueuedConnection);
    return {};
#else
    return libgitUnavailable<void>();
#endif
  });
}

QFuture<Result<void>> GitService::unstage(QStringList explicitPaths) {
  const auto path = validatedRepoPath(config_);
  const auto valid = validatePaths(explicitPaths);
  return async::run([this, path, valid, explicitPaths = std::move(explicitPaths)]
                        () -> Result<void> {
    if (!path) return tl::unexpected(path.error());
    if (!valid) return tl::unexpected(valid.error());
#if defined(AEGIS_HAS_LIBGIT2)
    auto repository = openRepository(path.value());
    if (!repository) return tl::unexpected(repository.error());
    git_object* target = nullptr;
    if (git_revparse_single(&target, repository->get(), "HEAD^{tree}") < 0) {
      return tl::unexpected(makeError(ErrorCode::GitOperationFailed));
    }
    QVector<QByteArray> storage;
    storage.reserve(explicitPaths.size());
    git_strarray paths{};
    paths.count = static_cast<size_t>(explicitPaths.size());
    paths.strings = new char*[paths.count];
    for (int index = 0; index < explicitPaths.size(); ++index) {
      storage.append(explicitPaths.at(index).toUtf8());
      paths.strings[index] = storage.last().data();
    }
    const auto code = git_reset_default(repository->get(), target, &paths);
    delete[] paths.strings;
    git_object_free(target);
    if (code < 0) return tl::unexpected(makeError(ErrorCode::GitOperationFailed));
    QMetaObject::invokeMethod(this, &GitService::repoChanged,
                              Qt::QueuedConnection);
    return {};
#else
    return libgitUnavailable<void>();
#endif
  });
}

QFuture<Result<QString>> GitService::commit(QString message,
                                             QStringList stagedPaths) {
  const auto path = validatedRepoPath(config_);
  const auto valid = validatePaths(stagedPaths);
  message = message.trimmed();
  return async::run([this, path, valid, message,
                     stagedPaths = std::move(stagedPaths)]() -> Result<QString> {
    if (!path) return tl::unexpected(path.error());
    if (!valid || message.isEmpty() || message.size() > 4096 ||
        message.contains(QChar::Null)) {
      return tl::unexpected(makeError(ErrorCode::ValidationFailed,
                                      QStringLiteral("invalid commit request")));
    }
#if defined(AEGIS_HAS_LIBGIT2)
    const auto current = repositoryStatus(path.value());
    if (!current) return tl::unexpected(current.error());
    QSet<QString> actual;
    for (const auto& entry : current->entries) {
      if (entry.staged) actual.insert(entry.path);
    }
    if (actual != QSet<QString>(stagedPaths.begin(), stagedPaths.end())) {
      return tl::unexpected(makeError(
          ErrorCode::ValidationFailed,
          QStringLiteral("staged paths changed before confirmation")));
    }
    auto repository = openRepository(path.value());
    if (!repository) return tl::unexpected(repository.error());
    git_index* index = nullptr;
    if (git_repository_index(&index, repository->get()) < 0) {
      return tl::unexpected(makeError(ErrorCode::GitOperationFailed));
    }
    git_oid treeId{};
    if (git_index_write_tree(&treeId, index) < 0) {
      git_index_free(index);
      return tl::unexpected(makeError(ErrorCode::GitOperationFailed));
    }
    git_index_free(index);
    git_tree* tree = nullptr;
    git_signature* signature = nullptr;
    git_commit* parent = nullptr;
    git_oid commitId{};
    if (git_tree_lookup(&tree, repository->get(), &treeId) < 0 ||
        git_signature_default(&signature, repository->get()) < 0) {
      git_tree_free(tree);
      return tl::unexpected(makeError(ErrorCode::GitOperationFailed,
                                      QStringLiteral("git identity unavailable")));
    }
    git_reference* head = nullptr;
    if (git_repository_head(&head, repository->get()) == 0) {
      const auto target = git_reference_target(head);
      if (target != nullptr) git_commit_lookup(&parent, repository->get(), target);
      git_reference_free(head);
    }
    const git_commit* parents[] = {parent};
    const auto code = git_commit_create(&commitId, repository->get(), "HEAD",
                                        signature, signature, "UTF-8",
                                        message.toUtf8().constData(), tree,
                                        parent == nullptr ? 0 : 1, parents);
    git_commit_free(parent);
    git_signature_free(signature);
    git_tree_free(tree);
    if (code < 0) return tl::unexpected(makeError(ErrorCode::GitOperationFailed));
    char buffer[GIT_OID_HEXSZ + 1] = {};
    git_oid_tostr(buffer, sizeof(buffer), &commitId);
    QMetaObject::invokeMethod(this, &GitService::repoChanged,
                              Qt::QueuedConnection);
    return QString::fromLatin1(buffer).left(12);
#else
    return libgitUnavailable<QString>();
#endif
  });
}

QFuture<Result<void>> GitService::pull() {
  const auto path = validatedRepoPath(config_);
  const auto remoteName = config_->gitRemoteName();
  return async::flatten(this, secrets_->read(QStringLiteral("git.credential"))
      .then(this, [path, remoteName](const Result<QString>& credential) {
        return async::run([path, remoteName, credential]() -> Result<void> {
          if (!path) return tl::unexpected(path.error());
          if (!remoteName) return tl::unexpected(remoteName.error());
          if (!credential) return tl::unexpected(makeError(ErrorCode::GitAuthFailed));
#if defined(AEGIS_HAS_LIBGIT2)
          auto repository = openRepository(path.value());
          if (!repository) return tl::unexpected(repository.error());
          const auto fetched = fetchRemote(repository->get(), remoteName.value(),
                                           credential.value());
          if (!fetched) return tl::unexpected(fetched.error());
          git_reference* head = nullptr;
          git_reference* upstream = nullptr;
          if (git_repository_head(&head, repository->get()) < 0 ||
              git_branch_upstream(&upstream, head) < 0) {
            git_reference_free(head);
            return tl::unexpected(makeError(ErrorCode::GitOperationFailed));
          }
          git_annotated_commit* target = nullptr;
          const auto targetId = git_reference_target(upstream);
          if (targetId == nullptr ||
              git_annotated_commit_lookup(&target, repository->get(), targetId) < 0) {
            git_reference_free(upstream);
            git_reference_free(head);
            return tl::unexpected(makeError(ErrorCode::GitOperationFailed));
          }
          const git_annotated_commit* targets[] = {target};
          git_merge_analysis_t analysis{};
          git_merge_preference_t preference{};
          git_merge_analysis(&analysis, &preference, repository->get(), targets, 1);
          Result<void> result;
          if ((analysis & GIT_MERGE_ANALYSIS_UP_TO_DATE) != 0) {
            result = {};
          } else if ((analysis & GIT_MERGE_ANALYSIS_FASTFORWARD) == 0) {
            result = tl::unexpected(makeError(ErrorCode::GitConflict));
          } else {
            git_reference* updated = nullptr;
            if (git_reference_set_target(&updated, head, targetId,
                                         "AEGIS ff-only pull") < 0) {
              result = tl::unexpected(makeError(ErrorCode::GitOperationFailed));
            } else {
              git_checkout_options checkout = GIT_CHECKOUT_OPTIONS_INIT;
              checkout.checkout_strategy = GIT_CHECKOUT_SAFE;
              if (git_repository_set_head(repository->get(),
                                          git_reference_name(updated)) < 0 ||
                  git_checkout_head(repository->get(), &checkout) < 0) {
                result = tl::unexpected(makeError(ErrorCode::GitConflict));
              }
              git_reference_free(updated);
            }
          }
          git_annotated_commit_free(target);
          git_reference_free(upstream);
          git_reference_free(head);
          return result;
#else
          return libgitUnavailable<void>();
#endif
        });
      }))
      .then(this, [this](const Result<void>& result) {
        if (result) emit repoChanged();
        return result;
      });
}

QFuture<Result<void>> GitService::push() {
  const auto path = validatedRepoPath(config_);
  const auto remoteName = config_->gitRemoteName();
  return async::flatten(this, secrets_->read(QStringLiteral("git.credential"))
      .then(this, [path, remoteName](const Result<QString>& credential) {
        return async::run([path, remoteName, credential]() -> Result<void> {
          if (!path) return tl::unexpected(path.error());
          if (!remoteName) return tl::unexpected(remoteName.error());
          if (!credential) return tl::unexpected(makeError(ErrorCode::GitAuthFailed));
#if defined(AEGIS_HAS_LIBGIT2)
          auto repository = openRepository(path.value());
          if (!repository) return tl::unexpected(repository.error());
          git_reference* head = nullptr;
          if (git_repository_head(&head, repository->get()) < 0 ||
              !git_reference_is_branch(head)) {
            git_reference_free(head);
            return tl::unexpected(makeError(ErrorCode::GitOperationFailed));
          }
          const auto branch = QByteArray(git_reference_shorthand(head));
          git_reference_free(head);
          git_remote* remote = nullptr;
          if (git_remote_lookup(&remote, repository->get(),
                                remoteName->toUtf8().constData()) < 0) {
            return tl::unexpected(makeError(ErrorCode::GitOperationFailed));
          }
          const auto refspecBytes = QByteArray("refs/heads/") + branch +
                                    ":refs/heads/" + branch;
          char* refspecString = refspecBytes.data();
          git_strarray refspec{&refspecString, 1};
          git_push_options options = GIT_PUSH_OPTIONS_INIT;
          options.callbacks.credentials = credentialCallback;
          options.callbacks.payload = const_cast<QString*>(&credential.value());
          const auto code = git_remote_push(remote, &refspec, &options);
          git_remote_free(remote);
          if (code == GIT_EAUTH) return tl::unexpected(makeError(ErrorCode::GitAuthFailed));
          if (code < 0) return tl::unexpected(makeError(ErrorCode::GitOperationFailed));
          return {};
#else
          return libgitUnavailable<void>();
#endif
        });
      }))
      .then(this, [this](const Result<void>& result) {
        if (result) emit repoChanged();
        return result;
      });
}

}  // namespace aegis
