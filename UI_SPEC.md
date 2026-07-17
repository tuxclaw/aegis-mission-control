# AEGIS — UI / UX Specification

> **AEGIS Mission Control** — native Qt 6 / Qt Quick (QML) cockpit UI.
> Theme: **"Midnight Command"**. Dark-first, GPU-rendered, 60 fps.
>
> Spec version: 1.0 · Author: Jeff 🛡️ · Date: 2026-07-17 · Companion to `SPEC.md`

---

## 1. Design Language

AEGIS should feel like a **mission-control cockpit**: you are commanding a fleet of AI agents and watching a live system. Calm dark field, glowing instruments, precise monospace numerals, subtle HUD grid, smooth motion. Information-dense but never noisy. Every glowing element means something is alive.

Principles:

1. **Instrument, don't decorate.** Glow and motion signal live state (value change, status, connection). Static chrome is muted.
2. **Truth over flourish.** If a metric is unknown, show "n/a" in a dim style — never a fake needle.
3. **Native performance.** All effects are Qt Quick scene-graph / shader based; target a steady 60 fps. Respect `ui.reduceMotion`.
4. **Legibility first.** Numbers in JetBrains Mono, labels in Inter. AA contrast on all text ≥ 4.5:1 against panel background.

---

## 2. Design Tokens

Defined once in `qml/theme/Theme.qml`, `Typography.qml`, `Motion.qml` as QML singletons. QML references `Theme.bg`, `Typography.data`, `Motion.fast`, etc. Swapping the `Theme` singleton values yields the light theme (stretch).

### 2.1 Color tokens (Midnight Command, dark)

| Token | Value | Use |
|---|---|---|
| `bg` | `#0a0e1a` | app background (deep navy) |
| `bgElevated` | `#0d1424` | secondary surfaces |
| `gridLine` | `rgba(0,212,255,0.05)` | radar/dotted HUD grid |
| `panel` | `rgba(15,25,50,0.85)` | frosted glass card fill |
| `panelBorder` | `rgba(0,212,255,0.18)` | card border |
| `panelGlow` | `rgba(0,212,255,0.12)` | card outer glow |
| `accent` | `#00d4ff` | primary electric cyan |
| `accentDim` | `#0891b2` | pressed / secondary cyan |
| `accentSoft` | `rgba(0,212,255,0.15)` | hover fills, selection |
| `warn` | `#ffa500` | amber warnings / idle |
| `alert` | `#ff3366` | red alerts / error |
| `ok` | `#22e39a` | success / healthy (green-cyan) |
| `textPrimary` | `#e6f0ff` | primary text |
| `textSecondary` | `#8fa3c8` | labels, secondary |
| `textMuted` | `#5a6c8f` | disabled, "n/a", timestamps |
| `divider` | `rgba(143,163,200,0.12)` | separators |
| `shadow` | `rgba(0,0,0,0.55)` | drop shadow base |

Status ring mapping: `active → accent (#00d4ff)`, `idle → warn (#ffa500)`, `error → alert (#ff3366)`, `unknown → textMuted`.

### 2.2 Typography

| Role | Font | Size | Weight | Tracking |
|---|---|---|---|---|
| `display` | JetBrains Mono | 32 | 700 | 0 | large gauge numerals |
| `data` | JetBrains Mono | 15 | 500 | 0 | metric values, tables |
| `dataSmall` | JetBrains Mono | 12 | 500 | 0.5 | sub-labels, units |
| `title` | Inter | 20 | 600 | 0 | view titles |
| `heading` | Inter | 15 | 600 | 0 | card headers |
| `label` | Inter | 13 | 500 | 0.2 | field labels, sidebar |
| `body` | Inter | 14 | 400 | 0 | descriptions, memory text |
| `caption` | Inter | 11 | 500 | 0.4 | status bar, hints (UPPERCASE for eyebrows) |

Fonts vendored in `resources/fonts/` (JetBrains Mono + Inter, both OFL/SIL). Loaded via `FontLoader` / bundled application fonts. Never rely on system font availability.

### 2.3 Spacing scale

`Theme.space = { xs:4, sm:8, md:12, lg:16, xl:24, xxl:32 }`. Grid gutter 16. Card padding 20. Sidebar width 224 (expanded) / 64 (collapsed).

### 2.4 Radii, borders, shadows

| Token | Value |
|---|---|
| `radiusCard` | 14 |
| `radiusControl` | 10 |
| `radiusPill` | 999 |
| `borderWidth` | 1 |
| `cardShadow` | y=8, blur=32, `shadow` |
| `glowBlur` | 24 (accent glow on hover/active) |

Frosted glass card = fill `panel` + 1px `panelBorder` + soft outer `panelGlow` + drop `cardShadow`. A `MultiEffect` (Qt 6.5+) provides blur/shadow; where backdrop blur is unavailable, fall back to the semi-opaque `panel` fill (still reads as glass over the dark grid).

---

## 3. Window Layout

Single `ApplicationWindow`, frameless-optional (native decorations by default), min size **1024×720**, default **1440×900**.

```
┌───────────────────────────────────────────────────────────────────────────┐
│  ▓▓ RadarGrid background (dotted HUD grid + faint radial sweep) ▓▓          │
│ ┌────────────┬──────────────────────────────────────────────────────────┐ │
│ │            │  ┌─ Top bar: view title + contextual actions ───────────┐ │ │
│ │  SIDEBAR   │  │  DASHBOARD                         [ ⟳ ] [ + ] [ ⚙ ]  │ │ │
│ │  (224px)   │  └──────────────────────────────────────────────────────┘ │ │
│ │            │                                                            │ │
│ │  ◉ AEGIS   │   ┌──────────── MAIN CONTENT (StackLayout view) ────────┐ │ │
│ │            │   │                                                      │ │ │
│ │  ▸ Dash    │   │   (active view renders here; cards slide+fade in)    │ │ │
│ │  ▸ Agents  │   │                                                      │ │ │
│ │  ▸ Calendar│   │                                                      │ │ │
│ │  ▸ Cron    │   │                                                      │ │ │
│ │  ▸ Memory  │   │                                                      │ │ │
│ │  ▸ Models  │   │                                                      │ │ │
│ │  ▸ Packages│   │                                                      │ │ │
│ │  ▸ Git     │   │                                                      │ │ │
│ │  ▸ Creative│   │                                                      │ │ │
│ │            │   └──────────────────────────────────────────────────────┘ │ │
│ │  ▸ Settings│                                                            │ │
│ └────────────┴──────────────────────────────────────────────────────────┘ │
│ ┌─ STATUS BAR (28px) ──────────────────────────────────────────────────┐   │
│ │ ● Connected  │  Agents: 4 active / 6  │  CPU 23%  │  Last sync 05:33:12 │  │
│ └──────────────────────────────────────────────────────────────────────┘   │
└───────────────────────────────────────────────────────────────────────────┘
```

- **RadarGrid**: full-window background layer (`qml/components/RadarGrid.qml`). Dotted grid (2px dots, 32px pitch, `gridLine`) plus a very slow (~24 s) radial sweep gradient at ~6% opacity. Purely decorative, GPU shader, pauses when `reduceMotion`.
- **Sidebar**: fixed left, 224px expanded / 64px collapsed.
- **Top bar**: 56px; left = view title (`title` role) + optional breadcrumb; right = contextual action buttons (refresh, add, view-specific).
- **Main content**: `StackLayout` swapping views; only the active view's timers run.
- **Status bar**: 28px persistent bottom strip.

---

## 4. Sidebar

```
┌────────────────────┐
│                    │
│   ◉  AEGIS         │  ← wordmark: cyan hexagon glyph + "AEGIS" (Inter 600, tracked)
│   MISSION CONTROL  │  ← caption eyebrow, textMuted, UPPERCASE
│                    │
│  ┌──────────────┐  │
│  │▸ ▦  Dashboard│  │  ← active item: accentSoft fill, 3px left cyan bar, icon+label cyan
│  └──────────────┘  │
│    ◇  Agents    ③  │  ← icon + label + optional count badge (active agents)
│    ▤  Calendar     │
│    ⏱  Cron         │
│    ▧  Memory       │
│    ⬢  Models       │
│    ▣  Packages     │
│    ⑂  Git       ●  │  ← dot badge if dirty working tree
│    ✦  Creative     │
│                    │
│   ── divider ──    │
│    ⚙  Settings     │
│                    │
└────────────────────┘
```

- **Item** (`SidebarItem.qml`): 44px tall, icon (20px SVG) + label (`label` role). States:
  - *default*: `textSecondary` icon+label.
  - *hover*: label→`textPrimary`, icon→`accent`, subtle `accentSoft` fill fades in (120 ms), faint glow on icon.
  - *active*: `accentSoft` fill, 3px `accent` left indicator bar (animated height on selection), icon+label `accent`.
  - *pressed*: scale 0.98, `accentDim`.
- **Badges**: Agents shows active-agent count pill (cyan); Git shows a dirty dot (`warn`) when working tree not clean. Driven by live controller state.
- **Collapse**: below 1180px window width, or via a collapse toggle, sidebar collapses to 64px (icons only, labels appear as tooltips on hover). Transition 200 ms width animation.
- **Wordmark**: cyan hexagon "aegis" mark (SVG) + wordmark; clicking returns to Dashboard.

---

## 5. Status Bar

`StatusBar.qml`, 28px, `bgElevated` fill, top 1px `divider`. Segments (left→right), separated by faint vertical dividers:

```
 ● Connected   │   Agents: 4 active / 6   │   CPU 23%  MEM 41%   │   ⟳ Last sync 05:33:12
```

- **Connection**: dot + label driven by `app.connectionState` (`GatewayService`). `Connected`=`ok` dot, `Connecting`=pulsing `warn`, `Disconnected`=`alert`, `No token`=`alert` + "Credential not set". **Never hard-coded** — reflects real gateway state.
- **Agents**: `N active / M total` from live `AgentListModel`.
- **Vitals mini**: CPU% and MEM% compact readouts from `VitalsController`.
- **Last sync**: timestamp of the most recent successful backend refresh, `data`/`caption` mono; a tiny refresh glyph pulses on each sync.
- Segments truncate gracefully at min width (drop vitals mini first, then agents label text keeping the count).

---

## 6. View Wireframes (ASCII)

All views render inside the main content area. All lists use QML `ListView`/`TableView` (delegate recycling = virtualized). All cards are `GlassCard`. Cards enter with slide-up (12px) + fade (200 ms, staggered 40 ms).

### 6.1 Dashboard

```
 DASHBOARD                                                   [ ⟳ Refresh ]
 ┌──────────────────┐ ┌──────────────────┐ ┌──────────────────┐ ┌────────────┐
 │  CPU             │ │  GPU             │ │  MEMORY          │ │  NETWORK   │
 │     ╭─────╮      │ │     ╭─────╮      │ │     ╭─────╮      │ │  ↑ 1.2 MB/s│
 │    ╱  23%  ╲     │ │    ╱ 47%  ╲     │ │    ╱ 41%  ╲     │ │  ↓ 4.8 MB/s│
 │   │  ●●●●   │    │ │   │ ●●●●●  │    │ │   │ ●●●●   │    │ │            │
 │    ╲ 32c   ╱     │ │    ╲nvidia╱     │ │    ╲39/96G╱     │ │  eth0      │
 │     ╰─────╯      │ │     ╰─────╯      │ │     ╰─────╯      │ │  ▁▂▅▇▅▂▁   │
 │  load 1.84       │ │  62°C  8/24GB    │ │  swap 0.2/8G     │ │  sparkline │
 └──────────────────┘ └──────────────────┘ └──────────────────┘ └────────────┘
 ┌──────────────────────────────────────────┐ ┌───────────────────────────────┐
 │  DISKS                                     │ │  FLEET AT A GLANCE            │
 │  /            ▓▓▓▓▓▓▓▓░░░░  612 / 2000 GB   │ │  ◉ Andy    active   opus-4    │
 │  /home        ▓▓▓▓░░░░░░░░  180 / 1000 GB   │ │  ◉ Jeff    active   opus-4    │
 │  Andys_Drive  ▓▓░░░░░░░░░░  240 / 2000 GB   │ │  ○ Dash    idle     sonnet    │
 │                                            │ │  ○ Helen   idle     sonnet    │
 │                                            │ │  ✕ Rick    error    —         │
 └──────────────────────────────────────────┘ └───────────────────────────────┘
```

- Four `VitalGauge` cards top row (CPU, GPU, MEM, NET). GPU shows "n/a" dimmed if `utilizationPct` is NaN. Each gauge **pulses** on significant value change.
- Disks card: horizontal usage bars (`ok`→`warn`→`alert` by fill %).
- "Fleet at a glance": compact roster summary (status ring + name + status + model), links to Agents view.

### 6.2 Agent Roster

```
 AGENTS                                                      [ ⟳ ]  filter: [ all ▾ ]
 ┌──────────────────────┐ ┌──────────────────────┐ ┌──────────────────────┐
 │  ╭──╮                │ │  ╭──╮                │ │  ╭──╮                │
 │  │AN│ ◉ active       │ │  │JE│ ◉ active       │ │  │DA│ ○ idle         │
 │  ╰──╯                │ │  ╰──╯                │ │  ╰──╯                │
 │  Andy                │ │  Jeff                │ │  Dash                │
 │  model: opus-4       │ │  model: opus-4       │ │  model: sonnet-4     │
 │  sessions: 2         │ │  sessions: 1         │ │  sessions: 0         │
 │  last seen: 05:32    │ │  last seen: 05:33    │ │  last seen: 04:58    │
 │  [ details ]         │ │  [ details ]         │ │  [ details ]         │
 └──────────────────────┘ └──────────────────────┘ └──────────────────────┘
 ┌──────────────────────┐ ┌──────────────────────┐
 │  │HE│ ○ idle         │ │  │RI│ ✕ error        │
 │  Helen               │ │  Rick                │
 │  ...                 │ │  detail: cli timeout │
 └──────────────────────┘ └──────────────────────┘
```

- Grid of agent cards. Avatar = colored disc with initials (color derived from `avatarSeed`), wrapped in a **StatusRing** (animated). Ring color = status mapping.
- Fully populated from `AgentListModel` (live CLI). No hard-coded roster.
- Filter dropdown (all / active / idle / error). Error cards surface `statusDetail` (safe string).
- `details` opens an inline expand or side detail (sessions, model, last seen). Selection is functional (drives detail), not cosmetic.

### 6.3 Calendar

```
 CALENDAR                                        [ ◀ ]  July 2026  [ ▶ ]   [ + Event ]
 ┌─────┬─────┬─────┬─────┬─────┬─────┬─────┐
 │ Mon │ Tue │ Wed │ Thu │ Fri │ Sat │ Sun │
 ├─────┼─────┼─────┼─────┼─────┼─────┼─────┤
 │  29 │  30 │   1 │   2 │   3 │   4 │   5 │
 │     │     │ ▉std│     │     │     │     │
 ├─────┼─────┼─────┼─────┼─────┼─────┼─────┤
 │  6  │  7  │  8  │  9  │ 10  │ 11  │ 12  │
 │     │▉mtg │     │▉1:1 │     │     │     │
 ├─────┼─────┼─────┼─────┼─────┼─────┼─────┤
 │ 13  │ 14  │ 15  │ 16  │[17] │ 18  │ 19  │  ← today ring on 17
 │     │     │     │     │▉rev │     │     │
 └─────┴─────┴─────┴─────┴─────┴─────┴─────┘
   ┌── EVENT EDITOR (drawer, slides from right) ──────────┐
   │  Title    [ Design review................. ]         │
   │  Start    [ 2026-07-17 ] [ 14:00 ]  ☐ all day        │
   │  End      [ 2026-07-17 ] [ 15:00 ]                    │
   │  Location [ ......................... ]               │
   │  Color    ● ● ● ● ●   (palette tokens)               │
   │  Notes    [ ................................. ]       │
   │           [ Cancel ]           [ Save Event ]         │
   └──────────────────────────────────────────────────────┘
```

- Month grid; today marked with a cyan ring. Event chips colored by validated palette token.
- `+ Event` / clicking a day opens the **editor drawer** (right slide-in, 260 ms). All fields validated client-side mirroring `CalendarEventDto` rules (title 1..200, end ≥ start, etc.); Save awaits the store result and shows inline error on failure (no optimistic silent write). Delete asks confirm.
- Loading state: skeleton grid. Error (parse failure) state: explicit banner "Calendar data could not be read — not overwritten" with Retry (never silently empties).

### 6.4 Cron Manager

```
 CRON                                                        [ ⟳ ]
 ┌────────────────────────────────────────────────────────────────────────┐
 │ NAME              SCHEDULE        STATE     LAST RUN     NEXT       ACT   │
 ├────────────────────────────────────────────────────────────────────────┤
 │ Morning distill   0 6 * * *       ◉ on      05:00 ✓      Tmrw 06:00 [▷][⏻]│
 │ Inbox triage      */30 * * * *    ◉ on      05:30 ✓      06:00      [▷][⏻]│
 │ Weekly backup     0 3 * * 0       ○ off     Jul 13 ✓     —          [▷][⏻]│
 │ Cert check        0 9 * * 1       ◉ on      Jul 13 ✕     Mon 09:00  [▷][⏻]│
 └────────────────────────────────────────────────────────────────────────┘
   [▷] run now (async, shows spinner + toast on completion)
   [⏻] toggle enable/disable (optimistic-off; awaits + reverts on failure)
```

- `TableView` bound to `CronJobModel`. State pill: `on`=`ok`, `off`=`textMuted`. Last-run result glyph: ✓ `ok` / ✕ `alert`.
- `Run now` disables its button + shows spinner until the async call returns; toast on success/fail. `Toggle` awaits result; reverts visual on error.
- No blocking: everything async with timeouts.

### 6.5 Memory Viewer

```
 MEMORY                                                      [ root: workspace ▾ ]
 ┌──────────────────────┬─────────────────────────────────────────────────┐
 │ FILES                │  2026-07-17.md                        1.2 KB      │
 │ ┌──────────────────┐ │ ┌─────────────────────────────────────────────┐ │
 │ │▸ 2026-07-17.md   │ │ │ # 2026-07-17 (Friday)                       │ │
 │ │  2026-07-16.md   │ │ │                                             │ │
 │ │  2026-07-15.md   │ │ │ ## AEGIS rebuild                            │ │
 │ │  MEMORY.md       │ │ │ - Tore out Tauri app                        │ │
 │ │  resume.md       │ │ │ - Writing Qt6 spec                          │ │
 │ │  ...             │ │ │   (rendered as inert monospace / md-lite)   │ │
 │ └──────────────────┘ │ │                                             │ │
 │  12 files            │ └─────────────────────────────────────────────┘ │
 └──────────────────────┴─────────────────────────────────────────────────┘
```

- Left: file list from `MemoryFileModel` (allowlisted root selector at top). Right: read-only content pane.
- Content rendered as **inert text** — QML `Text` with a lightweight, safe markdown-lite formatter (headers/bold/lists → font styling only). **No HTML, no links executed, no images fetched.** Long files virtualized/scrollable.
- Root selector limited to configured `memory.roots`. Size cap enforced; oversize file shows "file too large to preview".
- States: empty ("no memory files"), loading skeleton, error (path rejected → "access denied").

### 6.6 Model Switcher

```
 MODELS                                                      [ ⟳ ]
 ┌────────────────────────────────────────────────────────────────────────┐
 │  Active model:  anthropic/claude-opus-4-8                                │
 └────────────────────────────────────────────────────────────────────────┘
 ┌──────────────────────────────┐ ┌──────────────────────────────┐
 │ ⬢ claude-opus-4-8   [ACTIVE] │ │ ⬢ claude-sonnet-4            │
 │   anthropic                  │ │   anthropic                  │
 │                              │ │            [ Set active ]    │
 └──────────────────────────────┘ └──────────────────────────────┘
 ┌──────────────────────────────┐ ┌──────────────────────────────┐
 │ ⬢ gpt-image-2               │ │ ⬢ llama3.1:70b               │
 │   openai      [ Set active ] │ │   ollama       [ Set active ]│
 └──────────────────────────────┘ └──────────────────────────────┘
```

- List/grid from **live** `ModelListModel`. Active card highlighted (cyan border glow + `ACTIVE` pill).
- `Set active` validates against the live list, calls `ModelService::setActive`, awaits, updates active on success, toast on failure. No hard-coded model IDs anywhere.

### 6.7 Packages

```
 PACKAGES                                    [ ⟳ ]   search: [ 🔍 ............ ]
 ┌────────────────────────────────────────────────────────────────────────┐
 │ NAME                    VERSION            SOURCE                         │
 ├────────────────────────────────────────────────────────────────────────┤
 │ qt6-qtbase              6.6.2-3.fc44       rpm                            │
 │ libgit2                 1.7.2-1.fc44       rpm                            │
 │ nodejs                  22.22.0            rpm                            │
 │ openclaw                2026.6.12          npm-global                     │
 │ ...                                        (virtualized, 2000+ rows)      │
 └────────────────────────────────────────────────────────────────────────┘
   1,842 packages · read-only inventory
```

- `TableView` bound to `PackageListModel`, virtualized. Client-side filter box (name contains). Read-only; no install/remove affordances.
- Count summary in footer.

### 6.8 Git Panel

```
 GIT                                    branch: andy/aegis-spec  ↑2 ↓0   [ ⟳ ]
 ┌─────────────────────────────┬──────────────────────────────────────────┐
 │ CHANGES                     │  DETAIL / COMMIT                          │
 │ ── Staged ───────────────── │  ┌────────────────────────────────────┐  │
 │ [✓] M src/main.cpp          │  │ selected: src/main.cpp             │  │
 │ [✓] A qml/views/GitView.qml │  │  (diff summary / stat, read-only)  │  │
 │ ── Unstaged ─────────────── │  │  +42 −7                            │  │
 │ [ ] M SPEC.md          [+]  │  └────────────────────────────────────┘  │
 │ [ ] ? scratch.txt      [+]  │                                          │
 │                             │  Commit message:                         │
 │  [ stage selected ]         │  [ Add Git view and main bootstrap.... ] │
 │  [ unstage selected ]       │                                          │
 │                             │  [ Commit ]  [ Pull ]  [ Push ]          │
 │  repo: ~/.../ao-mission-... │   (each → ConfirmDialog before running)  │
 └─────────────────────────────┴──────────────────────────────────────────┘
```

- Left: `GitFileModel` split into Staged / Unstaged / Untracked groups, each row a checkbox + state glyph + path. **Staging is explicit per-path** (`[+]` per row or check + "stage selected"). There is **no "stage all"** button that bypasses review.
- Right: selected file stat (read-only), commit message field, and destructive/remote actions.
- **Commit / Pull / Push** each raise a `ConfirmDialog` (focus defaults to Cancel) summarizing exactly what will happen (which paths, which remote/branch). Only on confirm does the controller proceed.
- Unconfigured state: if `git.repoPath` unset/invalid → full-panel `EmptyState`: "No repository configured" + button to Settings. Never a broken hard-coded path.
- Pull is ff-only; on non-ff → `ErrorState` "Pull would not fast-forward — resolve manually" (no silent merge).
- Credentials are handled in C++ (`SecretStore`); UI never shows or requests them inline beyond a Settings flow.

### 6.9 Creative

```
 CREATIVE                                                    backend: [ Ollama ▾ ]
 ┌──────────────────────────────┐ ┌────────────────────────────────────────┐
 │ CONFIG                       │  OUTPUT                                   │
 │ Model   [ llama3.1:70b ▾ ]   │ ┌──────────────────────────────────────┐ │
 │ Temp    [====●====] 0.7      │ │ (streamed tokens appear here, inert   │ │
 │ Max tok [ 1024 ]             │ │  text, monospace, auto-scroll)        │ │
 │                              │ │                                       │ │
 │ Prompt                       │ │  ▍ generating...                      │ │
 │ [ ........................ ] │ │                                       │ │
 │ [ ........................ ] │ │                                       │ │
 │ [ ........................ ] │ └──────────────────────────────────────┘ │
 │                              │  finish: —      out: 512 B                │
 │ [ Generate ]  [ Cancel ]     │  [ Copy ]  [ Clear ]                      │
 └──────────────────────────────┘ └────────────────────────────────────────┘
```

- Left config: backend (Ollama / Gateway), model (from live list), temperature slider (0..2 clamp), max tokens (1..8192 clamp), prompt (1..8000 validated).
- `Generate` streams into the output pane (inert text). `Cancel` aborts the in-flight request (`finishReason=cancelled`). Buttons reflect busy state; only one active request at a time.
- Output rendered as inert monospace text — never HTML/markdown-to-HTML. Final commit is one snapshot on `finished` (no stale last token).
- Footer shows finish reason + bounded output byte count.

### 6.10 Settings

```
 SETTINGS
 ┌────────────────────────────────────────────────────────────────────────┐
 │  GATEWAY                                                                 │
 │   Base URL       [ http://localhost:PORT .................. ]           │
 │   Gateway token  [ ●●●●●●●●●●●● (set) ]   [ Update token ]  ← masked     │
 │   Status         ● Connected                                            │
 ├────────────────────────────────────────────────────────────────────────┤
 │  GIT                                                                     │
 │   Repo path      [ /home/tux/.openclaw/workspace ....... ] [ Browse ]   │
 │   Remote         [ origin ]     Pull mode: ff-only                       │
 │   Credential     [ ●●●● (SSH key set) ]  [ Update credential ]          │
 ├────────────────────────────────────────────────────────────────────────┤
 │  MEMORY ROOTS    workspace → ~/.openclaw/workspace/memory   [ + add ]    │
 │  DATA ROOT       [ ~/.local/share/Aegis ]                                │
 │  OLLAMA URL      [ http://localhost:11434 ]                              │
 │  VITALS INTERVAL [ 1000 ] ms                                            │
 │  APPEARANCE      Theme [ Dark ▾ ]   ☐ Reduce motion                     │
 │  LOGGING         Level [ Info ▾ ]                                        │
 │                                              [ Save ]                    │
 └────────────────────────────────────────────────────────────────────────┘
```

- Secret fields are **masked**, show only "(set)"/"(not set)". Entering a value submits to `SecretStore` via C++; the value is never read back into QML. "Update token"/"Update credential" open a masked single-field dialog.
- URL/path fields validated (TLS policy for non-loopback gateway URL, git repo must be a worktree — validated on Save with inline error).
- Theme selector (Dark primary; Light stretch). Reduce-motion toggle disables all non-essential animation globally.

---

## 7. Component Library

Each component is a QML file in `qml/components/`. Props are QML properties; behavior noted.

### GlassCard.qml
- **Props**: `title` (string, optional header), `padding` (int=20), `interactive` (bool), `contentItem` (default slot).
- **Behavior**: frosted panel (fill `panel`, 1px `panelBorder`, outer `panelGlow`, `cardShadow`, `radiusCard`). If `interactive`, hover raises glow (`glowBlur`) + border→`accent` over 160 ms. Enter animation: slide-up 12px + fade, 200 ms `Motion.standard`.

### VitalGauge.qml
- **Props**: `label` (string), `value` (real 0..100 or NaN), `unit` (string), `subtext` (string), `accentColor` (color=`accent`), `available` (bool).
- **Behavior**: circular arc gauge (Canvas or `Shape`/`PathAngleArc`), 270° sweep. Numeral in center (`display` role). Arc fill animates to new value over 400 ms `Motion.gauge` (easeOutCubic). On value delta > threshold, a one-shot **pulse** (scale 1.0→1.04→1.0 + glow flash, 300 ms). If `!available` or `isNaN(value)`, render dim "n/a" and static muted arc — never a fabricated value. Color shifts `ok`→`warn`→`alert` by thresholds (configurable per gauge).

### StatusRing.qml
- **Props**: `status` (enum AgentStatus), `size` (int), `pulse` (bool).
- **Behavior**: ring around an avatar/disc. Color per status mapping. `active` ring has a slow rotating dashed highlight + gentle breathing glow (2 s loop). `error` ring flashes `alert` twice on status entry. Static when `reduceMotion`.

### SidebarItem.qml
- **Props**: `icon` (url), `label` (string), `active` (bool), `badgeText` (string), `badgeColor` (color).
- **Behavior**: see §4. Hover/active/pressed transitions 120–160 ms.

### PrimaryButton.qml / SecondaryButton / GhostButton
- **Props**: `text`, `enabled`, `busy`, `destructive` (bool).
- **Behavior**: Primary = cyan fill on dark, glow on hover, scale 0.98 press. `busy` swaps label for inline spinner and disables. `destructive` uses `alert` accents and always pairs with a ConfirmDialog upstream.

### StatusBar.qml
- **Props**: bound to `app` controller state. Segments in §5. Refresh glyph pulses on sync.

### ConfirmDialog.qml
- **Props**: `action` (string), `detail` (string), `destructive` (bool).
- **Behavior**: modal, dim backdrop, glass card. **Focus defaults to Cancel** (fixes old bug where confirm was focused). Buttons: Cancel (safe, focused) + Confirm (`alert` if destructive). Returns accepted/rejected signal. Accessible labels (`aria`-equivalent via `Accessible.name`).

### ToastHost.qml
- **Props**: none (singleton host, bottom-right stack).
- **Behavior**: transient toasts (info/success/warn/error color-coded), slide-in from right + fade, auto-dismiss 4 s (errors 8 s, dismissible). Fed by controller `toast(msg, level)` and `errorRaised`.

### EmptyState.qml / LoadingState.qml / ErrorState.qml
- **EmptyState**: centered muted icon + title + optional action button (e.g. Git "configure repo").
- **LoadingState**: skeleton shimmer placeholders matching the view's card layout (shimmer sweep 1.2 s) OR a centered pulsing radar spinner for small areas.
- **ErrorState**: `alert`-tinted card, safe `userMessage`, a **Retry** button shown only when `retryable`. Never shows raw stderr/paths.

### RadarGrid.qml
- Background dotted HUD grid + slow radial sweep shader. `reduceMotion` freezes the sweep.

### Skeleton.qml / Sparkline.qml / UsageBar.qml
- **Sparkline**: tiny line chart for network throughput history (ring buffer of samples).
- **UsageBar**: horizontal fill bar, color by threshold (disks, memory).

---

## 8. Animation Catalog

Central durations/easings in `qml/theme/Motion.qml`. All honor `settings.reduceMotion` (which collapses durations to 0 / disables loops).

| Name | Duration | Easing | Used by |
|---|---|---|---|
| `Motion.instant` | 0 ms | — | reduce-motion fallback |
| `Motion.fast` | 120 ms | `OutQuad` | hover fills, sidebar highlight |
| `Motion.standard` | 200 ms | `OutCubic` | card enter (slide+fade), view transitions |
| `Motion.gauge` | 400 ms | `OutCubic` | gauge arc value change |
| `Motion.drawer` | 260 ms | `OutCubic` | calendar editor / detail drawers |
| `Motion.pulse` | 300 ms | `InOutQuad` | gauge value-change pulse |
| `Motion.sidebarCollapse` | 200 ms | `OutCubic` | sidebar width |
| `Motion.toast` | 220 ms | `OutBack` (subtle) | toast slide-in |
| `Motion.sweep` | 24 s loop | `Linear` | radar background sweep |
| `Motion.breathe` | 2 s loop | `InOutSine` | active status ring glow |
| `Motion.shimmer` | 1.2 s loop | `Linear` | skeleton loading |

Rules:
- **View switch**: outgoing view fades out 120 ms; incoming view cards slide-up + fade in with 40 ms per-card stagger (max ~6 staggered, rest instant).
- **Value pulse**: only fires when delta exceeds a per-metric threshold (avoid constant twitching).
- **60 fps budget**: no more than a handful of concurrent looping animations visible; background sweep + active rings are the only always-on loops. Everything GPU-composited (opacity/scale/transform, `layer.enabled` where blur needed).

---

## 9. Responsive Behavior

- **Min window**: 1024×720. Below the OS min, content clamps (no mobile layout — desktop only).
- **Sidebar collapse**: auto-collapse to 64px icon rail when width < 1180px; manual toggle always available. Tooltips replace labels when collapsed.
- **Card grids**: Dashboard/Agents/Models use a responsive `Flow`/`GridLayout` — column count derives from available width (`Math.max(1, floor(width / minCardWidth))`, minCardWidth ≈ 240). Cards reflow with a 200 ms layout animation.
- **Split views** (Memory, Git, Creative): left pane has a min width; below a threshold the panes stack or the left pane becomes collapsible.
- **Tables** (Cron, Packages): horizontal scroll only as last resort; prefer column priority hiding (drop least-important columns first — e.g. Packages drops "summary/source" before "version").
- **Status bar**: segment dropping order on narrowing = vitals mini → agent label text → keep connection dot always.

---

## 10. States: Empty / Loading / Error

Every view implements all three (mapped from controller state):

- **Loading**: skeleton shimmer matching the target layout on first load; a subtle top progress hairline (cyan) on background refresh so content doesn't jump.
- **Empty**: `EmptyState` with context copy + action where relevant (Git "configure repo", Memory "no files", Calendar "no events this month", Creative "enter a prompt").
- **Error**: `ErrorState` with safe `userMessage` + Retry when `retryable`. Special cases:
  - Gateway no-token → "Gateway credential not set" + link to Settings (fail-closed, not a crash).
  - Calendar parse error → "Data could not be read and was NOT overwritten" (data-loss-safe).
  - Git non-ff pull → manual-resolution message.
  - GPU unavailable → gauge "n/a", not an error.

Never surface raw stderr, tokens, or absolute paths in any state.

---

## 11. Theming (Dark primary, Light stretch)

- All color usage flows through `Theme` singleton tokens. A `Theme.mode` property (`"dark"`/`"light"`) selects a token set. Dark ("Midnight Command") is the shipped default and the fully-designed theme.
- **Light theme (stretch)** token direction (not final): `bg #f4f7fc`, `panel rgba(255,255,255,0.85)`, `panelBorder rgba(8,145,178,0.25)`, `accent #0891b2`, text inverted, warm/alert hues unchanged for consistency. Radar grid opacity reduced. Glass becomes light frosted. Only ship when contrast + glow legibility are verified across all views.
- Fonts and layout are theme-independent; only color tokens and a few glow intensities change.

---

## 12. Accessibility & Input

- All interactive elements set `Accessible.name`/`role`. Focus ring is a 2px `accent` outline (visible on keyboard nav).
- Full keyboard navigation: sidebar (↑/↓ + Enter), tab traversal within views, `Esc` closes dialogs/drawers (defaults to Cancel).
- `reduceMotion` disables loops and collapses transitions to instant.
- Minimum text contrast 4.5:1; status is never conveyed by color alone (glyph + label accompany every status ring / state pill).

---

*End of UI_SPEC.md*
