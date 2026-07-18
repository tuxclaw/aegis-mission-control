# AEGIS Mission Control — Connection & "Malformed Data" Diagnosis

_Investigated 2026-07-17. All findings below are backed by reading the actual
source and by live-testing the OpenClaw gateway + CLI on this host._

## TL;DR

Five independent defects, not one:

| # | Symptom | Root cause | Severity |
|---|---------|-----------|----------|
| 1 | "Disconnected" | Gateway token read from the **wrong JSON path**; AND nothing ever pings the gateway on startup; AND client uses **`/api/...` endpoints that return 404** | High |
| 2 | Fleet empty + "malformed data" toast | `AgentDto::fromJson` **schema does not match** real `openclaw agents list --json` output → `rejectUnknown` fails → `CliOutputMalformed` | High |
| 3 | 2nd "malformed data" toast | Same class of bug in `CronJobDto` and `ModelDto` (schemas don't match the CLI) | High |
| 4 | "repository could not be opened" | `git.repoPath` is unset, but git refresh runs on every load and raises `GitOpenFailed` as a **toast** instead of a neutral empty state | Medium |
| 5 | CPU/MEM 0%, GPU n/a, disks empty | The vitals sample never reaches QML, and the Dashboard has **no error wiring for vitals**, so it fails silently at 0 | Medium |

The evidence for each follows.

---

## Symptom 1 — "Disconnected"

### 1a. Token is read from the wrong path (fails closed → MissingToken)

`src/app/app_context.cpp:52-56`:

```cpp
// Token is at top-level auth.token (not gateway.auth.token)
const auto token = root.value(QStringLiteral("auth")).toObject()
                       .value(QStringLiteral("token")).toString();
if (!token.isEmpty()) {
  (void)secretStore_->write(QStringLiteral("gateway.token"), token);
}
```

The code comment is **factually wrong** for this install. Live check of
`~/.openclaw/openclaw.json`:

```
$ jq '.auth.token'          -> null  (top-level auth has only "profiles"/"order")
$ jq '.gateway.auth.token'  -> "781fafa0…2bd0"   (the real token lives HERE)
```

So `token` is empty, nothing is written to the SecretStore, and every gateway
call in `GatewayService` fails closed with `ErrorCode::MissingToken`
(`gateway_service.cpp` `get()`/`postJson()` both bail when
`secrets_->read("gateway.token")` returns empty).

**Fix** — `src/app/app_context.cpp:52-54`. Read `gateway.auth.token` first and
fall back to top-level `auth.token`:

```cpp
const auto gatewayAuth = root.value(QStringLiteral("gateway")).toObject()
                             .value(QStringLiteral("auth")).toObject();
auto token = gatewayAuth.value(QStringLiteral("token")).toString();
if (token.isEmpty()) {
  token = root.value(QStringLiteral("auth")).toObject()
              .value(QStringLiteral("token")).toString();
}
```

### 1b. No startup/heartbeat gateway health-check → state is stuck at Disconnected

`GatewayService::connectionState_` initialises to
`ConnectionState::Disconnected` (`gateway_service.h`) and only changes inside
`get()` / `postJson()` / `beginStream()`. A grep of every caller shows the
**only** consumer of those methods is `CreativeService` (chat streaming):

```
$ grep -rn 'gateway_->get|gateway_->postJson' src/controllers src/services
   (no hits outside creative_service / gateway_service itself)
```

Therefore, unless the user opens the Creative view and sends a message, the
gateway is never contacted and the StatusBar shows "Disconnected" **forever** —
even with a valid token. `AppController::refreshAll()` fans out to agents,
calendar, cron, memory, models, packages, git — **none** of which touch the
gateway (agents/cron/models go through the CLI, not HTTP).

**Fix** — add a lightweight health probe:
- Give `GatewayService` a `ping()` that does `get("status")` (see 1c for the
  path), updating `connectionState`.
- Call it from `AppController` on construction and on each `refreshAll()` (or a
  small QTimer). Wire the existing `connectionStateChanged` signal (already
  connected in `app_controller.cpp`) so the StatusBar reflects reality.

### 1c. Client hits `/api/...` endpoints that don't exist (404)

Live probe against the running gateway (`localhost:18789`, valid Bearer token):

```
/            -> 200      /api/health   -> 404
/health      -> 200      /api/status   -> 404
/status      -> 200      /api/agents   -> 404
/agents      -> 200      /api/v1/status-> 404
```

The real gateway API has **no `/api` prefix**. Any request path with `/api/...`
(or an `/api` base) will 404 → `HttpStatus` error → `ConnectionState::Error`.
`authorizedRequest()` (`gateway_service.cpp`) cleanly joins base + relative path,
so the health check and any REST calls must use bare paths like `status`,
`agents`, `health`.

**Fix** — ensure every gateway path passed into `get/postJson` omits `/api`,
and use `"status"` for the health probe. Confirm `gateway.baseUrl`
(default `http://localhost:18789`, `config_service.cpp:18`) has no `/api`
suffix.

---

## Symptom 2 & 3 — Empty Fleet + "The program returned malformed data"

`"The program returned malformed data."` is the user message for
`ErrorCode::CliOutputMalformed` (`aegis_error.cpp:50`). It is produced by
`OpenClawCli::parseArray<Dto>()` (`openclaw_cli.cpp`) whenever `Dto::fromJson`
rejects an item. The CLI **stdout is clean JSON** (warnings go to stderr, and the
service uses `SeparateChannels`), so the failure is **purely a DTO schema
mismatch**. The DTOs use `json::rejectUnknown`, which fails on the **first**
unexpected key (`json_validation.cpp:74`).

### 2a. AgentDto — does not match `openclaw agents list --json`

Real keys (live):
`agentDir, bindings, id, identityEmoji, identityName, identitySource,
isDefault, model, name, workspace`

`AgentDto::fromJson` allow-list (`agent_dto.cpp:39-43`):
`id, displayName, model, status, statusDetail, lastSeen, activeSessions,
avatarSeed`

- `rejectUnknown` trips immediately on `agentDir` → `ValidationFailed` →
  wrapped as `CliOutputMalformed`.
- Even past that, the DTO requires `displayName` (CLI has `name`/`identityName`)
  and reads `status` (CLI has none).

**Result:** every agent item fails → Fleet at a Glance is empty + one
"malformed data" toast.

**Fix** — `src/dto/agent_dto.cpp`. Rewrite `fromJson` to the real schema:
- Expand `rejectUnknown` to include the real keys (or drop it for CLI DTOs and
  ignore unknowns).
- Map `displayName` from `name` → `identityName` → `id`.
- `status` isn't emitted by the CLI; default to `Unknown` (already handled) and
  drop it from `rejectUnknown` requirements.
- Optionally surface `model`, `workspace`, `isDefault`.

### 2b. CronJobDto — does not match `openclaw cron list --json`

Real keys (live):
`agentId, createdAtMs, delivery, description, enabled, id, lastDelivered,
lastDeliveryStatus, lastFailureNotificationDeliveryStatus, lastRunAtMs,
lastRunStatus, name, nextRunAtMs, payload, schedule, sessionTarget, state,
status, updatedAtMs, wakeMode`

`CronJobDto::fromJson` allow-list (`cron_job_dto.cpp:34-37`):
`id, name, schedule, state, command, lastRun, nextRun, lastResult`

- `rejectUnknown` trips on `agentId` (and ~11 others).
- Timestamps are epoch-ms fields (`lastRunAtMs`, `nextRunAtMs`), not the
  ISO-ish `lastRun`/`nextRun` the DTO expects.

**Fix** — `src/dto/cron_job_dto.cpp`: expand/relax `rejectUnknown`, read
`lastRunAtMs`/`nextRunAtMs` as epoch-ms, and map `state`/`enabled` correctly.

### 2c. ModelDto — does not match `openclaw models list --json`

Real keys (live):
`available, contextWindow, input, key, local, missing, name, tags`

`ModelDto::fromJson` allow-list (`model_dto.cpp:9-10`) requires:
`id, provider, label, isActive`

- Every required field is **absent** (there's no `id`, `provider`, `label`, or
  `isActive`), and every present field is "unknown" → guaranteed failure.

**Fix** — `src/dto/model_dto.cpp`: use `key` as id, derive `provider` from the
`key` prefix before `/`, use `name` (or `key`) as label, and derive `isActive`
from `tags` containing `"default"`/`"configured"` or from `available`.

> Note: `cron list`/`models list` also print a `Config warnings:` banner, but to
> **stderr**; the CLI runner reads stdout separately, so this is not the cause.
> The cause is strictly the DTO schemas.

---

## Symptom 4 — "The configured repository could not be opened"

That string is the user message for `ErrorCode::GitOpenFailed`
(`aegis_error.cpp:56`). Path:

- `git.repoPath` QSetting is unset. `ConfigService::gitRepoPath()`
  (`config_service.cpp:179-189`) returns an **empty string** (not an error).
- `GitService::validatedRepoPath()` (`git_service.cpp:24-28`) turns the empty
  path into `GitOpenFailed("repository not configured")`.
- `AppContext` wires `refreshAll → GitController::refresh`
  (`app_context.cpp:72-73`), so git refreshes on **every** load and the
  controller raises the error as a toast.

`GitController::repoConfigured()` (`git_controller.cpp:16`) already exists but
isn't used to suppress the error.

**Fix (choose one):**
- Preferred: in `GitController::refresh()` / `GitService`, treat an **empty**
  `git.repoPath` as a neutral "not configured" empty state and **do not** emit
  `errorRaised`. Only raise a toast when a *configured* path fails to open.
- Or: default `git.repoPath` to a real repo (e.g. the workspace) in
  `config_service.cpp`.

---

## Symptom 5 — CPU/MEM 0%, GPU n/a, disks empty

All four vitals readouts are empty **together**, which means the vitals sample
is not reaching the controller (a single successful sample would populate MEM,
disk, and GPU immediately — CPU is legitimately 0 only on the *first* sample
because it needs a delta).

Important context found:
- The `/proc` + `statvfs` parsing in `vitals_service.cpp` is **correct for this
  host** (verified: `/proc/stat` cpu line has 11 fields, `MemAvailable` present,
  3 non-lo interfaces, `statvfs("/")` works). So the logic itself is sound here.
- GPU hardware **is** present (`/sys/class/drm/card0/device/gpu_busy_percent`
  and `card1` exist), so `readGpu()` should return `available=true`. GPU showing
  "n/a" therefore also indicates **no sample emitted**, not a GPU-detection bug.
- Crucially, `DashboardView.qml` only wires **agents** errors to a toast
  (`Connections { target: agents; onErrorRaised … }`, lines ~53-58). There is
  **no** `Connections { target: vitals; onErrorRaised … }`. So if
  `VitalsService` emits `failed`, it is **swallowed silently** and the gauges
  simply stay at their defaults (0 / NaN) — exactly what the screenshot shows.

**What this means:** vitals is failing (or not starting) at runtime and the UI
hides it. Because the parsing is valid on this host, likely candidates are:
(a) the running binary predates a fix / wasn't rebuilt, (b) the process runs in a
sandbox (e.g. Flatpak) with a restricted `/proc`/`/sys`, or (c) the
`QFutureWatcher`/thread-pool sample never completes so `sampling_` stays `true`
and no further samples run.

**Fixes:**
1. `DashboardView.qml` — add
   `Connections { target: vitals; function onErrorRaised(message, canRetry){ root.errorMessage = message } }`
   so vitals failures become visible instead of silent 0s. (This is the concrete
   code defect: vitals errors are unhandled in the view.)
2. Add a debug log in both `VitalsService::sampleNow` branches (the `sampled` and
   `failed` emits) and run the app once to capture the real runtime error.
3. Verify a fresh build is actually being run, and check whether the app is
   sandboxed (restricted `/proc`).
4. Minor robustness: `readGpu()` hardcodes `hwmon/hwmon0/temp1_input`
   (`vitals_service.cpp`); the hwmon index isn't always `0`. This only affects
   GPU temperature, not the util/availability, so it is not the cause here, but
   worth globbing `hwmon/hwmon*`.

> The screenshot's "Disks: Unable to load — malformed data" is the **global
> error toast** (fed by the CLI failures in Symptoms 2/3) overlaying the disk
> card. The disk card itself (`DashboardView.qml` ~141-204) binds to
> `vitals.diskModel` and only has a neutral "No disk data" empty state — it has
> no "malformed" path of its own. The disk DTO serialization is **not** broken.

---

## Priority order for fixing

1. **DTO schemas** (Symptoms 2 & 3) — biggest visible win: restores Fleet,
   Cron, Models, and kills both "malformed data" toasts. Files:
   `dto/agent_dto.cpp`, `dto/cron_job_dto.cpp`, `dto/model_dto.cpp`.
2. **Gateway token path** (1a) — one-line-ish fix in `app/app_context.cpp`.
3. **Gateway health probe + drop `/api` prefix** (1b, 1c) — makes the StatusBar
   truthful.
4. **Git empty-state vs error** (4) — stop the false "repository" toast.
5. **Vitals error wiring + runtime log** (5) — surface the real vitals failure,
   then fix whatever it reveals.

## Quick verification commands used

```bash
# Token location (real token is under gateway.auth, not top-level auth)
jq '.auth.token, .gateway.auth.token' ~/.openclaw/openclaw.json

# Endpoint prefix (no /api)
TOKEN=$(jq -r .gateway.auth.token ~/.openclaw/openclaw.json)
curl -s -o /dev/null -w '%{http_code}\n' -H "Authorization: Bearer $TOKEN" \
     http://localhost:18789/status        # 200
curl -s -o /dev/null -w '%{http_code}\n' -H "Authorization: Bearer $TOKEN" \
     http://localhost:18789/api/status     # 404

# Real CLI shapes vs DTO expectations
openclaw agents list --json 2>/dev/null | jq '[.[]|keys[]]|unique'
openclaw cron   list --json 2>/dev/null | jq '[.jobs[0]|keys]'
openclaw models list --json 2>/dev/null | jq '[.models[0]|keys]'
```
