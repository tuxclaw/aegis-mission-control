# AEGIS Mission Control — Frozone Deep Review

**Reviewer:** Frozone 🧊  
**Reviewed branch/commit:** `main` at `ad96183`  
**Review date:** 2026-07-17  
**Scope:** `SPEC.md`, `UI_SPEC.md`, `.context/decisions.md`, all CMake, C++, QML, resources, and tests in the repository.

## Executive summary

**Verdict: FAIL — not merge ready.**

The project has a sound intended boundary—QML receives controllers, services own platform access, secrets use the OS keychain, CLI processes use argument arrays, memory paths are canonicalized, and calendar writes use `QSaveFile`. Those foundations are materially better than the retired Tauri design.

The current integration is not safe to merge, however. Several QML views call properties or signals that do not exist, destructive-operation confirmation handshakes stop before the operation, the calendar UI emits color values rejected by its DTO, and Creative is neither streaming nor hard-cancellable. Most seriously, the fast-forward pull implementation advances the branch ref before safe checkout succeeds, so a checkout conflict can leave the ref and worktree inconsistent. The mandatory credential/Git/security regression suites from the technical specification are absent.

Finding counts below count finding groups, not individual file references:

- 🔴 **10 blockers**
- 🟡 **13 warnings**
- 🔵 **3 suggestions**
- ✅ **9 passes**

### Verification performed

- Fresh Debug configure/build with `BUILD_TESTING=ON`: **passed for `aegis_core` and tests** with warnings treated as errors.
- `ctest --output-on-failure`: **7/7 passed** (`PathGuard`, `AtomicFile`, `AegisError`, `HttpClient`, `CalendarStore`, `MemoryService`, OpenClaw CLI parsing).
- The application executable was **not built** because Qt Quick development modules are unavailable in the review environment.
- The reviewed build also lacked libgit2 and QtKeychain, so their real implementations were compiled out and could not be executed.
- `qmllint`, `clang-tidy`, and `cppcheck` executables were unavailable. The integration failures below were established by matching QML bindings/handlers against controller meta-object declarations and implementations.
- Source searches found no shell execution, `git add -A`, or hard-coded QML colors outside the Theme singleton.

## 🔴 BLOCKER — must fix before merge

### B1. Calendar create and update always submit an invalid color token

The UI uses `accent`, `ok`, `warn`, `alert`, and `accentDim`; the DTO accepts only `cyan`, `blue`, `violet`, `green`, `amber`, `red`, `magenta`, and `slate`. Both create and update round-trip through DTO validation, so every event save initiated by this view fails.

Fix by defining one shared calendar-color contract and testing the controller/view DTO boundary.

```json
{ "verdict": "blocker", "issues": [ { "severity": "blocker", "file": "qml/views/CalendarView.qml", "line": 15, "msg": "The default and selectable color tokens do not belong to CalendarEventDto's accepted palette." }, { "severity": "blocker", "file": "src/dto/calendar_event_dto.cpp", "line": 47, "msg": "DTO validation rejects every color token emitted by CalendarView." } ], "reviewer": "frozone" }
```

### B2. Calendar and Git confirmation handshakes are disconnected

The views display their own confirmation dialogs. On acceptance they call `deleteEvent`, `commit`, `pull`, or `push`, but those controller methods only emit a second `confirmRequested` signal. Neither view handles that signal or calls the corresponding `confirm*` method, so deletion, commit, pull, and push never execute. Busy/saving state can remain stuck.

The controller-side `confirmPull()` and `confirmPush()` methods also have no pending-action token, so arbitrary QML can invoke them directly. Make the controller the authoritative confirmation state machine: request an operation, expose the pending action, and only execute a matching one-shot confirmation.

```json
{ "verdict": "blocker", "issues": [ { "severity": "blocker", "file": "qml/views/CalendarView.qml", "line": 499, "msg": "Dialog acceptance calls deleteEvent(), which only requests another confirmation; confirmDeleteEvent() is never called." }, { "severity": "blocker", "file": "src/controllers/calendar_controller.cpp", "line": 11, "msg": "Delete is split across request and confirm methods without a connected QML confirmation bridge." }, { "severity": "blocker", "file": "qml/views/GitView.qml", "line": 348, "msg": "Dialog acceptance calls request methods, not confirmCommit/confirmPull/confirmPush." }, { "severity": "blocker", "file": "src/controllers/git_controller.cpp", "line": 44, "msg": "Commit/pull/push only emit confirmRequested, which GitView does not handle." } ], "reviewer": "frozone" }
```

### B3. Settings uses nonexistent/read-only properties and reset deadlocks the view

`loadSettings()` reads `settings.ollamaUrl`, but the property is named `ollamaBaseUrl`. Save assigns the same nonexistent property and attempts to write the `CONSTANT` `gitPullMode`; QML will raise a runtime type error before `settings.save()` is reached. The credential status bindings also reference properties that do not exist.

Reset sets `busy = true` and calls a controller method that only mutates memory and emits `settingsChanged`; it neither saves nor emits a toast/error, so the full-screen loading state never clears. Several reset values are invalid empty values under the current validators.

```json
{ "verdict": "blocker", "issues": [ { "severity": "blocker", "file": "qml/views/SettingsView.qml", "line": 41, "msg": "Reads nonexistent settings.ollamaUrl instead of ollamaBaseUrl." }, { "severity": "blocker", "file": "qml/views/SettingsView.qml", "line": 75, "msg": "Attempts to assign read-only CONSTANT gitPullMode; save execution stops before persistence." }, { "severity": "blocker", "file": "qml/views/SettingsView.qml", "line": 78, "msg": "Writes nonexistent settings.ollamaUrl." }, { "severity": "blocker", "file": "qml/views/SettingsView.qml", "line": 491, "msg": "Reset enters busy state but has no completion path." }, { "severity": "blocker", "file": "src/controllers/settings_controller.cpp", "line": 16, "msg": "resetDefaults neither persists nor emits success/error and supplies values rejected by existing validators." } ], "reviewer": "frozone" }
```

### B4. Memory and Creative views listen to signals their controllers do not expose

Memory sets `busy` before refresh and expects `filesChanged`; its controller updates the model but exposes no such signal. The loading overlay therefore remains indefinitely. Creative emits `failed(requestId, message)`, while its view listens for `errorRaised`; generation failures never reach the error UI. `onToast` handlers in Memory, Creative, and Packages also target nonexistent signals.

This would have been caught by loading each view under a QML engine or by a meta-object contract test.

```json
{ "verdict": "blocker", "issues": [ { "severity": "blocker", "file": "qml/views/MemoryView.qml", "line": 45, "msg": "onFilesChanged targets a signal that MemoryController does not declare; refresh leaves busy true." }, { "severity": "blocker", "file": "src/controllers/memory_controller.cpp", "line": 8, "msg": "Successful list refresh updates the model without a view-visible completion signal." }, { "severity": "blocker", "file": "qml/views/CreativeView.qml", "line": 43, "msg": "Listens for errorRaised, but CreativeController exposes failed(requestId,message)." }, { "severity": "blocker", "file": "src/controllers/creative_controller.h", "line": 34, "msg": "Creative failures use an incompatible signal contract and omit retryability." } ], "reviewer": "frozone" }
```

### B5. Creative streaming and cancellation are simulated after full buffering

Ollama requests set `stream: true`, but `HttpClient::request()` buffers the entire response; NDJSON lines are parsed and emitted only when the future completes. Cancel removes local state but does not abort the network reply. Gateway streaming wraps a buffered `postJson()`, emits the entire compact JSON object as one chunk, and records cancellation without aborting the request. A cancelled five-minute request therefore continues consuming network/resources, and Gateway output is raw JSON rather than extracted model text.

Implement a streaming `QNetworkReply` path with incremental caps/parsing and an owned reply handle that `cancel()` aborts immediately. Validate stream frames and extract only the documented text delta.

```json
{ "verdict": "blocker", "issues": [ { "severity": "blocker", "file": "src/services/creative_service.cpp", "line": 103, "msg": "Ollama response is buffered to completion before stream frames are parsed or emitted." }, { "severity": "blocker", "file": "src/services/creative_service.cpp", "line": 70, "msg": "Ollama cancellation does not abort the in-flight HTTP request." }, { "severity": "blocker", "file": "src/services/gateway_service.cpp", "line": 132, "msg": "Gateway beginStream uses buffered postJson and emits one compact JSON object, not incremental content." }, { "severity": "blocker", "file": "src/services/gateway_service.cpp", "line": 158, "msg": "cancelStream only records an ID and leaves the network request running." } ], "reviewer": "frozone" }
```

### B6. Fast-forward pull can advance the branch ref before checkout fails

`git_reference_set_target()` mutates the current branch first. Only afterward does `git_checkout_head(...GIT_CHECKOUT_SAFE)` run. If local worktree changes cause safe checkout to fail, the method reports a conflict but the branch ref has already moved to the fetched commit, leaving HEAD/ref and the worktree out of sync.

Perform a dry-run/safe checkout feasibility check before ref mutation, or use a transaction/rollback that preserves the original OID on every failure. Add a regression test with dirty tracked files. Pull/push also have no timeout or cancellation path despite the required 120-second cap.

```json
{ "verdict": "blocker", "issues": [ { "severity": "blocker", "file": "src/services/git_service.cpp", "line": 427, "msg": "The local branch ref is advanced before safe checkout succeeds, risking ref/worktree inconsistency." }, { "severity": "blocker", "file": "src/services/git_service.cpp", "line": 386, "msg": "Pull has no timeout or cancellation mechanism." }, { "severity": "blocker", "file": "src/services/git_service.cpp", "line": 456, "msg": "Push has no timeout or cancellation mechanism." } ], "reviewer": "frozone" }
```

### B7. Synchronous QSettings persistence runs on the GUI thread

`SettingsController::save()` is a direct QML invocation and loops through 19 synchronous `ConfigService::setValue()` calls. Each call performs `QSettings::setValue()` and `sync()`. This violates the mandatory “no filesystem I/O on the GUI thread” rule and can visibly freeze on slow or unhealthy storage. Many config reads are also performed synchronously before worker dispatch.

Validate an immutable settings snapshot first, then persist it atomically on a worker. Return one future/result to the controller and apply runtime reconfiguration on success.

```json
{ "verdict": "blocker", "issues": [ { "severity": "blocker", "file": "src/controllers/settings_controller.cpp", "line": 15, "msg": "QML-invoked save performs a synchronous sequence of configuration writes on the GUI thread." }, { "severity": "blocker", "file": "src/config/config_service.cpp", "line": 303, "msg": "Every setting write calls blocking QSettings::sync()." } ], "reviewer": "frozone" }
```

### B8. Backend-controlled strings are rendered with `Text.AutoText`, not inert plain text

The specification requires model/backend text to remain inert. QML `Text` defaults to `Text.AutoText`, yet agent names/details, cron results, package fields, model fields, Git paths/branch, calendar titles, network interface names, and mount paths are assigned without `textFormat: Text.PlainText`. Rich-looking content can be interpreted instead of displayed literally; rich text supports embedded resources and violates the no-render/no-fetch trust boundary.

Set `textFormat: Text.PlainText` on every backend/persisted-data `Text` consumer (not only the Memory and Creative previews) and add a malicious-rich-text regression fixture.

```json
{ "verdict": "blocker", "issues": [ { "severity": "blocker", "file": "qml/views/AgentRosterView.qml", "line": 178, "msg": "Backend statusDetail is rendered with AutoText rather than PlainText." }, { "severity": "blocker", "file": "qml/views/PackagesView.qml", "line": 124, "msg": "Package-manager output is rendered with AutoText." }, { "severity": "blocker", "file": "qml/views/GitView.qml", "line": 215, "msg": "Repository paths are rendered with AutoText." }, { "severity": "blocker", "file": "qml/views/CalendarView.qml", "line": 285, "msg": "Persisted event titles are not explicitly rendered as plain text." } ], "reviewer": "frozone" }
```

### B9. Required security dependencies and the GUI are optional in the build

The technical specification declares QtKeychain and libgit2 required, yet CMake quietly produces a successful core build without them and compiles both security-sensitive features into fail-closed stubs. Qt Quick is likewise optional, allowing a green build that does not compile or validate the application UI at all. The review environment demonstrated exactly this misleading green state.

Keep a deliberate core-only developer option if useful, but make the production/default application target fail configuration when GUI, keychain, or libgit2 dependencies are missing.

```json
{ "verdict": "blocker", "issues": [ { "severity": "blocker", "file": "CMakeLists.txt", "line": 14, "msg": "QtKeychain and libgit2 are discovered QUIET even though production features require them." }, { "severity": "blocker", "file": "CMakeLists.txt", "line": 30, "msg": "Missing Qt Quick modules silently suppress the application target." }, { "severity": "blocker", "file": "src/CMakeLists.txt", "line": 109, "msg": "A successful default build may contain no usable GitService or SecretStore implementation." } ], "reviewer": "frozone" }
```

### B10. Mandatory security and integration regression gates are missing

Only seven unit executables are registered. There are no GitService, SecretStore, controller meta-object, secret-reflection, source secret-scan, Gateway fail-closed, non-loopback HTTP policy, Creative cancellation/streaming, or QML integration tests. The SPEC explicitly names several of these as required security gates, and the uncaught blockers above show why they cannot be deferred.

```json
{ "verdict": "blocker", "issues": [ { "severity": "blocker", "file": "tests/CMakeLists.txt", "line": 10, "msg": "Required Git, keychain, credential-containment, gateway fail-closed, and controller/QML contract tests are absent." }, { "severity": "blocker", "file": "tests/CMakeLists.txt", "line": 11, "msg": "AtomicFile lacks concurrency/durability coverage and Creative/Git cancellation paths have no tests." } ], "reviewer": "frozone" }
```

## 🟡 WARNING — should fix, not blocking individually

### W1. Initial application state is empty and connection status is not actively established

`AppContext` wires `refreshAllRequested` but does not request an initial refresh. Most views also do not refresh on construction. The status bar can remain “Disconnected” even while CLI-backed data would be available, and `lastSync` advances only after agent refresh rather than the latest successful backend refresh.

```json
{ "verdict": "warning", "issues": [ { "severity": "warning", "file": "src/app/app_context.cpp", "line": 37, "msg": "Refresh signals are wired but no initial refresh is triggered." }, { "severity": "warning", "file": "src/app/app_context.cpp", "line": 51, "msg": "Global lastSync is sourced only from AgentController." } ], "reviewer": "frozone" }
```

### W2. Settings persistence is non-transactional and runtime consumers stay stale

Settings are written one key at a time before each key is validated. A later failure leaves earlier keys persisted. Successful changes do not restart the vitals interval or update MemoryController's `CONSTANT` root list, so the UI and services can disagree with persisted configuration until restart.

```json
{ "verdict": "warning", "issues": [ { "severity": "warning", "file": "src/controllers/settings_controller.cpp", "line": 15, "msg": "Sequential write-then-validate behavior permits partially saved settings." }, { "severity": "warning", "file": "src/controllers/memory_controller.h", "line": 16, "msg": "rootIds is CONSTANT even though settings can change memory roots." }, { "severity": "warning", "file": "src/app/app_context.cpp", "line": 53, "msg": "Vitals interval is read only at startup and is not reconfigured after save." } ], "reviewer": "frozone" }
```

### W3. Path validation and file open have a TOCTOU window

`MemoryService::read()` canonicalizes and validates a pathname, then opens that pathname separately. A process able to mutate an allowlisted root can exchange a file/symlink between those operations. Use descriptor-relative opening with no-follow semantics and verify the opened descriptor with `fstat`. During listing, one invalid/escaping entry aborts the entire inventory rather than being skipped, enabling a trivial availability failure.

```json
{ "verdict": "warning", "issues": [ { "severity": "warning", "file": "src/services/memory_service.cpp", "line": 69, "msg": "PathGuard validation and QFile open are separate pathname operations vulnerable to replacement between check and use." }, { "severity": "warning", "file": "src/services/memory_service.cpp", "line": 39, "msg": "One rejected entry aborts the whole memory listing instead of being omitted safely." } ], "reviewer": "frozone" }
```

### W4. Git staging and remote authentication have functional gaps

`git_index_add_bypath()` cannot stage a deleted worktree path; deletions need explicit index removal. Stage/unstage operations are not serialized and do not participate in controller busy state, allowing concurrent index mutations. Pull/push require a stored credential before reaching libgit2 callbacks, preventing public remotes and SSH-agent-only use.

```json
{ "verdict": "warning", "issues": [ { "severity": "warning", "file": "src/services/git_service.cpp", "line": 258, "msg": "Deleted paths cannot be explicitly staged with add_bypath alone." }, { "severity": "warning", "file": "src/controllers/git_controller.cpp", "line": 36, "msg": "Stage and unstage operations are not serialized or reflected in busy state." }, { "severity": "warning", "file": "src/services/git_service.cpp", "line": 394, "msg": "Pull fails before libgit2 can use public/SSH-agent authentication when no keychain credential exists." } ], "reviewer": "frozone" }
```

### W5. The “read-only” package query is arbitrary executable configuration with a late output cap

`packages.queryCommand` can name any executable, so an edited INI turns an inventory feature into code execution under the user's account. The 8 MiB cap is checked only after the process exits, while QProcess may buffer much more stdout/stderr in memory. Restrict to an allowlisted adapter or validate the executable and argument grammar; drain and cap output incrementally.

```json
{ "verdict": "warning", "issues": [ { "severity": "warning", "file": "src/services/package_service.cpp", "line": 18, "msg": "A config-controlled executable is launched without an allowlist or adapter boundary." }, { "severity": "warning", "file": "src/services/package_service.cpp", "line": 29, "msg": "Output is capped only after completion and stderr is never drained/capped." } ], "reviewer": "frozone" }
```

### W6. Gateway URL joining and connection-state semantics are incomplete

`authorizedRequest()` rejects absolute URLs but cleans `basePath + relative`; `..` can escape a configured base URL path while retaining the same origin. Keychain backend errors are collapsed to `MissingToken`, and there is no distinct no-token connection state required by the UI design. Some authorization failures leave the service in `Connecting`.

```json
{ "verdict": "warning", "issues": [ { "severity": "warning", "file": "src/services/gateway_service.cpp", "line": 183, "msg": "Relative path normalization can escape a configured gateway base-path prefix." }, { "severity": "warning", "file": "src/services/gateway_service.cpp", "line": 24, "msg": "SecretStore failures are reported as missing token, masking backend/configuration errors." }, { "severity": "warning", "file": "src/services/gateway_service.cpp", "line": 162, "msg": "Connection state cannot represent the required no-token condition." } ], "reviewer": "frozone" }
```

### W7. Vitals implementation does not meet the specified hardware coverage

GPU sampling recognizes only an AMD sysfs busy counter; NVIDIA NVML and Intel paths are absent. Disk sampling reports only `/`, not configured mounts. Missing readings correctly remain unavailable, but the promised platform coverage is incomplete.

```json
{ "verdict": "warning", "issues": [ { "severity": "warning", "file": "src/services/vitals_service.cpp", "line": 79, "msg": "Only AMD sysfs utilization is implemented; NVIDIA and Intel discovery are absent." }, { "severity": "warning", "file": "src/services/vitals_service.cpp", "line": 196, "msg": "Disk telemetry is hard-coded to the root filesystem." } ], "reviewer": "frozone" }
```

### W8. Credential status and update UI does not match SettingsController

The view binds to nonexistent `gatewayTokenSet` and `gitCredentialSet` properties. It disables Git credential updates and claims the controller lacks an intent even though `setGitCredential()` exists. Add asynchronous boolean status properties that never reveal values, and wire the masked update field to the existing setter.

```json
{ "verdict": "warning", "issues": [ { "severity": "warning", "file": "qml/views/SettingsView.qml", "line": 189, "msg": "gatewayTokenSet is not declared by SettingsController." }, { "severity": "warning", "file": "qml/views/SettingsView.qml", "line": 275, "msg": "gitCredentialSet is not declared and the existing credential update intent is disabled in UI." }, { "severity": "warning", "file": "src/controllers/settings_controller.h", "line": 83, "msg": "setGitCredential exists but is not usable from the Settings view." } ], "reviewer": "frozone" }
```

### W9. Toast severity types disagree

Controllers emit integer levels (success is generally `1`), while `ToastHost` compares strings such as `"success"` and `"error"`. Successful actions are therefore styled/timed as generic information, and the model role type relies on coercion. Define a registered enum or one string contract.

```json
{ "verdict": "warning", "issues": [ { "severity": "warning", "file": "qml/components/ToastHost.qml", "line": 17, "msg": "ToastHost expects string severity values." }, { "severity": "warning", "file": "src/controllers/calendar_controller.cpp", "line": 9, "msg": "Controllers emit integer toast levels, so string severity comparisons never match." } ], "reviewer": "frozone" }
```

### W10. Required legacy calendar migration is absent

No implementation or test imports the documented legacy calendar store on first run. This risks silently abandoning existing user data during the rebuild.

```json
{ "verdict": "warning", "issues": [ { "severity": "warning", "file": "src/services/calendar_store.cpp", "line": 1, "msg": "CalendarStore has no one-time legacy import/migration path required by SPEC section 11." } ], "reviewer": "frozone" }
```

### W11. The full GUI has no compile/lint/ASan gate in this checkout

There is no CI configuration or sanitizer preset, and QML compilation/lint is skipped whenever GUI development packages are absent. The core warning policy is good, but it cannot detect the QML/controller contract failures found here. Add a CI image with all production dependencies, `qmllint`, a QML smoke load, and ASan/UBSan test jobs.

```json
{ "verdict": "warning", "issues": [ { "severity": "warning", "file": "CMakeLists.txt", "line": 26, "msg": "The GUI can be omitted without failing the default validation build." }, { "severity": "warning", "file": "CMakeLists.txt", "line": 40, "msg": "No sanitizer-enabled validation target or preset is defined by the build." } ], "reviewer": "frozone" }
```

### W12. Bootstrap requests font resources that do not exist

`main.cpp` loads variable-font aliases that are absent from `aegis.qrc`. Typography's static `FontLoader`s provide a fallback, but the startup calls fail silently and should use existing aliases or be removed.

```json
{ "verdict": "warning", "issues": [ { "severity": "warning", "file": "src/main.cpp", "line": 20, "msg": "Inter-Variable.ttf and JetBrainsMono-Variable.ttf are not embedded resources." }, { "severity": "warning", "file": "resources/aegis.qrc", "line": 49, "msg": "Only static font aliases are declared." } ], "reviewer": "frozone" }
```

### W13. Several implementation files are effectively minified

The public naming generally follows the requested convention, but multiple controller implementations compress whole classes and functions into one physical line with setter/getter macros. This defeats normal review, line-level diagnostics, coverage attribution, and maintainability for security-sensitive orchestration code.

```json
{ "verdict": "warning", "issues": [ { "severity": "warning", "file": "src/controllers/settings_controller.cpp", "line": 5, "msg": "Most of the controller is macro-generated or compressed into very long single lines, contrary to the project's Google-style requirement." }, { "severity": "warning", "file": "src/controllers/creative_controller.cpp", "line": 4, "msg": "Signal wiring and state transitions are compressed into one line, obscuring error-state review." } ], "reviewer": "frozone" }
```

## 🔵 SUGGESTION — nice to have

### S1. Update models incrementally instead of resetting them

All list models reset wholesale on refresh. Stable IDs are present, so an ID-based diff would preserve selection, reduce delegate churn, and improve animation quality without creating a competing data path.

```json
{ "verdict": "suggestion", "issues": [ { "severity": "suggestion", "file": "src/models/agent_list_model.cpp", "line": 37, "msg": "Consider ID-based insert/update/remove operations instead of model resets." } ], "reviewer": "frozone" }
```

### S2. Remove or fix the unused shared watcher helper

`async::watch()` is unused and returns a shared watcher whose connected lambda captures the same shared pointer. That ownership pattern is easy to turn into a self-cycle. Remove dead code or parent/delete the watcher with a non-owning capture.

```json
{ "verdict": "suggestion", "issues": [ { "severity": "suggestion", "file": "src/core/async.h", "line": 56, "msg": "Unused watcher helper has an avoidable shared-ownership capture pattern." } ], "reviewer": "frozone" }
```

### S3. Add operator-facing build and configuration documentation

The repository lacks a README describing production dependencies, credential-store behavior, config location, supported package adapters, or the difference between core-only and shippable builds. Documenting those boundaries will reduce insecure deployments.

```json
{ "verdict": "suggestion", "issues": [ { "severity": "suggestion", "file": "README.md", "line": 1, "msg": "No operator/developer README is present." } ], "reviewer": "frozone" }
```

## ✅ PASS — clean aspects

### P1. Layer direction is structurally clean

QML receives controller context properties and does not import or instantiate C++ services. `AppContext` owns services/controllers with RAII, and controllers generally delegate platform work to services.

```json
{ "verdict": "pass", "issues": [ { "severity": "pass", "file": "src/app/app_context.cpp", "line": 7, "msg": "Ownership and QML → controller → service composition follow the intended layer direction." } ], "reviewer": "frozone" }
```

### P2. Gateway credential confinement and log redaction are sound

Gateway token reads occur only in `GatewayService`; QML can write but cannot read token values. Secrets are registered for redaction, and generic bearer/authorization/assignment patterns are sanitized. No token is present in a DTO or model.

```json
{ "verdict": "pass", "issues": [ { "severity": "pass", "file": "src/services/gateway_service.cpp", "line": 24, "msg": "Gateway token retrieval is confined to GatewayService." }, { "severity": "pass", "file": "src/core/logging.cpp", "line": 62, "msg": "Known and pattern-matched credential material is redacted from Qt logs." } ], "reviewer": "frozone" }
```

### P3. PathGuard enforces the specified lexical/canonical policy

It rejects NUL and absolute paths, canonicalizes the root and candidate, checks containment with a separator boundary, rejects escaping symlink components, optionally requires `.md`, and enforces the configured file-size cap. Unit tests cover traversal, sibling-prefix escape, symlinks, type, and size. W3 remains the open-time race outside PathGuard itself.

```json
{ "verdict": "pass", "issues": [ { "severity": "pass", "file": "src/core/path_guard.cpp", "line": 27, "msg": "Canonical containment, symlink escape, markdown extension, and size checks are correctly implemented." }, { "severity": "pass", "file": "tests/unit/tst_pathguard.cpp", "line": 1, "msg": "Critical PathGuard rejection cases have direct regression coverage." } ], "reviewer": "frozone" }
```

### P4. Process execution avoids shell interpolation

OpenClaw and package operations use `QProcess::start(program, arguments)`. No `system`, `popen`, shell command construction, or shell invocation was found. OpenClaw CLI work runs on the global thread pool with timeout and streaming output caps.

```json
{ "verdict": "pass", "issues": [ { "severity": "pass", "file": "src/services/openclaw_cli.cpp", "line": 1, "msg": "CLI execution uses an executable plus argument array and bounded worker-thread processing." }, { "severity": "pass", "file": "src/services/package_service.cpp", "line": 18, "msg": "Package command arguments are passed without shell interpolation." } ], "reviewer": "frozone" }
```

### P5. Git operations use libgit2 and explicit path lists

There is no shell Git invocation or `git add -A`; stage/unstage/commit accept validated explicit repository-relative paths, and pull rejects non-fast-forward merge analysis. B6 and W4 must still be fixed before these guarantees are operationally safe.

```json
{ "verdict": "pass", "issues": [ { "severity": "pass", "file": "src/services/git_service.cpp", "line": 244, "msg": "Staging accepts explicit validated paths and never stages the whole worktree implicitly." }, { "severity": "pass", "file": "src/services/git_service.cpp", "line": 423, "msg": "Non-fast-forward pull analysis is rejected rather than merged." } ], "reviewer": "frozone" }
```

### P6. Calendar writes are atomic and corruption-safe

`AtomicFile` serializes writers and uses `QSaveFile` with direct-write fallback disabled; `commit()` provides the temporary-file flush/atomic replacement path. CalendarStore locks mutations, validates DTOs, caps input, and refuses to overwrite malformed JSON. Existing atomic/calendar tests pass.

```json
{ "verdict": "pass", "issues": [ { "severity": "pass", "file": "src/core/atomic_file.cpp", "line": 8, "msg": "QSaveFile provides fail-closed atomic replacement with serialized writers." }, { "severity": "pass", "file": "src/services/calendar_store.cpp", "line": 142, "msg": "Calendar persistence validates data and preserves corrupt originals on read failure." } ], "reviewer": "frozone" }
```

### P7. DTO parsing is strict and error presentation is generally safe

DTO factories validate required types, lengths/ranges, enums, timestamps, and unknown fields. Service failures generally reach controllers as `AegisError`, and controllers emit only `userMessage`/retryability rather than raw debug context or stderr.

```json
{ "verdict": "pass", "issues": [ { "severity": "pass", "file": "src/dto/json_helpers.cpp", "line": 1, "msg": "Shared DTO helpers enforce required types, bounds, and unknown-field rejection." }, { "severity": "pass", "file": "src/core/aegis_error.cpp", "line": 1, "msg": "User-facing messages are separated from debug context and tested for sensitive-data containment." } ], "reviewer": "frozone" }
```

### P8. C++ ownership and Qt exposure are safe overall

Owned application objects use `std::unique_ptr`; QObject child ownership and stack RAII are used consistently. QML-facing state is exposed with `Q_PROPERTY`/NOTIFY or constant model pointers, actions use `Q_INVOKABLE`, and public enums are registered with `Q_ENUM`/QML registration. No owning raw pointer was identified.

```json
{ "verdict": "pass", "issues": [ { "severity": "pass", "file": "src/app/app_context.h", "line": 1, "msg": "Application ownership is explicit and RAII-based." }, { "severity": "pass", "file": "src/app/qml_registration.cpp", "line": 1, "msg": "Controller context and enum/type exposure use Qt's supported QML mechanisms." } ], "reviewer": "frozone" }
```

### P9. Theme tokens and baseline accessibility are consistently applied

Hard-coded color literals are centralized in `Theme.qml`; views/components consume Theme and Typography tokens. Reusable buttons and primary interactive controls generally define accessible names and roles, and reduce-motion hooks are present. Memory and Creative explicitly use plain text for their large content surfaces.

```json
{ "verdict": "pass", "issues": [ { "severity": "pass", "file": "qml/theme/Theme.qml", "line": 22, "msg": "Visual colors are centralized in the Theme singleton." }, { "severity": "pass", "file": "qml/components/PrimaryButton.qml", "line": 1, "msg": "Reusable controls provide centralized styling and accessibility behavior." } ], "reviewer": "frozone" }
```

## Architecture assessment

The static dependency direction is good: QML does not bypass controllers, and services own network, process, Git, keychain, and filesystem primitives. `AppContext` is a clear composition root and its `unique_ptr` declaration order keeps dependencies alive while dependents are destroyed.

The runtime architecture is not yet coherent. QML/controller APIs are not contract-tested, causing multiple dead workflows. Settings persistence blocks the GUI thread, while startup refresh and settings reconfiguration are incomplete. The intended single source of truth exists per domain model, but cached configuration-derived state (`rootIds`, vitals interval, global sync timestamp, gateway connection state) can diverge from persisted or actual state.

Required architecture exit criteria:

1. Add QML smoke/meta-object tests for every view/controller pair.
2. Move configuration validation/persistence off the GUI thread and commit it transactionally.
3. Define controller-owned one-shot confirmation state for destructive/remote Git and calendar operations.
4. Trigger an initial refresh and define how live services consume changed settings.
5. Make genuine streaming/cancellation an owned service abstraction rather than a buffered future.

## Security assessment

The credential design, redacting logger, path policy, no-shell process policy, explicit Git staging intent, strict DTO parsing, safe error messages, and atomic calendar persistence are all strong. No plaintext credential leak to QML, DTOs, or logs was found in source.

Security assurance is nevertheless incomplete because production keychain/libgit2 code was optional and untested, mandatory containment tests are absent, rich-looking backend strings are not universally forced to plain text, the PathGuard-to-open boundary has a race, and Git pull can mutate a ref before its worktree safety check completes. Those are merge-gating issues for a replacement whose purpose is to close the former app's security failures.

Required security exit criteria:

1. Fix the Git fast-forward transaction and test dirty-worktree rollback.
2. Require and execute real QtKeychain/libgit2 implementations in the production CI build.
3. Add the SPEC-mandated credential reflection/source-scan/gateway fail-closed tests.
4. Force all untrusted text consumers to `Text.PlainText` and test a rich-text payload.
5. Open memory files without following a post-validation replacement and verify the opened descriptor.
6. Enforce hard cancellation/timeouts for Git and Creative network operations.

## Final verdict

🔴 **BLOCKER — must fix before merge.**

The core safety primitives are promising, but the current `main` branch is neither functionally integrated nor sufficiently regression-protected for a security-sensitive release. Re-review after all B1–B10 items are fixed and the full GUI/keychain/libgit2 build passes its new security and QML contract suites.

```json
{ "verdict": "blocker", "issues": [ { "severity": "blocker", "file": "REVIEW_FROZONE.md", "line": 1, "msg": "AEGIS Mission Control is not merge ready; ten blocker finding groups require remediation and full production-dependency verification." } ], "reviewer": "frozone" }
```
