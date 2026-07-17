# AEGIS — Technical Specification

> **AEGIS Mission Control** — a native Qt 6 / QML / C++ desktop cockpit for commanding a fleet of OpenClaw AI agents and monitoring the host system.
>
> Spec version: 1.0 · Author: Jeff 🛡️ (security architect) · Date: 2026-07-17 · Status: authoritative rebuild spec

---

## 0. Naming

The old app was "Andy's Overview (AO)". This is a full rebrand + rebuild. Three candidate names:

1. **AEGIS** — the shield of Athena/Zeus. Security-forward, short, a strong single word. Fits a security-hardened command surface that guards a fleet. **← chosen.**
2. **Nightwatch** — evokes the "Midnight Command" theme and a vigilant operator on watch. Good, but slightly generic and collides with existing products.
3. **Helm** — you steer the fleet. Clean and nautical, but too generic and weak as a searchable identifier.

**Chosen name: AEGIS** (styled all-caps in the wordmark, "Aegis" in prose). Product string: **AEGIS Mission Control**. Reverse-DNS app id: `dev.tux.aegis`. Binary: `aegis`. Config/data namespace: `Aegis` / `aegis`.

The repository directory remains `projects/ao-mission-control/` for continuity, but no shipped artifact, window title, about box, or config key uses "AO" or "Andy's Overview".

---

## 1. Project Overview & Goals

### 1.1 What AEGIS is

AEGIS is a single-window native desktop application. It is the operator console for a personal OpenClaw deployment. It surfaces nine capabilities:

1. **Agent Roster** — live status of every configured OpenClaw agent (no hard-coded list).
2. **System Dashboard** — real CPU / GPU utilization, memory, disk, network with animated gauges.
3. **Calendar** — event management with typed DTOs, atomic writes, validation.
4. **Cron Manager** — view / toggle / run OpenClaw cron jobs, fully async with timeouts.
5. **Memory Viewer** — browse and read memory `.md` files under sandboxed, allowlisted paths.
6. **Model Switcher** — switch the active OpenClaw model from a server-provided list.
7. **Packages View** — system package inventory.
8. **Git Panel** — status / stage / commit / pull / push via libgit2, explicit path staging, confirmation on destructive ops.
9. **Creative** — Ollama + OpenClaw gateway creative generation, async with timeouts and cancellation.

### 1.2 What AEGIS deliberately is NOT

Dropped entirely (these were the primary source of the old app's architectural failure and XSS surface):

- Chat system — `ChatView`, `ChatMarkdown`, streaming, chat export, and all three competing WebSocket chat backends (Tauri IPC chat, `GatewayClient`, and the unauthenticated `ws://localhost:3851` sidecar).
- Mission Board.
- Idea Board.

There is **no embedded webview anywhere in AEGIS**. The entire UI is native Qt Quick (QML) rendered on the GPU. This structurally eliminates the XSS / CSP / `dangerouslySetInnerHTML` class of vulnerabilities that dominated the audit.

### 1.3 Goals

- **G1 — Security by construction.** Every audit finding (§6 mapping) is closed by architecture, not by patching. Secrets never reach the presentation layer. No shell interpolation. No raw HTML injection surface.
- **G2 — One source of truth.** One process, one service layer, no out-of-tree sidecar, no dead transport code carrying credentials.
- **G3 — Truthful telemetry.** Every displayed metric reflects a real measurement or is explicitly labeled. No "always Connected", no VRAM-ratio masquerading as GPU utilization.
- **G4 — Non-blocking UI.** All I/O (process, network, filesystem, git) runs off the GUI thread. The UI thread never blocks.
- **G5 — Buildable from this spec.** A developer with Qt 6.6+, a C++20 compiler, CMake, and libgit2 can build AEGIS reading only this document.

### 1.4 Non-goals

- Multi-user / multi-tenant operation. Single local operator.
- Cross-machine remote control beyond talking to the configured local/tailnet gateway.
- Mobile / touch layouts. Desktop only, minimum window 1024×720.

---

## 2. Architecture

### 2.1 Layer model

```
┌──────────────────────────────────────────────────────────────┐
│  Presentation (QML / Qt Quick)                                 │
│  Views, components, animations. NO secrets. NO business logic. │
│  Talks only to Controllers via exposed context properties.     │
├──────────────────────────────────────────────────────────────┤
│  Controllers (C++ QObject, exposed to QML)                     │
│  Thin adapters: expose Q_PROPERTY state + Q_INVOKABLE intents, │
│  emit signals on change. Own no I/O. Delegate to Services.     │
│  Marshal typed DTOs -> QML-friendly models (QAbstractListModel)│
├──────────────────────────────────────────────────────────────┤
│  Services (C++, no Qt Quick dependency, unit-testable)         │
│  GatewayService, OpenClawCli, VitalsService, CalendarStore,    │
│  CronService, MemoryService, ModelService, PackageService,     │
│  GitService, CreativeService, ConfigService, SecretStore.      │
│  All blocking work runs on QtConcurrent / QThreadPool.         │
├──────────────────────────────────────────────────────────────┤
│  Platform / Vendor                                             │
│  QProcess, QNetworkAccessManager, libgit2, QtKeychain,         │
│  /proc + /sys + sysinfo, QFileSystemWatcher.                   │
└──────────────────────────────────────────────────────────────┘
```

**Hard rule (enforced by review + build layering):** QML never imports a Service directly and never sees a token, header, or raw CLI stderr. QML sees only Controllers. Controllers never construct QNetworkRequest auth headers; they call Services. Secrets live only in `SecretStore` and `GatewayService`.

### 2.2 Threading model

- **GUI thread:** QML scene graph, Controllers, model updates. Never performs blocking I/O.
- **Service work:** every network / process / git / file operation is dispatched to a `QThreadPool` (default global pool, sized `max(4, QThread::idealThreadCount())`) via `QtConcurrent::run` returning `QFuture<Result<T>>`, or via a dedicated `QProcess`/`QNetworkReply` running asynchronously with signal completion.
- **Result delivery:** results are marshaled back to the GUI thread using `QFutureWatcher` (lives on GUI thread) or queued signal/slot connections (`Qt::QueuedConnection`). Controllers only mutate their `Q_PROPERTY` state on the GUI thread.
- **Cancellation:** long operations (creative generation, git network ops, gateway calls) expose a cancel token. Implemented via `QFutureWatcher::cancel()` where cooperative, and via aborting the underlying `QNetworkReply`/`QProcess` for hard cancel. Every Creative and Git-network call is cancellable.
- **Timeouts:** every network call has a connect timeout and a total-deadline `QTimer`; every `QProcess` call has a kill-after deadline. Defaults in §8.5.

### 2.3 Process & tooling boundaries

AEGIS shells out to two external programs only, always via `QProcess` with an **argument array** (never a command string, never `/bin/sh -c`):

- `openclaw` CLI — agent roster, cron, model, creative-gateway, config reads.
- The system package manager query binary — read-only inventory (`rpm -qa` / `dpkg-query` / configured command).

All git operations use **libgit2** (via `git2` C API or the `libgit2` C++ wrapper) — never the `git` binary, never `git add -A`.

### 2.4 Directory structure

```
projects/ao-mission-control/
├── CMakeLists.txt                  # top-level, C++20, Qt6 find_package
├── SPEC.md
├── UI_SPEC.md
├── README.md
├── cmake/
│   ├── FindLibgit2.cmake           # fallback finder if no config package
│   └── AegisWarnings.cmake         # -Wall -Wextra -Werror profile
├── resources/
│   ├── aegis.qrc                   # QML + fonts + icons bundled
│   ├── fonts/                      # JetBrains Mono, Inter (OFL/SIL, vendored)
│   ├── icons/                      # SVG sidebar + status icons
│   └── qtquickcontrols2.conf       # force Basic style, dark palette
├── qml/
│   ├── Main.qml                    # ApplicationWindow, sidebar + stack
│   ├── theme/
│   │   ├── Theme.qml               # singleton design tokens (colors/spacing)
│   │   ├── Typography.qml          # singleton font roles
│   │   └── Motion.qml              # singleton durations/easings
│   ├── components/                 # reusable widgets (see UI_SPEC §Component Library)
│   │   ├── GlassCard.qml
│   │   ├── VitalGauge.qml
│   │   ├── StatusRing.qml
│   │   ├── SidebarItem.qml
│   │   ├── StatusBar.qml
│   │   ├── PrimaryButton.qml
│   │   ├── ConfirmDialog.qml
│   │   ├── ToastHost.qml
│   │   ├── EmptyState.qml
│   │   ├── LoadingState.qml
│   │   ├── ErrorState.qml
│   │   └── RadarGrid.qml           # background HUD grid
│   └── views/
│       ├── DashboardView.qml
│       ├── AgentRosterView.qml
│       ├── CalendarView.qml
│       ├── CronView.qml
│       ├── MemoryView.qml
│       ├── ModelView.qml
│       ├── PackagesView.qml
│       ├── GitView.qml
│       ├── CreativeView.qml
│       └── SettingsView.qml
├── src/
│   ├── main.cpp                    # bootstrap, DI wiring, QML registration
│   ├── app/
│   │   ├── AppContext.{h,cpp}      # owns services + controllers, lifetime root
│   │   └── QmlRegistration.{h,cpp} # qmlRegisterType/singletons, enum exposure
│   ├── core/
│   │   ├── Result.h                # Result<T> = expected<T, AegisError>
│   │   ├── AegisError.{h,cpp}      # structured error enum + codes
│   │   ├── Logging.{h,cpp}         # categorized logging, secret redaction
│   │   ├── Async.h                 # QtConcurrent helpers, watcher glue
│   │   ├── PathGuard.{h,cpp}       # canonicalization + containment + symlink reject
│   │   ├── AtomicFile.{h,cpp}      # temp+fsync+rename writer
│   │   └── HttpClient.{h,cpp}      # QNAM wrapper: timeouts, size caps, retries
│   ├── config/
│   │   ├── ConfigService.{h,cpp}   # QSettings-backed typed config
│   │   └── SecretStore.{h,cpp}     # QtKeychain wrapper, fail-closed
│   ├── dto/                        # plain structs + (de)serialization
│   │   ├── AgentDto.{h,cpp}
│   │   ├── VitalsDto.{h,cpp}
│   │   ├── CalendarEventDto.{h,cpp}
│   │   ├── CronJobDto.{h,cpp}
│   │   ├── MemoryFileDto.{h,cpp}
│   │   ├── ModelDto.{h,cpp}
│   │   ├── PackageDto.{h,cpp}
│   │   ├── GitStatusDto.{h,cpp}
│   │   └── CreativeDto.{h,cpp}
│   ├── services/
│   │   ├── GatewayService.{h,cpp}
│   │   ├── OpenClawCli.{h,cpp}
│   │   ├── VitalsService.{h,cpp}
│   │   ├── CalendarStore.{h,cpp}
│   │   ├── CronService.{h,cpp}
│   │   ├── MemoryService.{h,cpp}
│   │   ├── ModelService.{h,cpp}
│   │   ├── PackageService.{h,cpp}
│   │   ├── GitService.{h,cpp}
│   │   └── CreativeService.{h,cpp}
│   ├── controllers/
│   │   ├── AppController.{h,cpp}         # connection state, active view, sync clock
│   │   ├── AgentController.{h,cpp}
│   │   ├── VitalsController.{h,cpp}
│   │   ├── CalendarController.{h,cpp}
│   │   ├── CronController.{h,cpp}
│   │   ├── MemoryController.{h,cpp}
│   │   ├── ModelController.{h,cpp}
│   │   ├── PackageController.{h,cpp}
│   │   ├── GitController.{h,cpp}
│   │   ├── CreativeController.{h,cpp}
│   │   └── SettingsController.{h,cpp}
│   └── models/                     # QAbstractListModel subclasses for QML
│       ├── AgentListModel.{h,cpp}
│       ├── CalendarEventModel.{h,cpp}
│       ├── CronJobModel.{h,cpp}
│       ├── MemoryFileModel.{h,cpp}
│       ├── ModelListModel.{h,cpp}
│       ├── PackageListModel.{h,cpp}
│       └── GitFileModel.{h,cpp}
└── tests/
    ├── CMakeLists.txt
    ├── unit/                       # QtTest per-service
    │   ├── tst_pathguard.cpp
    │   ├── tst_atomicfile.cpp
    │   ├── tst_aegiserror.cpp
    │   ├── tst_calendarstore.cpp
    │   ├── tst_memoryservice.cpp
    │   ├── tst_gitservice.cpp
    │   ├── tst_httpclient.cpp
    │   ├── tst_secretstore.cpp
    │   └── tst_openclawcli_parse.cpp
    └── fixtures/                   # sample CLI output, sample repos, sample md
```

---

## 3. Data Model (DTOs)

All DTOs are plain C++ structs (value types) in `namespace aegis::dto`. They carry **no Qt Quick dependency** and are unit-testable. Each has a `static Result<T> fromJson(const QJsonObject&)` (or `fromCliLine`) and, where persisted, `QJsonObject toJson() const`. Deserialization **validates every field**; unknown/oversized/malformed input yields `AegisError`, never a silent default.

Types use fixed-width where it matters. Timestamps are `QDateTime` in UTC internally, formatted for display only in QML.

### 3.1 AgentDto

```cpp
enum class AgentStatus { Active, Idle, Error, Unknown };

struct AgentDto {
    QString   id;            // stable agent key from openclaw config (required, non-empty)
    QString   displayName;   // human label; falls back to id
    QString   model;         // model id currently bound, if reported
    AgentStatus status = AgentStatus::Unknown;
    QString   statusDetail;  // short reason string, safe for display
    QDateTime lastSeen;      // UTC; invalid if never seen
    int       activeSessions = 0; // >= 0
    QString   avatarSeed;    // deterministic seed for generated avatar color/glyph
};
```

Roster is derived **entirely** from `openclaw` output (see §5.2). No legacy hard-coded roster. Each agent's status is derived independently — never a single global `pgrep` result applied to all agents.

### 3.2 VitalsDto

```cpp
struct CpuVitals {
    double   utilizationPct = 0.0;   // 0..100, real busy% from /proc/stat delta
    int      coreCount = 0;
    QVector<double> perCorePct;      // optional, 0..100 each
    double   loadAvg1 = 0.0;
    double   tempC = qQNaN();        // NaN if unavailable (labeled "n/a" in UI)
};

struct GpuVitals {
    bool     available = false;
    QString  vendor;                 // "nvidia" | "amd" | "intel" | ""
    double   utilizationPct = qQNaN(); // REAL util; NaN => UI shows "n/a", never fabricated
    double   memUsedBytes = 0.0;
    double   memTotalBytes = 0.0;
    double   tempC = qQNaN();
};

struct MemVitals {
    quint64 totalBytes = 0;
    quint64 usedBytes = 0;
    quint64 availableBytes = 0;
    quint64 swapTotalBytes = 0;
    quint64 swapUsedBytes = 0;
};

struct DiskVitals {
    QString  mount;           // e.g. "/"
    quint64  totalBytes = 0;
    quint64  usedBytes = 0;
};

struct NetVitals {
    QString  iface;
    quint64  rxBytesPerSec = 0;   // computed from sampled delta
    quint64  txBytesPerSec = 0;
};

struct VitalsDto {
    CpuVitals cpu;
    GpuVitals gpu;
    MemVitals mem;
    QVector<DiskVitals> disks;
    QVector<NetVitals>  nets;
    QDateTime sampledAt;      // UTC
};
```

**Truthfulness rule:** `GpuVitals.utilizationPct` is real utilization (NVML for nvidia, `/sys/class/drm/.../gpu_busy_percent` for amdgpu, `intel_gpu_top`-equivalent or NaN for intel). If no real utilization source exists, it is `NaN` and the UI renders "n/a" — never a VRAM ratio and never a fabricated 15% fallback.

### 3.3 CalendarEventDto

```cpp
struct CalendarEventDto {
    QString   id;            // UUID (validated format); generated on create
    QString   title;         // 1..200 chars after trim (validated)
    QString   description;   // 0..4000 chars
    QDateTime start;         // UTC, required, valid
    QDateTime end;           // UTC, required, >= start
    bool      allDay = false;
    QString   location;      // 0..200 chars
    QString   color;         // one of a fixed enum palette token, validated
    QDateTime createdAt;     // UTC
    QDateTime updatedAt;     // UTC
};
```

Validation is enforced in `fromJson` and again in `CalendarStore` before write. Array size cap on the whole store (default 10,000 events) and per-field length caps.

### 3.4 CronJobDto

```cpp
enum class CronState { Enabled, Disabled, Unknown };

struct CronJobDto {
    QString   id;            // opaque job id from openclaw (required)
    QString   name;          // display label
    QString   schedule;      // cron expression string as reported (display only)
    CronState state = CronState::Unknown;
    QString   command;       // short description; NOT executed by AEGIS
    QDateTime lastRun;       // UTC, invalid if never
    QDateTime nextRun;       // UTC, invalid if unknown
    QString   lastResult;    // safe short status string
};
```

AEGIS never composes or executes the cron command itself; it only calls `openclaw cron <toggle|run>` with the validated job id.

### 3.5 MemoryFileDto

```cpp
struct MemoryFileDto {
    QString   name;          // basename, must end ".md"
    QString   relativePath;  // path relative to an allowlisted root
    QString   rootId;        // which allowlisted root this belongs to
    quint64   sizeBytes = 0; // <= size cap
    QDateTime modifiedAt;    // UTC
};
```

Content is delivered separately (`MemoryService::readFile`) and is **plain text**, rendered as read-only monospace/markdown-lite text in QML — never parsed to HTML, never executed.

### 3.6 ModelDto

```cpp
struct ModelDto {
    QString id;              // canonical model id from server config (required)
    QString provider;        // e.g. "anthropic"
    QString label;           // display name
    bool    isActive = false;// whether this is the current active model
};
```

The model list is fetched live from the gateway/CLI. There is **no** hard-coded allowlist in C++ or QML.

### 3.7 PackageDto

```cpp
struct PackageDto {
    QString name;            // required
    QString version;         // required
    QString source;          // "rpm" | "dpkg" | "flatpak" | configured
    QString summary;         // optional, truncated to 200 chars
};
```

### 3.8 GitStatusDto

```cpp
enum class GitFileState { New, Modified, Deleted, Renamed, Untracked, Conflicted, Ignored };

struct GitFileEntry {
    QString      path;               // repo-relative
    GitFileState indexState = GitFileState::Untracked;   // staged side
    GitFileState worktreeState = GitFileState::Untracked;// unstaged side
    bool         staged = false;
};

struct GitStatusDto {
    QString  repoPath;               // canonical, validated worktree
    QString  branch;                 // current branch or detached-HEAD label
    QString  upstream;               // tracking ref, may be empty
    int      ahead = 0;
    int      behind = 0;
    bool     clean = true;
    bool     detached = false;
    QVector<GitFileEntry> entries;
    QString  lastCommitSummary;      // subject of HEAD
    QString  lastCommitHash;         // short sha
};
```

### 3.9 CreativeDto

```cpp
enum class CreativeBackend { Ollama, GatewayCreative };

struct CreativeRequest {
    CreativeBackend backend = CreativeBackend::Ollama;
    QString model;                   // selected from live model list
    QString prompt;                  // 1..8000 chars, validated
    double  temperature = 0.7;       // clamped 0..2
    int     maxTokens = 1024;        // clamped 1..8192
};

struct CreativeResult {
    QString requestId;               // client-generated correlation id
    QString text;                    // accumulated output
    bool    done = false;
    QString finishReason;            // "stop" | "length" | "cancelled" | "error"
    quint64 outputBytes = 0;         // bounded by response-size cap
};
```

Streaming (if the backend streams) is delivered via incremental controller signals keyed by `requestId`; the final commit is a single snapshot (avoids the stale-final-token bug from the old app).

---

## 4. Error Model

### 4.1 Structured error

```cpp
namespace aegis {

enum class ErrorCode {
    Ok = 0,
    // config / secrets
    MissingToken,          // fail-closed: no gateway token in secret store
    ConfigInvalid,
    // network
    NetworkTimeout,
    NetworkUnreachable,
    NetworkTls,
    HttpStatus,            // non-2xx; detail carries status
    ResponseTooLarge,
    ResponseMalformed,
    // process / cli
    ProcessSpawnFailed,
    ProcessTimeout,
    ProcessNonZeroExit,
    CliOutputMalformed,
    // filesystem
    PathOutsideSandbox,
    PathSymlinkRejected,
    PathNotFound,
    FileTooLarge,
    WriteFailed,
    // git
    GitOpenFailed,
    GitOperationFailed,
    GitAuthFailed,
    GitConflict,
    // validation
    ValidationFailed,
    // generic / control
    Cancelled,
    Unknown
};

struct AegisError {
    ErrorCode   code = ErrorCode::Unknown;
    QString     userMessage;   // safe, human-readable, NO paths/tokens/stderr
    QString     debugContext;  // internal only, logged, NEVER shown in QML
    bool        retryable = false;
};

} // namespace aegis
```

### 4.2 Result type

```cpp
template <class T>
using Result = tl::expected<T, aegis::AegisError>;   // or std::expected under C++23
```

Services return `Result<T>`. Controllers translate `AegisError` into: (a) a `userMessage` surfaced via a toast/error-state signal, and (b) a boolean `retryable` that drives a "Retry" affordance. `debugContext` is written only to the categorized log with secret redaction (§4.3). Raw CLI stderr, tokens, and absolute paths are **never** placed in `userMessage`.

### 4.3 Logging & redaction

`Logging` provides `QLoggingCategory` per subsystem (`aegis.gateway`, `aegis.git`, `aegis.vitals`, …). A redaction filter strips anything matching known secret patterns (bearer tokens, `Authorization:` headers, keychain values) before emission. Default log level is `Info`; `Debug` is opt-in via `AEGIS_LOG` env or Settings.

---

## 5. Service Layer

Each service is a `QObject` living on the GUI thread that **dispatches** work to the thread pool and emits queued signals. Public API returns `QFuture<Result<T>>` for one-shot calls or emits signals for streaming/watched state. No service touches QML.

### 5.1 GatewayService

Owns the **only** code path that reads and uses the gateway token.

```cpp
class GatewayService : public QObject {
    Q_OBJECT
public:
    explicit GatewayService(SecretStore*, ConfigService*, HttpClient*, QObject* parent=nullptr);

    // fail-closed: returns MissingToken error if no token in secret store
    QFuture<Result<QJsonObject>> get(const QString& path,
                                     const QUrlQuery& query = {},
                                     std::chrono::milliseconds deadline = kDefaultDeadline);
    QFuture<Result<QJsonObject>> postJson(const QString& path,
                                          const QJsonObject& body,
                                          std::chrono::milliseconds deadline = kDefaultDeadline);

    // streaming creative (SSE/newline-delimited); emits chunks; cancellable
    QString beginStream(const QString& path, const QJsonObject& body); // returns requestId
    void    cancelStream(const QString& requestId);

    ConnectionState connectionState() const;

signals:
    void connectionStateChanged(ConnectionState);
    void streamChunk(QString requestId, QByteArray chunk);
    void streamFinished(QString requestId, aegis::AegisError errorOrOk);

private:
    // token is fetched from SecretStore per-request, added as Authorization header
    // INSIDE this class only. Never returned, never logged, never exposed to QML.
};
```

- **Base URL** comes from `ConfigService` (no hard-coded tailnet address). Scheme must be `https` for non-loopback hosts; `http` permitted only for `localhost`/`127.0.0.1`/`::1` (loopback exception). TLS enforced otherwise.
- **Auth:** `Authorization: Bearer <token>` added inside `GatewayService`. If `SecretStore` returns no token → immediate `MissingToken` error, no request sent. Fail-closed.
- **There is no `get_gateway_token`-equivalent.** No method returns the token to any caller.
- `HttpClient` (below) enforces connect + total timeout and response-size cap.

### 5.2 OpenClawCli

Async wrapper over the `openclaw` binary using `QProcess` with an **arg array**.

```cpp
class OpenClawCli : public QObject {
    Q_OBJECT
public:
    struct CliResult { int exitCode; QByteArray stdoutData; QByteArray stderrData; };

    // args passed as QStringList -> QProcess::start(program, args). No shell.
    QFuture<Result<CliResult>> run(const QStringList& args,
                                   std::chrono::milliseconds timeout = kCliTimeout,
                                   quint64 maxOutputBytes = kCliOutputCap);

    // Typed helpers (parse + validate output into DTOs):
    QFuture<Result<QVector<dto::AgentDto>>>   listAgents();
    QFuture<Result<QVector<dto::CronJobDto>>> listCron();
    QFuture<Result<QVector<dto::ModelDto>>>   listModels();
};
```

- Program path resolved from config (`openclawBinary`, default: resolve `openclaw` on `PATH`), validated to be an executable regular file.
- Output is captured with a byte cap; exceeding the cap → `CliOutputMalformed`/`ResponseTooLarge` and the process is killed.
- Timeout kills the process (`QProcess::kill`) and yields `ProcessTimeout`.
- Parsers are pure functions (unit-tested against fixtures in `tests/fixtures/`).
- Agent status is derived per agent from the CLI's structured output; **no** global `pgrep`.

### 5.3 VitalsService

Samples real system metrics on a timer (default 1 s) off-thread.

```cpp
class VitalsService : public QObject {
    Q_OBJECT
public:
    void start(std::chrono::milliseconds interval);
    void stop();
signals:
    void sampled(dto::VitalsDto);
    void failed(aegis::AegisError);   // reported, never crashes
};
```

- **CPU:** delta of `/proc/stat` aggregate + per-core between samples → real busy%. No panic on read failure — reported via `failed`.
- **GPU:** vendor-specific real utilization (NVML / amdgpu `gpu_busy_percent` / intel). Missing → `NaN`.
- **Mem:** `/proc/meminfo`.
- **Disk:** `statvfs` on configured mounts.
- **Net:** delta of `/proc/net/dev` per iface → bytes/sec.
- Any poisoned-mutex / unwrap-equivalent is replaced with error propagation. No `.unwrap()`/`.expect()` analog on hot paths.

### 5.4 CalendarStore

Typed, atomic, validated persistence of calendar events.

```cpp
class CalendarStore : public QObject {
    Q_OBJECT
public:
    QFuture<Result<QVector<dto::CalendarEventDto>>> load();
    QFuture<Result<dto::CalendarEventDto>>          create(dto::CalendarEventDto draft);
    QFuture<Result<dto::CalendarEventDto>>          update(dto::CalendarEventDto ev);
    QFuture<Result<void>>                           remove(QString id);
signals:
    void changed(); // fires after any successful mutation
};
```

- Backing file: `<dataRoot>/calendar/events.json` (dataRoot from config, defaults to app data dir; see §7 & §11).
- **Load:** parse failure is reported (`ResponseMalformed`/`ValidationFailed`), **never** silently coerced to empty. On parse failure the store refuses to overwrite; it surfaces the error so the user does not lose data.
- **Write:** `AtomicFile` — write to `events.json.tmp`, `fsync`, `rename` over the target. A single in-process `QMutex` serializes mutations.
- All fields validated on create/update; invalid → `ValidationFailed` with safe message.
- No optimistic best-effort writes: controllers await the result and surface failures.

### 5.5 CronService

```cpp
class CronService : public QObject {
    Q_OBJECT
public:
    QFuture<Result<QVector<dto::CronJobDto>>> list();
    QFuture<Result<dto::CronJobDto>>          toggle(QString jobId, bool enable);
    QFuture<Result<QString>>                  runNow(QString jobId); // returns run correlation/status
};
```

- Delegates to `OpenClawCli` with validated `jobId` (charset + length validation before spawn).
- All calls async with timeouts. `runNow` never blocks the UI.

### 5.6 MemoryService

Sandboxed browse + read of memory `.md` files.

```cpp
class MemoryService : public QObject {
    Q_OBJECT
public:
    QFuture<Result<QVector<dto::MemoryFileDto>>> list(QString rootId);      // allowlisted roots only
    QFuture<Result<QString>>                     read(QString rootId, QString relativePath);
    QVector<QString> rootIds() const;  // configured allowlist keys
};
```

- Allowlisted roots configured (`memoryRoots`), e.g. `{ "workspace-memory": "<workspace>/memory", "root-memory": "<workspace>" }`. Each root is canonicalized once at startup.
- Every `read`/`list` runs the path through `PathGuard` (§6.4): resolve `relativePath` against the root, `canonicalize`, assert the canonical result is still contained within the canonical root, reject symlinks whose target escapes, require `.md` suffix for reads, enforce a size cap (default 5 MiB).
- Content returned as UTF-8 text. No HTML conversion anywhere.

### 5.7 ModelService

```cpp
class ModelService : public QObject {
    Q_OBJECT
public:
    QFuture<Result<QVector<dto::ModelDto>>> list();          // live from CLI/gateway
    QFuture<Result<dto::ModelDto>>          setActive(QString modelId); // validated against live list
signals:
    void activeModelChanged(QString modelId);
};
```

- List fetched live (no hard-coded model IDs). `setActive` validates `modelId` is a member of the freshly fetched list before issuing the switch — server-side policy is source of truth.

### 5.8 PackageService

```cpp
class PackageService : public QObject {
    Q_OBJECT
public:
    QFuture<Result<QVector<dto::PackageDto>>> list(); // read-only inventory
};
```

- Query command configurable (`packageQueryCommand` as arg array). Read-only. Output capped and parsed into DTOs. Never installs/removes.

### 5.9 GitService

libgit2-backed, no `git` binary, explicit-path staging, confirmation on destructive ops.

```cpp
class GitService : public QObject {
    Q_OBJECT
public:
    explicit GitService(ConfigService*, SecretStore*, QObject* parent=nullptr);

    QFuture<Result<dto::GitStatusDto>> status();                         // open + validate worktree
    QFuture<Result<void>>              stage(QStringList explicitPaths); // NEVER add-all
    QFuture<Result<void>>              unstage(QStringList explicitPaths);
    QFuture<Result<QString>>           commit(QString message, QStringList stagedPaths); // returns short sha
    QFuture<Result<void>>              pull();   // fetch + ff-only (or reports GitConflict)
    QFuture<Result<void>>              push();
signals:
    void repoChanged();
};
```

- **Repo path** from config (`gitRepoPath`), canonicalized, validated as a real git worktree (`git_repository_open_ext` success). Missing/invalid → `GitOpenFailed` with actionable safe message; never a hard-coded nonexistent path.
- **Staging** takes an explicit `QStringList` of repo-relative paths and stages exactly those (`git_index_add_bypath` per path). There is **no `git add -A`** and no "stage all" that bypasses review.
- **Commit** requires an explicit staged path set and a non-empty message; the controller requires a `ConfirmDialog` acknowledgment before commit/push/pull (destructive/remote ops).
- **Credentials for push/pull:** obtained via a credential callback that pulls from `SecretStore` (SSH key path / token), never logged, never surfaced. Default preferred transport is SSH agent / configured key.
- `pull` is fetch + fast-forward-only by default; a non-ff situation returns `GitConflict` (no silent merge/rebase, no data loss).

### 5.10 CreativeService

```cpp
class CreativeService : public QObject {
    Q_OBJECT
public:
    QString  generate(dto::CreativeRequest req);   // returns requestId; async + streaming
    void     cancel(QString requestId);
signals:
    void chunk(QString requestId, QString textDelta);
    void finished(QString requestId, dto::CreativeResult result);
    void failed(QString requestId, aegis::AegisError error);
};
```

- **Ollama backend:** `HttpClient` to configured Ollama base URL (`ollamaBaseUrl`, default `http://localhost:11434`), reusable QNAM, connect/total timeouts, response-size cap, streaming line parse.
- **Gateway backend:** via `GatewayService::beginStream` (token stays in GatewayService).
- Every request is cancellable; cancel aborts the underlying reply and emits `finished` with `finishReason="cancelled"`.
- Prompt validated (length/charset) before dispatch.

### 5.11 ConfigService & SecretStore

Covered in §7.

---

## 6. Security Model

### 6.1 Threat model

Assets: gateway credential, git push credentials, the operator's filesystem (memory + workspace), the host system (process spawning), and the integrity of persisted data (calendar).

Adversaries / vectors considered:

- **T1 — Malicious/compromised model or gateway response.** Old app turned this into code via Markdown→HTML→`dangerouslySetInnerHTML` inside a privileged webview. AEGIS has **no webview and no HTML rendering path**; model/gateway text is displayed as inert text in native QML `Text` items. This class is structurally eliminated.
- **T2 — Credential theft / exposure.** Old app compiled a real token into the binary and returned it to the webview. AEGIS stores the token only in the OS secret store, uses it only inside `GatewayService`, never returns/logs it, and fails closed if absent.
- **T3 — Command injection.** All process calls use `QProcess` arg arrays; no `/bin/sh -c`, no string interpolation. Git uses libgit2, not the shell.
- **T4 — Path traversal / arbitrary file read.** `PathGuard` canonicalizes, enforces containment within allowlisted roots, rejects escaping symlinks, requires `.md` for memory reads, and caps size.
- **T5 — Data corruption / loss.** Atomic writes + typed schemas + refuse-to-overwrite-on-parse-error prevent the "corrupt file read as empty then overwritten" failure and the hydration-race destructive write.
- **T6 — Denial of service via oversized responses / hung I/O.** Every network/process call has timeouts and byte caps; UI never blocks.
- **T7 — Supply chain.** Minimal, pinned dependencies; audited at build (§10.4).

### 6.2 Old-vulnerability → mitigation mapping

| Audit finding | Severity | AEGIS mitigation |
|---|---|---|
| Real gateway token compiled in as fallback (`sessions.rs:7`) | BLOCKER | Token only in OS secret store via `SecretStore`; no compiled/default credential; fail-closed `MissingToken`. |
| Token sent over plain HTTP to hard-coded tailnet (`sessions.rs:68`) | BLOCKER | Base URL from config; TLS required for non-loopback; loopback-only HTTP exception; per-request timeouts + size caps. |
| `get_gateway_token` returns credential to webview (`chat.rs:178`) | BLOCKER | No token-returning API exists. Token confined to `GatewayService`. No webview at all. |
| Markdown→HTML injected via `dangerouslySetInnerHTML` (`ChatMarkdown.tsx:9`) | BLOCKER | Chat feature dropped. All text rendered as inert native QML `Text`. No HTML sink anywhere. |
| CSP allows `unsafe-inline`/`unsafe-eval` (`tauri.conf.json:23`) | BLOCKER | No webview, no CSP surface. N/A by construction. |
| Mission Board hydration race overwrites server data (`MissionBoard.tsx:59`) | BLOCKER | Mission Board dropped. General fix: stores refuse writes before successful load; parse-error never coerced to empty; atomic writes. |
| Git targets nonexistent hard-coded dir (`git.rs:19`) | BLOCKER | `gitRepoPath` from config, canonicalized, validated worktree; safe error if invalid. |
| Sessions client no timeout, hard-coded http (`sessions.rs:3`) | WARN | `HttpClient` mandatory timeouts + caps; config URL; TLS policy. |
| Unauthenticated `ws://localhost:3851` chat (`ChatView.tsx:17`) | WARN | Sidecar and chat dropped entirely. No out-of-tree service. |
| All sensitive commands in one webview (`lib.rs:20`) | WARN | Layered arch; QML has no privileged primitive; Controllers validate intents; secrets in C++ only. |
| Unused shell-open capability (`default.json:8`) | WARN | No Tauri/shell plugin. `QProcess` limited to `openclaw` + package query, both config-validated executables. |
| `get_memory_file` no canonicalize/symlink/size/ext checks (`files.rs:36`) | WARN | `PathGuard`: canonicalize + containment + symlink reject + `.md` required + size cap. |
| `git add -A` stages whole worktree (`git.rs:60`) | WARN | Explicit-path staging only; commit requires explicit staged set + confirm dialog. |
| Multiple dead chat/session impls with secret logic (`ChatView.tsx:5`) | WARN | No dead transports; single service layer; chat removed. |
| Chat export relative URL bug (`ChatView.tsx:445`) | WARN | Feature removed. |
| Hard-coded cross-project data path (`files.rs:1`) | WARN | `dataRoot` from config (default app data dir); documented migration (§11). |
| Silent JSON→empty + in-place overwrite (`files.rs:62`) | WARN | Typed schemas, reported parse errors, atomic temp+fsync+rename, serialized mutations. |
| Untyped `serde_json::Value` across IPC (`files.rs:82`) | WARN | Typed DTOs with full field validation at the boundary. |
| Optimistic writes discard errors (`CalendarView.tsx:97`) | WARN | Controllers await results, surface failures, no silent divergence. |
| Cosmetic agent selection (`App.tsx:20`) | WARN | Agent selection either drives real detail view or is not presented as functional. |
| Hard-coded legacy roster + global `pgrep` (`agents.rs:44`) | WARN | Roster fully from CLI; per-agent status; no global pgrep. |
| Duplicated hard-coded model allowlist (`models.rs:86`) | WARN | Live model list; validate against server; no hard-coded IDs. |
| Blocking `std::process` in async cmds (`chat.rs:75`) | WARN | All process work on thread pool; async `QProcess`; no `sleep` on worker. |
| New reqwest client per call, no timeout/size bound (`creative.rs:7`) | WARN | Reusable QNAM in `HttpClient`; connect+total timeouts; response-size cap. |
| Stale final stream token (`ChatView.tsx:148`) | WARN | Creative stream assembled in synchronous buffer keyed by requestId; single final snapshot commit. |
| Hard-coded "Connected" / agent counts (`Header.tsx:32`) | WARN | Status bar driven by real `GatewayService` connection state + live agent model counts. |
| GPU % is VRAM ratio + 15% fallback (`vitals.rs:113`) | WARN | Real GPU utilization or `NaN`→"n/a". Never fabricated. |
| Panic points / unwrap on poisoned mutex (`vitals.rs:38`) | WARN | Error propagation; no crash-on-failure on hot paths. |
| Views stay mounted, duplicate polling (`App.tsx:33`) | WARN | Single shared service layer; timers pause when a view is inactive where state retention allows. |
| No pagination/virtualization (`ChatView.tsx:598`) | WARN | Lists use QML delegate recycling (`ListView`/`TableView`) — virtualized by default. |
| Oversized JS bundle (`index.js`) | WARN | Native binary; no JS bundle. |
| Errors flattened to strings / raw stderr (`models.rs` suggestion) | SUGGESTION | `AegisError` enum with stable codes, safe messages, retryability; stderr never surfaced. |

### 6.3 Secret handling rules (normative)

1. The gateway token, git credentials, and any API key are stored **only** in the OS secret store via `SecretStore` (QtKeychain).
2. Secrets are read **only** by `GatewayService` and `GitService`, and only at point of use.
3. No secret is ever: returned across the QML boundary, written to a DTO, placed in a `userMessage`, or emitted to logs (redaction filter enforces the last).
4. Absence of a required secret is **fail-closed**: the operation returns `MissingToken`/`GitAuthFailed`; the UI shows a "credential not configured" state with a link to Settings, and no request/op is attempted.
5. There is no compiled-in, default, or fallback credential anywhere in the source tree.

### 6.4 PathGuard (normative algorithm)

```
resolve(rootCanonical, userRelative):
  reject if userRelative is absolute
  reject if userRelative contains a NUL byte
  joined      = rootCanonical / userRelative
  canonical   = QFileInfo(joined).canonicalFilePath()   // resolves symlinks; empty if missing
  reject (PathNotFound)      if canonical.isEmpty()
  reject (PathOutsideSandbox) unless canonical == rootCanonical
                              or canonical.startsWith(rootCanonical + "/")
  reject (PathSymlinkRejected) if any path component is a symlink escaping root
                               (verified by comparing canonical vs lexically-normalized join)
  for reads: reject unless canonical endsWith ".md" (case-insensitive)
  reject (FileTooLarge)      if size > cap
  return canonical
```

`rootCanonical` is computed once at startup for each allowlisted root. Unit tested with traversal (`../`), absolute paths, symlink-escape, non-`.md`, and oversize fixtures.

### 6.5 QProcess rules (normative)

- Always `QProcess::start(program, QStringList args)`. Never `start(QString commandLine)`, never a shell.
- `program` is a config-resolved absolute path to a validated executable regular file.
- Every spawn has a kill deadline and an output byte cap.
- `stderr` is captured for logging (redacted) only; it is **never** surfaced to the UI as an error message — the UI receives a mapped `AegisError.userMessage`.

---

## 7. Configuration & Settings

### 7.1 ConfigService

Backed by `QSettings` (INI format under the app config dir, `<configDir>/Aegis/aegis.ini`). Typed accessors, validated on read, defaults applied, invalid values rejected with `ConfigInvalid` and reset to default with a logged warning.

Config keys (all overridable; **none** hard-coded in business logic):

| Key | Type | Default | Notes |
|---|---|---|---|
| `gateway.baseUrl` | string(URL) | `http://localhost:PORT` | TLS required unless loopback host |
| `gateway.connectTimeoutMs` | int | 5000 | 1000..60000 |
| `gateway.totalTimeoutMs` | int | 30000 | 1000..300000 |
| `gateway.maxResponseBytes` | int | 8_388_608 | 64 KiB..64 MiB |
| `openclaw.binary` | string(path) | resolved from `PATH` | must be executable file |
| `openclaw.cliTimeoutMs` | int | 20000 | |
| `openclaw.cliOutputCap` | int | 4_194_304 | |
| `data.root` | string(path) | app data dir | calendar + local state |
| `memory.roots` | map<id,path> | `{workspace: <workspace>/memory}` | canonicalized at startup |
| `memory.maxFileBytes` | int | 5_242_880 | |
| `git.repoPath` | string(path) | *unset* | must be a git worktree; unset → Git view shows "configure repo" |
| `git.remoteName` | string | `origin` | |
| `git.pullMode` | enum | `ff-only` | `ff-only` only for v1 |
| `ollama.baseUrl` | string(URL) | `http://localhost:11434` | |
| `vitals.intervalMs` | int | 1000 | 250..10000 |
| `packages.queryCommand` | string[] | platform default | arg array |
| `ui.theme` | enum | `dark` | `dark`\|`light` (light = stretch) |
| `ui.reduceMotion` | bool | false | honors accessibility |
| `log.level` | enum | `info` | `error`\|`warn`\|`info`\|`debug` |

### 7.2 SecretStore

```cpp
class SecretStore : public QObject {
    Q_OBJECT
public:
    QFuture<Result<QString>> read(const QString& key);           // e.g. "gateway.token"
    QFuture<Result<void>>    write(const QString& key, const QString& value);
    QFuture<Result<void>>    erase(const QString& key);
    QFuture<Result<bool>>    has(const QString& key);
};
```

- Implemented over **QtKeychain** (`QKeychain::ReadPasswordJob`/`WritePasswordJob`), service name `dev.tux.aegis`.
- Values never logged. `read` failure due to absence maps to `MissingToken` at call sites that require a secret.
- Settings view offers "Set gateway token" / "Set git credential" flows that write to `SecretStore`; the token is entered in a masked field, submitted to C++, and never echoed back to QML.

---

## 8. IPC / C++ ↔ QML Communication

### 8.1 Exposure mechanism

At startup `AppContext` constructs services + controllers and registers controllers as context properties / singletons:

```cpp
engine.rootContext()->setContextProperty("app",       appController);
engine.rootContext()->setContextProperty("agents",    agentController);
engine.rootContext()->setContextProperty("vitals",    vitalsController);
engine.rootContext()->setContextProperty("calendar",  calendarController);
engine.rootContext()->setContextProperty("cron",      cronController);
engine.rootContext()->setContextProperty("memory",    memoryController);
engine.rootContext()->setContextProperty("models",    modelController);
engine.rootContext()->setContextProperty("packages",  packageController);
engine.rootContext()->setContextProperty("git",       gitController);
engine.rootContext()->setContextProperty("creative",  creativeController);
engine.rootContext()->setContextProperty("settings",  settingsController);
```

Enums (`AgentStatus`, `CronState`, `GitFileState`, `ErrorCode`, `ConnectionState`, `CreativeBackend`) are registered with `qmlRegisterUncreatableMetaObject` / `Q_ENUM` so QML can reference them by name.

### 8.2 Controller contract

Each controller exposes:

- **State** via `Q_PROPERTY` with `NOTIFY` signals (e.g. `Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)`).
- **List data** via a `QAbstractListModel*` property consumed by QML `ListView`/`TableView` delegates (virtualized).
- **Intents** via `Q_INVOKABLE` methods that accept only primitive/validated params (strings, ints, bools) — never raw JSON, never file handles.
- **Feedback** via signals: `errorRaised(QString userMessage, bool retryable)`, `toast(QString message, int level)`.

Example — `GitController`:

```cpp
class GitController : public QObject {
    Q_OBJECT
    Q_PROPERTY(GitFileModel* files READ files CONSTANT)
    Q_PROPERTY(QString branch READ branch NOTIFY statusChanged)
    Q_PROPERTY(int ahead READ ahead NOTIFY statusChanged)
    Q_PROPERTY(int behind READ behind NOTIFY statusChanged)
    Q_PROPERTY(bool clean READ clean NOTIFY statusChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(bool repoConfigured READ repoConfigured NOTIFY statusChanged)
public:
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void stagePath(const QString& repoRelativePath);
    Q_INVOKABLE void unstagePath(const QString& repoRelativePath);
    Q_INVOKABLE void commit(const QString& message);     // controller enforces confirm + staged set
    Q_INVOKABLE void pull();
    Q_INVOKABLE void push();
signals:
    void statusChanged();
    void busyChanged();
    void errorRaised(QString userMessage, bool retryable);
    void confirmRequested(QString action, QString detail); // QML shows ConfirmDialog, calls back
};
```

Destructive/remote intents (`commit`,`pull`,`push`) go through a **two-phase confirm**: controller emits `confirmRequested`; QML shows `ConfirmDialog`; on accept QML calls the corresponding `confirm*` invokable. No destructive op proceeds without explicit UI acknowledgment.

### 8.3 No secret ever crosses

Controllers do not expose any property or method that returns a token, header, absolute credential path, or raw stderr. This is verified by a unit test that reflects over each controller's meta-object and asserts no method/property name matches a secret denylist (`token`, `secret`, `password`, `bearer`, `credential`).

### 8.4 Streaming pattern (Creative)

`CreativeController` exposes `Q_INVOKABLE QString generate(...)` returning a `requestId`, plus signals `chunk(requestId, delta)`, `finished(requestId, resultText, finishReason)`, `failed(requestId, msg)`, and `Q_INVOKABLE void cancel(requestId)`. QML binds by matching `requestId`. Assembly buffer is synchronous in the controller; the final snapshot is committed once on `finished`.

### 8.5 Default timeouts / caps (single source table)

| Operation | Connect | Total | Output cap |
|---|---|---|---|
| Gateway GET/POST | 5 s | 30 s | 8 MiB |
| Gateway stream | 5 s | 300 s (idle 60 s) | 16 MiB |
| Ollama generate | 5 s | 300 s (idle 60 s) | 16 MiB |
| openclaw CLI | n/a | 20 s | 4 MiB |
| Package query | n/a | 30 s | 8 MiB |
| Git network (pull/push) | 10 s | 120 s | n/a |
| Memory read | n/a | n/a | 5 MiB |

---

## 9. Build System

### 9.1 Requirements

- **Qt 6.6+** modules: `Core`, `Gui`, `Qml`, `Quick`, `QuickControls2`, `Concurrent`, `Network`, `Test` (tests only). Optional `Svg` for icons.
- **C++20** compiler (GCC 13+ / Clang 16+).
- **CMake 3.24+**.
- **libgit2 1.7+** (config package `unofficial-libgit2` or system `libgit2` + `cmake/FindLibgit2.cmake`).
- **QtKeychain** (`Qt6Keychain` / `qtkeychain-qt6`).
- **tl::expected** (single header) unless the toolchain provides `std::expected` (C++23) — selected via a CMake feature check.

### 9.2 Top-level CMake sketch

```cmake
cmake_minimum_required(VERSION 3.24)
project(aegis LANGUAGES CXX VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.6 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 Concurrent Network Svg)
find_package(Qt6Keychain REQUIRED)
find_package(Libgit2 REQUIRED)          # cmake/FindLibgit2.cmake fallback

qt_standard_project_setup(REQUIRES 6.6)

add_subdirectory(src)                   # builds aegis_core (services/dto/controllers) static lib
qt_add_executable(aegis src/main.cpp)
qt_add_qml_module(aegis
    URI Aegis
    VERSION 1.0
    QML_FILES ${AEGIS_QML_FILES}
    RESOURCES resources/aegis.qrc
)
target_link_libraries(aegis PRIVATE aegis_core
    Qt6::Quick Qt6::QuickControls2 Qt6::Concurrent Qt6::Network Qt6::Svg
    Qt6Keychain::Qt6Keychain Libgit2::Libgit2)

include(cmake/AegisWarnings.cmake)      # -Wall -Wextra -Werror on aegis_core
enable_testing()
add_subdirectory(tests)
```

- `aegis_core` is a static library holding `dto/`, `core/`, `config/`, `services/`, `controllers/`, `models/` — **no QML dependency** — so services/controllers are unit-testable headless. Only `main.cpp`, QML, and `QmlRegistration` depend on Qt Quick.
- Warnings-as-errors on `aegis_core`. Address/UB sanitizers enabled in a `Debug+ASAN` preset.
- `qtquickcontrols2.conf` forces the `Basic` style with a dark palette (no platform theme drift).

### 9.3 Packaging

- Linux primary target. Bundle via `linuxdeploy` + `linuxdeploy-plugin-qt` into an AppImage, or a Flatpak manifest (`dev.tux.aegis.yaml`) as the preferred distributable (sandboxing bonus). Fonts vendored under OFL/SIL are bundled in `resources/fonts/`.
- No sidecar binary is bundled or required (the old port-3851 service is gone).

---

## 10. Testing Strategy

### 10.1 Unit tests (QtTest, headless, on `aegis_core`)

- `tst_pathguard` — traversal (`../`), absolute path, symlink-escape, non-`.md`, oversize, NUL byte, valid-contained. **Security-critical, required.**
- `tst_atomicfile` — temp+rename atomicity, fsync path, failure leaves original intact, concurrent-mutation serialization.
- `tst_aegiserror` — code→userMessage mapping never leaks debugContext; retryable flags correct.
- `tst_calendarstore` — validation rejects bad fields; parse error does not coerce to empty; parse error refuses overwrite; round-trip create/update/remove.
- `tst_memoryservice` — allowlist enforcement, `.md` requirement, size cap, symlink reject.
- `tst_gitservice` — open/validate worktree on a fixture repo; explicit-path staging stages exactly the given paths and nothing else; commit requires staged set; ff-only pull reports conflict on non-ff; no code path invokes add-all.
- `tst_httpclient` — connect/total timeout fires; response-size cap aborts; non-2xx → `HttpStatus`; malformed body → `ResponseMalformed`.
- `tst_secretstore` — write/read/erase round-trip (mock keychain backend); absent → MissingToken mapping.
- `tst_openclawcli_parse` — parse agent/cron/model fixtures into DTOs; malformed → `CliOutputMalformed`; oversize output rejected.

### 10.2 Security regression tests (required gate)

Dedicated suite asserting the closed blockers stay closed:

- No controller meta-object exposes a secret-named method/property (reflection test).
- No source file contains a literal bearer/token pattern (build-time `grep` gate in CI).
- `GatewayService` sends no request when token absent (fail-closed).
- Non-loopback `http://` base URL is rejected by config validation.
- Memory read of a symlink escaping root is rejected.

### 10.3 Integration / smoke

- Headless `QGuiApplication`-less service integration against a mock gateway (`QTcpServer` stub) and a temp git repo.
- QML load smoke: `qmlscene`/`qmllint` over all QML files in CI (catches binding/type errors).
- Manual installed-binary smoke checklist per release (dashboard renders real vitals, roster populates from CLI, git status against real repo, memory read of a real `.md`).

### 10.4 Static analysis & supply chain

- `clang-tidy` + `cppcheck` in CI.
- `qmllint` clean.
- Dependency audit: pin Qt/libgit2/QtKeychain versions; document any accepted advisory. No unmaintained transitive web stack (the old npm/RustSec debt does not exist in this native tree).

### 10.5 CI gates (must pass before merge)

`cmake --build` (Release + Debug+ASAN) · all unit tests · security regression suite · `qmllint` · `clang-tidy` · secret-pattern grep gate.

---

## 11. Migration Notes

Old AEGIS-predecessor data lived at hard-coded cross-project paths under `projects/andys-overview/data` (calendar, ideas, missions). Migration policy:

1. **Calendar** is the only carried-over persisted store. On first launch, if `data.root` has no `calendar/events.json` and a legacy `events.json` (or the old calendar file) is found at the documented legacy path, AEGIS offers a **one-time import**: it reads the legacy file, validates every event through `CalendarEventDto::fromJson`, drops/logs invalid entries (never crashes), and writes a fresh atomic `events.json` under `data.root`. Legacy file is left untouched (non-destructive).
2. **Ideas / Missions** data is **not** migrated (features dropped). It is left in place; AEGIS never reads or writes it.
3. **Chat history / exports** are not migrated (feature dropped).
4. **Secrets:** the old committed token is treated as **compromised**. Migration explicitly does NOT import it. The operator must set a fresh token in Settings → it is written to `SecretStore`. The old token must be rotated server-side and scrubbed from git history (tracked as a release precondition, out of app scope).
5. **Config:** no automatic import of old `tauri.conf.json`. New `aegis.ini` is generated with defaults; the operator sets `git.repoPath`, `gateway.baseUrl`, and `memory.roots` on first run via Settings.
6. **Repo path:** the old nonexistent hard-coded git path is discarded. Git view starts in "configure repository" state until `git.repoPath` is set and validated.

Migration runs off-thread, is idempotent (guarded by a `migration.calendarImported` config flag), and reports outcome via a toast.

---

## 12. Open Questions / v1 Cut Lines

- **Light theme** is a stretch goal; v1 ships dark ("Midnight Command") only, but tokens are theme-swappable (§UI_SPEC).
- **Non-ff git pull** (merge/rebase) is deferred; v1 is ff-only with a clear conflict message.
- **Per-core CPU sparklines** are optional if they threaten the 60 fps budget.
- **Windows/macOS** ports are out of v1 scope but the layering (only `main.cpp`/QML touch Qt Quick; vitals behind a `VitalsService` interface) keeps them feasible.

---

*End of SPEC.md*
