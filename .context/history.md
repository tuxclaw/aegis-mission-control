# Build History

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
