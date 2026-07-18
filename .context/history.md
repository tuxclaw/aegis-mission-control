# Build History

## [2026-07-18] Dashboard Layout Follow-up
**Agent:** Dash ⚡
**Branch:** `main` (explicit task requirement)

- Added intentional disk-card sizing and an empty state, plus container-list top padding and a tighter Processes height cap.
- Aligned Fleet name, status, and model columns; increased small dashboard text readability; and brightened the global muted-text token.
- Shortened Network gauge values to compact B/K/M/G notation while retaining full directional rates in the subtitle.
- Verified QML parsing/lint, a successful GUI build, all 12 QtTest suites, installed-artifact parity, and an offscreen runtime smoke test.

**Status:** Complete

## [2026-07-18] Gemini CLI and Xiaomi passToken Follow-up
**Agent:** Helen 🦸‍♀️
**Branch:** `main` (explicit task requirement)

- Replaced Gemini's internal OAuth HTTP quota calls with a Gemini CLI API-key availability probe using `--skip-trust` and a process-local `GEMINI_API_KEY`.
- Added Xiaomi `passToken` fallback after existing cookie and service-token credentials, plus explicit environment/config credential mapping.
- Re-enabled Xiaomi in the local monitor config with the Firefox account token and user ID without committing credentials.
- Verified a successful GUI/test build, all 12 QtTest suites, and an exact `OK` response from the Gemini CLI.
- A live Xiaomi balance request returned HTTP/API 401, confirming that the account `passToken` is attempted correctly but is not accepted as a platform service token.

**Status:** Complete; Xiaomi requires a platform login for successful quota data

## [2026-07-18] Provider Quota Fetcher Repair
**Agent:** Helen 🦸‍♀️
**Branch:** `main` (explicit task requirement)

- Replaced Anthropic's nonexistent `/v1/usage` call with the Claude OAuth usage endpoint and mapped five-hour, weekly, scoped, and extra-usage windows.
- Replaced legacy OpenAI dashboard billing calls with the Codex OAuth subscription usage endpoint and its primary/weekly rate-limit schema.
- Removed the nonexistent Grok `usage` subcommand and added honest 30-day local session accounting from `~/.grok/sessions/**/signals.json`.
- Removed Gemini's quota-consuming `generateContent` probe, added Gemini CLI OAuth quota support, and made API-key-only installs report that quota data is unavailable.
- Normalized Xiaomi MiMo Cookie headers/copied curl commands, accepted split cookie credentials, scanned all Firefox profiles, and hardened partial/error response parsing.
- Verified a warnings-as-errors GUI/test build, all 12 QtTest suites, a live provider probe, and an offscreen runtime smoke test.

**Status:** Complete

## [2026-07-18] Provider Usage Page Frontend
**Agent:** Dash ⚡
**Branch:** `main` (explicit task requirement)

- Added a responsive one-, two-, or three-column provider quota grid with animated threshold-colored usage bars, token summaries, balances, fetch timestamps, and provider/global error states.
- Added Usage at navigation index 8, shifted Settings to index 9, and wired both view-level and shell-level refresh actions to the forthcoming `Usage` singleton.
- Added the Usage chart icon and registered the new view and icon in the QML/resource manifests.
- Verified QML parsing, XML/SVG validation, a successful GUI build, and all 12 QtTest executables. The offscreen smoke test reaches the new view but awaits the parallel backend singleton registration before it can resolve `Usage`.

**Status:** Complete; backend integration is recorded below

## [2026-07-18] Provider Quota Tracking Backend
**Agent:** Helen 🦸‍♀️
**Branch:** `main` (explicit task requirement)

- Ported the provider quota DTO, fetcher base, manager, eight provider
  fetchers, and legacy monitor configuration adapter from openclaw-monitor.
- Added the QML-facing usage controller and list model, including provider,
  window, balance, plan, error, and fetch-time fields.
- Wired provider usage through AppContext and registered the `Usage` singleton
  expected by the Usage view without modifying QML.
- Added Qt Network/SQL build integration for HTTP requests and read-only
  Firefox cookie lookup.
- Verified a warnings-as-errors GUI build, all 12 QtTest executables, and an
  offscreen smoke test with no usage singleton or QML load errors.

**Status:** Complete

## [2026-07-18] Liquid Glass UI Integration
**Agent:** Dash ⚡
**Branch:** `main` (explicit task requirement)

- Bundled the QML LiquidGlass module and retuned its material tokens to the AEGIS Midnight Command palette.
- Split the window into sibling `scene` and `uiLayer` items so every glass surface captures only the gradient, aurora, and radar backdrop without render recursion.
- Replaced the sidebar, status bar, and shared card surface with real blurred glass; passed the scene reference through all nine views and enabled live capture on moving cards.
- Verified a warnings-as-errors GUI build, all 12 QtTest executables, QML format/lint parsing, and an offscreen runtime smoke test.

**Status:** Complete

## [2026-07-18] Container and Process Monitoring Backend
**Agent:** Helen 🦸‍♀️
**Branch:** `main` (explicit task requirement)

- Added strict container/process sample DTOs with stable JSON round trips.
- Added asynchronous ten-second Podman inventory sampling with graceful empty
  fallback and five-second `/proc` process sampling with delta CPU, RSS memory,
  username, and bounded command-line collection.
- Added QAbstractListModels, refreshable controllers, AppContext ownership,
  `Containers`/`Processes` QML singletons, and model type registration.
- Added focused DTO parsing regressions and expanded the controller reflection
  security gate to cover both new controllers.
- Verified a warnings-as-errors GUI build, all 12 QtTest executables, and an
  offscreen application smoke test.

**Status:** Complete

## [2026-07-17] Disk, GPU, and Agent Data Accuracy
**Agent:** Helen 🦸‍♀️
**Branch:** `main` (explicit task requirement)

- Detected the read-only Bazzite composefs root and sampled its writable `/var`
  filesystem instead, using `f_bavail` for user-visible disk consumption.
- Sampled every AMD DRM card on every vitals tick and reported the highest live
  utilization, with temperature discovery across real hwmon directory names.
- Enriched configured agents from `openclaw sessions --all-agents --json`,
  mapping running, completed, and failed sessions to active/idle/error status.
- Verified a warnings-as-errors GUI build, all 10 QtTest suites, and an
  offscreen trace with repeated `/var` samples and non-zero `card1` GPU load.

**Status:** Complete

## [2026-07-17] Memory, Disk, GPU, and Creative Backend Fixes
**Agent:** Helen 🦸‍♀️
**Branch:** `main` (explicit task requirement)

- Pointed the default and reset memory allowlist at the OpenClaw workspace,
  loaded memory on controller startup, and added filesystem diagnostics plus
  service/controller regression coverage.
- Made root-disk sampling fail visibly instead of silently returning an empty
  model, reported real total/used bytes, and added `statvfs` diagnostics.
- Hardened AMD DRM utilization probing to skip invalid card readings while
  preserving the muted `n/a` fallback when no usable sysfs value exists.
- Removed Creative service/controller construction and QML context exposure
  while retaining their sources and focused tests for future reuse.
- Verified a warnings-as-errors GUI build, all 10 QtTest suites, QML lint, and
  an offscreen runtime trace with 106 memory files, one root disk, and live AMD
  GPU utilization.

**Status:** Complete

## [2026-07-17] UI Polish
**Agent:** Dash ⚡
**Branch:** `main` (explicit task requirement)

- Removed the animated radar sweep and reduced the static grid-dot opacity.
- Disabled overshoot across scrollable views and clipped their content.
- Made the Calendar editor an explicitly dismissible 260 ms right-side drawer opened by a labeled header action.
- Removed the Creative view, navigation entry, and QML resources.
- Added dimmed `n/a` presentation for unavailable GPU and disk data while preserving the `/` mount label when sampled.

**Status:** Complete

## [2026-07-17] Vitals and Agent Status Remediation
**Agent:** Helen 🦸‍♀️
**Branch:** `main` (explicit task requirement)

- Replaced zero-size-sensitive `/proc` EOF checks so CPU, memory, and network
  samples reach the controller; added opt-in `aegis.vitals` diagnostics.
- Mapped structured agent statuses, treated status-less configured inventory as
  idle, and exposed cached active/total counts directly to QML.
- Added live vitals/controller and CLI schema regression coverage.
- Verified a warnings-as-errors GUI build, all 10 QtTest suites, QML lint
  execution, and an offscreen runtime trace with non-zero CPU and real memory,
  disk, and network samples.

**Status:** Complete

## [2026-07-17] Frontend Foundation
**Agent:** Dash ⚡
**Branch:** `andy/foundation-frontend`

- Added the Midnight Command `Theme`, `Typography`, and `Motion` QML singletons.
- Added 14 reusable QML components covering cards, gauges, status, navigation, dialogs, toasts, state surfaces, and button variants.
- Added the native `ApplicationWindow` shell with responsive sidebar, top bar, static view stack, radar background, and property-driven status bar.
- Bundled seven SIL OFL font weights, eleven SVG icons, the Basic Controls configuration, and every QML source through `resources/aegis.qrc`.
- Verified all QML with Qt 6.11 `qmllint`, loaded `Main.qml` offscreen with `qmlscene`, validated XML/SVG files, and compiled the resource collection with `rcc`.

**Status:** Complete

## [2026-07-17] Security Review Warnings and Suggestions
**Agent:** Dash ⚡
**Branch:** `main`

- Removed redundant canonical-root cleanup and made creative request ranges reject invalid values.
- Added incremental Ollama NDJSON delivery through the managed HTTP client's `readyRead` path, including partial-line buffering and final unterminated-frame handling.
- Paired successful process-wide libgit2 initialization with guarded shutdown and capped package output at 100,000 lines.
- Added a strict controller meta-object denylist gate plus creative range and incremental-streaming regression coverage.
- Verified a clean warnings-as-errors core build and all 8 QtTest executables from a detached `main` worktree.

**Status:** Complete

## [2026-07-17] Controller-Backed Mission Control Views
**Agent:** Dash ⚡
**Branch:** `andy/views`

- Built Dashboard, Agent Roster, Calendar, Cron, Memory, Models, Packages, Git, Creative, and Settings views against the declared C++ controller/model contracts.
- Replaced the placeholder stack with real navigation, contextual refresh/add actions, live status bindings, Git-to-Settings routing, and the complete QML resource manifest.
- Added empty/loading/error surfaces to every view, explicit per-path Git staging and confirmations, a guarded Calendar editor drawer, inert memory/creative text rendering, masked Settings secrets, and reduced-motion-aware staggered card entry.
- Verified all view QML with Qt 6.11 `qmllint`, compiled the resource manifest with `rcc`, passed `git diff --check`, and built the available C++ core target. The immutable host lacks installed Qt Quick development modules, so the GUI CMake target remains gated until those modules are present.

**Status:** Complete

## [2026-07-17] Critical Runtime Defect Remediation
**Agent:** Helen 🦸‍♀️
**Branch:** `main` (explicit task requirement)

- Corrected gateway token discovery, added startup and 30-second health probes,
  and kept connection state aligned with successful gateway transport responses.
- Remapped agent, cron, and model DTOs to the live OpenClaw CLI JSON schemas.
- Made an unset Git repository a neutral empty state and forwarded vitals
  failures through both controller error and toast signals.
- Added live-schema parser fixtures; warnings-as-errors build and all 9 QtTest
  suites pass, and an offscreen application smoke test stayed running.

**Status:** Complete
