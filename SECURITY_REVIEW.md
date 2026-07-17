# AEGIS Mission Control — Security Review

> **Reviewer:** Jeff 🛡️ (security architect)  
> **Date:** 2026-07-17  
> **Spec version:** 1.0  
> **Codebase commit:** HEAD at review time  
> **Scope:** Full source tree — `src/`, `qml/`, `resources/`

---

## Summary

The implementation is **structurally sound**. The core security architecture — credential containment, no webview, arg-array-only process execution, libgit2, PathGuard, atomic writes, typed DTOs, and structured errors — is correctly realized. The old Tauri-era BLOCKERs are all closed by construction.

There are **2 blockers** (both QML↔C++ binding mismatches that will crash at runtime), **4 warnings**, and **5 suggestions**. No credential leakage, no shell injection, no XSS surface was found.

---

## 🔴 BLOCKER

### B1 — QML references undefined `gatewayTokenSet` property

**File:** `qml/views/SettingsView.qml:189`  
**File:** `src/controllers/settings_controller.h`

```qml
placeholderText: settings.gatewayTokenSet ? qsTr("Credential set") : qsTr("Credential not set")
```

`SettingsController` has no `Q_PROPERTY` named `gatewayTokenSet`. QML will log a binding error and the placeholder will always show "Credential not set" regardless of actual state.

**Fix:** Add a `Q_PROPERTY(bool gatewayTokenSet READ gatewayTokenSet NOTIFY gatewayTokenSetChanged)` that calls `SecretStore::has("gateway.token")` and delivers the result asynchronously. Same pattern for `gitCredentialSet`.

---

### B2 — QML references undefined `gitCredentialSet` property

**File:** `qml/views/SettingsView.qml:275`

```qml
placeholderText: settings.gitCredentialSet ? qsTr("Credential set") : qsTr("Credential not set")
```

Same issue as B1. `gitCredentialSet` does not exist on `SettingsController`.

**Fix:** Same as B1 — add the async `has()`-backed property. A single helper method `hasSecret(key)` on the controller that returns a `bool` property updated via signal would cover both.

---

## 🟡 WARNING

### W1 — QML references `settings.ollamaUrl` but property is `ollamaBaseUrl`

**File:** `qml/views/SettingsView.qml:35`  
**File:** `src/controllers/settings_controller.h`

```qml
ollamaUrlField.text = settings.ollamaUrl;  // ← wrong name
```

The controller exposes `ollamaBaseUrl` (Q_PROPERTY), not `ollamaUrl`. This binding will silently fail and the Ollama URL field will be empty on load.

**Fix:** Change QML to `settings.ollamaBaseUrl`, or rename the Q_PROPERTY to `ollamaUrl` for consistency.

---

### W2 — `PathGuard::create` does not resolve symlinks in root path via `canonicalFilePath()`

**File:** `src/core/path_guard.cpp:15-23`

```cpp
Result<PathGuard> PathGuard::create(const QString& rootPath, quint64 maxFileBytes) {
  const QFileInfo rootInfo(rootPath);
  const auto canonical = rootInfo.canonicalFilePath();
  if (canonical.isEmpty() || !rootInfo.isDir()) {
```

The implementation **does** call `canonicalFilePath()` on the root, which is correct. However, `isContained()` then compares against `rootCanonical_` which is `QDir::cleanPath(canonical)`:

```cpp
return PathGuard(QDir::cleanPath(canonical), maxFileBytes);
```

`QDir::cleanPath` on an already-canonical path is a no-op, so this is safe. **However**, the spec (§6.4) says `rootCanonical` should be the result of `canonicalFilePath()` — the extra `cleanPath` wrapper is redundant and could mask a future bug if `canonical` were somehow not clean. Not exploitable today, but worth tightening.

**Fix:** Use `canonical` directly without the `QDir::cleanPath` wrapper, since `canonicalFilePath()` already returns a clean absolute path.

---

### W3 — `CreativeService::generateOllama` reads entire streamed response into memory before parsing

**File:** `src/services/creative_service.cpp:92-130`

The Ollama streaming handler collects the full response via `QFutureWatcher<Result<QByteArray>>`, then splits by `\n` and parses each line. This means the entire response (up to 16 MiB) is buffered in memory before any chunks reach the UI. The `chunk()` signal only fires after the full response is received, defeating the purpose of streaming.

**Fix:** Use a line-buffered `QNetworkReply` with `readyRead` signal to emit chunks incrementally, similar to how `HttpClient::RequestOperation` already handles streaming internally.

---

### W4 — `CreativeRequest::fromJson` validates temperature/maxTokens with absurdly wide ranges before clamping

**File:** `src/dto/creative_dto.cpp:34-38`

```cpp
const auto rawTemperature = json::requiredNumber(
    object, QStringLiteral("temperature"), -1000000.0, 1000000.0);
const auto rawMaxTokens = json::requiredInt(
    object, QStringLiteral("maxTokens"), -1000000, 1000000);
```

Then clamped: `qBound(0.0, rawTemperature.value(), 2.0)` and `qBound(1, rawMaxTokens.value(), 8192)`.

The spec (§3.9) says `temperature` is 0..2 and `maxTokens` is 1..8192. The fromJson should reject out-of-range values at the boundary, not accept absurd values and silently clamp. A caller sending `temperature: 999999` gets `2.0` with no error — this is silent coercion, which the spec explicitly prohibits ("never a silent default").

**Fix:** Validate with the actual ranges in `fromJson`: `requiredNumber(..., 0.0, 2.0)` and `requiredInt(..., 1, 8192)`. Remove the `qBound` clamping.

---

## 🔵 SUGGESTION

### S1 — Add `Q_INVOKABLE bool hasGatewayToken()` / `hasGitCredential()` as stopgap for B1/B2

While the full async property approach (B1/B2 fix) is preferred, a synchronous `Q_INVOKABLE bool hasSecret(const QString& key)` on `SettingsController` that reads the secret store on the GUI thread would unblock the QML immediately. The async property can follow. Note: `SecretStore::read()` is async, but `has()` could be implemented as a synchronous check if the keychain backend supports it, or the controller could cache the last-known state.

---

### S2 — `git_libgit2_init()` thread-safety relies on static local

**File:** `src/services/git_service.cpp:51`

```cpp
static const auto initialized = git_libgit2_init();
```

C++11 guarantees thread-safe initialization of function-local statics, so this is correct. However, `git_libgit2_init()` is reference-counted and should be paired with `git_libgit2_shutdown()` at process exit. Without the shutdown call, libgit2 may leak resources on exit. Not a security issue, but worth adding for correctness.

**Fix:** Add `git_libgit2_shutdown()` in `AppContext::~AppContext()` or via `atexit`.

---

### S3 — `PackageService` does not validate output line count

**File:** `src/services/package_service.cpp:37-42`

The output byte cap (8 MiB) is enforced, but there's no explicit line-count cap. A pathological package database could produce millions of tiny lines within the byte cap, causing the `QVector` to grow unbounded.

**Fix:** Add a line-count cap (e.g., 100,000) in the parsing loop.

---

### S4 — `CalendarStore::loadLocked` reads entire file into memory

**File:** `src/services/calendar_store.cpp:106`

```cpp
const auto document = QJsonDocument::fromJson(file.readAll(), &parseError);
```

The 32 MiB file-size check happens first, so the read is bounded. But `file.readAll()` loads the entire file into a `QByteArray` before parsing. For the 10,000-event cap with typical event sizes, this is fine. Just noting the pattern.

---

### S5 — No test for the "no controller exposes secret-named properties" regression

The spec (§10.2) calls for a reflection test that asserts no controller meta-object exposes a method/property matching a secret denylist (`token`, `secret`, `password`, `bearer`, `credential`). This test is not present in the `tests/` directory.

**Fix:** Add `tst_security_regression.cpp` with a `QMetaObject` iteration test over all registered controllers.

---

## ✅ PASS

### Credential Containment (SPEC §6, T1-T7)

- ✅ **Gateway token confined to GatewayService.** `SecretStore::read("gateway.token")` is called only inside `GatewayService::get()` and `GatewayService::postJson()`. The token is used to construct the `Authorization` header inside `authorizedRequest()` and never returned, logged, or exposed.
- ✅ **No `get_gateway_token` equivalent.** No method in `GatewayService` or any other class returns the token to any caller.
- ✅ **Tokens never in QML properties.** `SettingsController` exposes `setGatewayToken(QString)` (write-only Q_INVOKABLE) and `setGitCredential(QString)` (write-only). No read-back property exists.
- ✅ **Tokens never in DTOs.** All DTOs are plain data structs; none carry credential fields.
- ✅ **Tokens never in logs.** `logging::registerSecret()` is called in `SecretStore::read()` and `SecretStore::write()`, and the redacting message handler replaces registered secrets and bearer patterns before emission.
- ✅ **SecretStore fails closed.** When QtKeychain is unavailable or entry is absent, `read()` returns `MissingToken` error. No fallback credential exists.
- ✅ **No compiled-in credential.** No literal token, default password, or fallback secret exists anywhere in the source tree.

### Input Validation

- ✅ **All DTO `fromJson()` methods validate every field.** `json_validation.h` provides typed validators with range checks: `requiredString` (min/max length, NUL rejection, surrogate-pair validation), `requiredInt` (range), `requiredUnsigned` (range + JSON integer precision), `requiredNumber` (finite + range), `requiredDateTime` (ISO 8601 + UTC enforcement), `requiredBool`, `rejectUnknown` (strict allowlist).
- ✅ **Size caps on arrays:** Git entries (100,000), calendar events (10,000), CLI output items (100,000).
- ✅ **Size caps on strings:** All string fields have explicit max lengths (128–16 MiB depending on context).
- ✅ **PathGuard correctly implemented:**
  - Canonicalization via `QFileInfo::canonicalFilePath()` ✅
  - Containment check (`canonical == root || canonical.startsWith(root + "/")`) ✅
  - Symlink escape rejection via per-component walk ✅
  - `.md` suffix enforcement for memory reads ✅
  - Size cap enforcement ✅
  - NUL byte rejection ✅
  - Absolute path rejection ✅
  - `isFile()` check ✅

### Process Security

- ✅ **All QProcess calls use arg arrays.** `OpenClawCli::run()` takes `QStringList args` and calls `process.setProgram()` + `process.setArguments()`. `PackageService::list()` calls `process.start(command->first(), command->mid(1))`.
- ✅ **No `/bin/sh -c` anywhere.** Grep confirms zero matches.
- ✅ **CLI invocations don't interpolate user input.** Agent list, cron list, and model list use fixed subcommands (`agents list --json`, `cron list --json`, `models list --json`). CronService uses validated job IDs (charset + length).
- ✅ **Program path validated.** `ConfigService::openclawBinary()` resolves the path, checks `isFile()` + `isExecutable()`, and returns `canonicalFilePath()`.
- ✅ **Output byte cap enforced.** `OpenClawCli::run()` monitors cumulative stdout+stderr and kills the process on overflow.
- ✅ **Timeout enforced.** `QDeadlineTimer` kills the process on expiry.

### Git Safety (SPEC §5.9)

- ✅ **libgit2 used, not shell git.** All git operations use the `git2` C API (`git_repository_open_ext`, `git_index_add_bypath`, `git_commit_create`, `git_remote_fetch`, `git_remote_push`).
- ✅ **Explicit-path staging only.** `GitService::stage()` iterates over the caller-provided `QStringList` and calls `git_index_add_bypath()` per path. No `git add -A` equivalent exists.
- ✅ **Commit validates staged set.** `GitService::commit()` re-reads the index and compares the actual staged set against the caller's declared set, rejecting mismatches.
- ✅ **Two-phase confirm for destructive ops.** `GitController` emits `confirmRequested` for commit/pull/push; QML shows `ConfirmDialog`; on accept, QML calls `confirmCommit()`/`confirmPull()`/`confirmPush()`.
- ✅ **Repo path from config, not hard-coded.** `ConfigService::gitRepoPath()` reads from `QSettings`; empty/unset → "configure repository" state.
- ✅ **ff-only pull.** `GitService::pull()` uses `git_merge_analysis()` and rejects non-fast-forward with `GitConflict`.
- ✅ **Credentials from SecretStore.** `git.credential` read via `SecretStore`; used in credential callback; never logged or surfaced.
- ✅ **Path validation.** `validatePaths()` rejects absolute paths, NUL bytes, `..` traversal, empty paths, and paths > 4096 chars.

### Data Integrity

- ✅ **Atomic writes.** `AtomicFile::write()` uses `QSaveFile` (temp + write + commit/rollback). `QSaveFile::commit()` does the atomic rename. `setDirectWriteFallback(false)` prevents in-place writes on failure.
- ✅ **Serialized mutations.** `CalendarStore` uses `QMutex` to serialize load-then-write operations.
- ✅ **Typed schemas.** All DTOs have explicit C++ struct types with validated `fromJson()`. No `QJsonValue` used as a bag across boundaries.
- ✅ **Error propagation.** Services return `Result<T>` (tl::expected). Controllers check `!result` and surface `error.userMessage` via signals. No silent defaults.
- ✅ **Parse error prevents overwrite.** `CalendarStore::loadLocked()` returns error on parse failure; the caller propagates it without writing.

### Network Security

- ✅ **HttpClient has connect timeout, total timeout, and response-size cap.** `HttpRequestOptions` struct with defaults: 5s connect, 30s total, 8 MiB cap. Validated in `validOptions()`.
- ✅ **No hard-coded URLs.** Gateway URL from `ConfigService::gatewayBaseUrl()` (QSettings). Ollama URL from `ConfigService::ollamaBaseUrl()`.
- ✅ **TLS enforced for non-loopback.** `ConfigService::validUrl()` rejects `http://` for non-loopback hosts. Loopback exception for localhost/127.x/::1.
- ✅ **Reusable managed client.** Single `QNetworkAccessManager` in `HttpClient`, shared across all services.
- ✅ **Content-length pre-check.** `RequestOperation` checks `Content-Length` header against cap before downloading.
- ✅ **Streaming size cap.** Incremental `readyRead` handler checks cumulative buffer size against cap.
- ✅ **Retry with backoff.** `RequestOperation` retries on 5xx/timeout/unreachable up to `maxRetries` with delay.

### Webview/XSS Surface

- ✅ **No webview anywhere.** Grep confirms zero `WebView`, `WebEngine`, `WebEngineView` imports in QML. The entire UI is native Qt Quick rendered on the GPU.
- ✅ **No HTML rendering.** No `setHtml`, `loadHtml`, `innerHTML`, or `dangerouslySetInnerHTML` equivalent.
- ✅ **Memory content rendered as text.** `MemoryController::currentContent` is a plain `QString` displayed in a QML `Text` item. No markdown-to-HTML conversion.
- ✅ **Creative output rendered as text.** `CreativeController::output` is a plain `QString`.

### Error Handling

- ✅ **AegisError enum with stable codes.** 24 `ErrorCode` values, each with a `stableCode()` string identifier and a `safeMessageFor()` human-readable message.
- ✅ **`userMessage` never contains paths, tokens, or stderr.** `safeMessageFor()` returns generic messages like "A required credential is not configured." — no interpolation of dynamic values.
- ✅ **`debugContext` only in logs.** Controllers access `error.userMessage` and `error.retryable` only. `debugContext` is not exposed via any Q_PROPERTY or signal.
- ✅ **Redaction filter installed.** `logging::installRedactingMessageHandler()` is called in `main()` before any service construction. Strips bearer tokens, Authorization headers, API key assignments, and registered secret values.

### Build & Dependency Hygiene

- ✅ **No npm/RustSec/web stack.** Pure C++20 + Qt 6 + libgit2 + QtKeychain. No JavaScript bundler, no Node.js runtime, no web dependencies.
- ✅ **Warnings-as-errors on core library.** `AegisWarnings.cmake` applies `-Wall -Wextra -Werror` to `aegis_core`.
- ✅ **ASAN preset available.** CMake `Debug+ASAN` preset for address/UB sanitizer coverage.

---

## Fix Priority

| ID | Severity | Description | Effort |
|----|----------|-------------|--------|
| B1 | 🔴 BLOCKER | `gatewayTokenSet` property missing | Small — add Q_PROPERTY + async has() |
| B2 | 🔴 BLOCKER | `gitCredentialSet` property missing | Small — same pattern as B1 |
| W1 | 🟡 WARNING | `ollamaUrl` vs `ollamaBaseUrl` name mismatch | Trivial — one-line QML fix |
| W2 | 🟡 WARNING | Redundant `cleanPath` on already-canonical root | Trivial — remove wrapper |
| W3 | 🟡 WARNING | Ollama streaming buffers full response | Medium — redesign as line-buffered |
| W4 | 🟡 WARNING | Temperature/maxTokens silently clamped | Small — tighten fromJson ranges |
| S1 | 🔵 SUGGESTION | Synchronous `hasSecret()` stopgap | Small |
| S2 | 🔵 SUGGESTION | Add `git_libgit2_shutdown()` | Trivial |
| S3 | 🔵 SUGGESTION | Package output line-count cap | Trivial |
| S4 | 🔵 SUGGESTION | CalendarStore full-file read pattern | Informational |
| S5 | 🔵 SUGGESTION | Security regression reflection test | Medium — new test file |

---

## Verdict

The implementation faithfully realizes the spec's security model. The two blockers are QML binding mismatches (missing properties), not architectural flaws — they will cause runtime warnings and degraded UX but no security exposure. All six original BLOCKER vulnerabilities from the old Tauri app are closed by construction.

**Recommendation:** Fix B1, B2, and W1 before the first build. W3 and W4 before the first release. Everything else is polish.
