# Build History

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
