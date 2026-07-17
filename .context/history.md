# History

## [2026-07-17] Services, Controllers, and List Models

**By:** Helen 🦸‍♀️
**Branch:** `andy/services-controllers`

- Added all ten service-layer classes from SPEC §5, including fail-closed gateway authorization, bounded argument-array process execution, Linux vitals sampling, atomic calendar persistence, sandboxed memory reads, live model/cron/package adapters, conditional libgit2 operations, and cancellable creative request correlation.
- Added all eleven QObject controllers and seven QAbstractListModel implementations with QML-facing properties, roles, intents, feedback signals, and two-phase confirmation for calendar deletion and Git commit/pull/push.
- Wired every service and controller through AppContext and exposed controllers/enums through QmlRegistration.
- Added calendar-store, memory-service, and OpenClaw CLI parser regression tests.
- Verification: warnings-as-errors `aegis_core` build passed; all 7 QtTest executables passed.
- Host limitation: Qt Quick, libgit2, and QtKeychain development packages were unavailable in the active environment. CMake therefore built core/tests and the affected production integrations retain explicit fail-closed paths.
