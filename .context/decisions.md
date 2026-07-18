# Decisions

## [2026-07-18] Backdrop Glass Uses Sibling Scene Capture
**By:** Tux + Dash ⚡
**Context:** Real blur requires `ShaderEffectSource` to capture content behind a surface without capturing itself or an ancestor.
**Decision:** Keep all background visuals in a root-level `scene` item and all controls in a sibling `uiLayer`; pass the scene reference through each view to LiquidGlass surfaces, with live tracking only for moving or scrolling cards.
**Status:** Active

## [2026-07-17] Full Rebrand: AO → AEGIS
**By:** Tux + Jeff 🛡️
**Context:** Complete rebuild/rebrand of Andy's Overview Tauri app. Old app had 6 BLOCKER security findings.
**Decision:** New name AEGIS (shield of Athena). Qt 6 / QML / C++ / CMake. No webview. App ID: `dev.tux.aegis`.
**Status:** Active

## [2026-07-17] Drop Chat, Missions, Ideas, Creative UI
**By:** Tux
**Context:** Chat system was primary XSS/CSP attack surface. Mission Board had hydration race. Idea Board was low-value.
**Decision:** Drop Chat, Missions, Ideas, and the Creative UI. Carry over: Agent Roster, Dashboard, Calendar, Cron, Memory, Models, Packages, and Git.
**Status:** Active

## [2026-07-17] Security Architecture
**By:** Jeff 🛡️
**Context:** Old app compiled gateway token into binary, returned it to webview, had unsafe CSP, shell injection surface.
**Decision:** Qt Keychain for secrets (fail-closed), no webview, QProcess arg arrays only, libgit2 not shell, PathGuard for file access, atomic writes, structured error enum.
**Status:** Active

## [2026-07-17] UI Theme: Midnight Command
**By:** Jeff 🛡️
**Context:** Need distinctive, security-forward visual identity.
**Decision:** Deep navy #0a0e1a, frosted glass cards, electric cyan #00d4ff accent, JetBrains Mono for data, Inter for labels, animated gauges, radar grid background. Dark primary, light stretch.
**Status:** Active

## [2026-07-17] 4-Layer Architecture
**By:** Jeff 🛡️
**Context:** Old app had 3 competing chat paths and mixed concerns.
**Decision:** QML → Controllers (QObject) → Services (unit-testable) → Platform (QProcess/libgit2/QNAM). QML never sees tokens. Services live on thread pool.
**Status:** Active

## [2026-07-17] Git via libgit2, not shell
**By:** Jeff 🛡️
**Context:** Old app used `git add -A` and hard-coded nonexistent path.
**Decision:** libgit2 C API, explicit-path staging only, configurable repo path from config, ff-only pull, confirm dialog before destructive ops.
**Status:** Active
