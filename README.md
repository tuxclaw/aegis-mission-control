# AEGIS Mission Control

A native Qt 6 / QML / C++20 desktop dashboard for monitoring system resources, managing AI agent fleets, and tracking provider usage — all with a Liquid Glass frosted-glass UI.

![License](https://img.shields.io/badge/license-MIT-blue)
![Qt](https://img.shields.io/badge/Qt-6.5+-green)
![C++](https://img.shields.io/badge/C++-20-blue)

## Features

### Dashboard
- **System Vitals** — CPU, GPU, memory, and network gauges with real-time sampling from `/proc`
- **Disk Usage** — Mount points with usage bars and color-coded thresholds
- **Containers** — Auto-detected running containers via Podman or Docker
- **Processes** — Top processes by CPU usage with memory breakdown
- **Fleet at a Glance** — Active/idle agent roster with model information

### Agent Management
- **Agent Roster** — Full agent list with status, model, and session info
- **Auto-generated** from `openclaw agents list --json` at startup
- **Calendar** — Event management with create, edit, and delete
- **Cron Jobs** — Scheduled task management
- **Memory** — Workspace memory file browser
- **Models** — AI model configuration and switching
- **Packages** — Package management
- **Git** — Repository status and operations

### Usage Tracking
- **Provider Quotas** — Real-time usage data from 5 AI providers:
  - **OpenAI** — ChatGPT Pro plan usage via Codex OAuth
  - **Anthropic** — Claude usage via OAuth credentials
  - **xAI (Grok)** — Local session activity from `~/.grok/sessions/`
  - **Gemini** — API key verification via Gemini CLI
  - **Xiaomi (MiMo)** — Token usage from platform API
- **Per-provider cards** — Usage bars, balance, plan info, and error states
- **Configurable** — Enable/disable providers in `~/.config/openclaw-monitor/config.ini`

### System Tray
- **KDE Plasma integration** — System tray icon with popup card
- **D-Bus SNI** — Native Plasma activation support
- **Single-instance** — Prevents duplicate instances via QLocalServer

## Architecture

```
aegis-mission-control/
├── CMakeLists.txt                 # Build configuration
├── src/
│   ├── main.cpp                   # App entry, QML engine, context properties
│   ├── app/
│   │   ├── app_context.cpp/h      # Dependency injection container
│   │   └── qml_registration.cpp/h # QML type registration
│   ├── core/                      # Shared utilities (HTTP, path, error, logging)
│   ├── config/                    # ConfigService, SecretStore
│   ├── controllers/               # QML-facing controllers (Q_OBJECT, Q_PROPERTY)
│   ├── services/                  # Business logic, system access, timers
│   │   ├── providers/             # Individual provider quota fetchers
│   │   ├── system_stats.h/cpp     # Fable 5 system sampler
│   │   ├── container_list.h/cpp   # Fable 5 container detection
│   │   └── agent_roster.h/cpp     # Fable 5 agent polling
│   ├── dto/                       # Data transfer objects (fromJson/toJson)
│   └── models/                    # QAbstractListModel subclasses
├── qml/
│   ├── Main.qml                   # Application window, scene + UI layer
│   ├── LiquidGlass/               # Liquid Glass UI kit components
│   ├── theme/                     # Theme, Typography, Motion tokens
│   ├── components/                # Reusable UI components
│   └── views/                     # Feature views
├── resources/
│   ├── aegis.qrc                  # Qt resource file
│   ├── icons/                     # SVG icons
│   └── fonts/                     # Inter + JetBrains Mono
├── tests/
│   └── unit/                      # QtTest unit tests
└── scripts/                       # Helper scripts
```

## Building

### Prerequisites

- Qt 6.5+ (Core, Concurrent, Network, Sql, DBus)
- CMake 3.28+
- C++20 compiler
- libgit2 1.7+ (optional, for Git integration)
- QtKeychain (optional, for secure credential storage)

### Build

```bash
# Configure
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build -j$(nproc)

# Test
cd build && ctest --output-on-failure

# Install
cp build/src/aegis ~/.local/bin/aegis
```

### Run

```bash
~/.local/bin/aegis
```

## Configuration

### Provider Usage

Create or edit `~/.config/openclaw-monitor/config.ini`:

```ini
[General]
refreshIntervalSecs=30

[providers]
openai/enabled=true
anthropic/enabled=true
xai/enabled=true
xai/cliPath=/home/user/.grok/bin/grok
gemini/enabled=true
gemini/apiKey=YOUR_API_KEY
xiaomi/enabled=true
xiaomi/serviceToken=YOUR_SERVICE_TOKEN
xiaomi/userId=YOUR_USER_ID
```

### Environment Variables

| Variable | Description |
|----------|-------------|
| `AGENTS_SOURCE` | Custom agents JSON file path (default: `~/.openclaw/agents.json`) |
| `GEMINI_API_KEY` | Google Gemini API key |
| `OPENCLAW_MONITOR_GATEWAY` | Custom gateway URL |
| `OPENCLAW_MONITOR_TOKEN` | Custom gateway auth token |

## UI Kit

AEGIS uses the **Liquid Glass** UI kit for a frosted-glass aesthetic:

- **GlassSurface** — Core backdrop-blur material with tint, sheen, and borders
- **GlassCard** — Padded content container with optional title
- **GlassButton** — Pill-shaped button with accent variant
- **GlassGauge** — Circular progress indicator
- **GlassGraph** — Sparkline/area chart
- **GlassList** — Scrollable list container
- **GlassTextField** — Input with accent focus ring
- **GlassToggle** — Frosted switch
- **GlassSlider** — Frosted slider

### Architecture Rule

Backdrop blur requires glass components to be **siblings** of the backdrop content, not descendants:

```qml
Window {
    Item { id: scene; ... }      // background content
    Item { id: uiLayer; ... }    // UI on top, referencing scene
}
```

## Testing

```bash
cd build && ctest --output-on-failure
```

**Test suites:**
- PathGuard — File path containment
- AtomicFile — Safe file writes
- AegisError — Error handling
- HttpClient — Network requests
- CalendarStore — Event persistence
- MemoryService — Memory file operations
- OpenClawCli — CLI parsing
- SecurityRegression — Security checks
- CreativeStreaming — Creative backend
- VitalsService — System sampling
- ContainerService — Container detection
- ProcessService — Process enumeration

## Development

### Agent Team

AEGIS is developed by a squad of specialized AI agents:

- **Jack 👶🔥** — Orchestrator (MiMo v2.5 Pro)
- **Dash ⚡** — Frontend builder (GPT-5.6 Sol)
- **Helen 🦸‍♀️** — Backend builder (GPT-5.6 Sol)
- **Elon ⚡** — Code reviewer (Grok 4.5)
- **Frozone 🧊** — Deep reviewer (GPT-5.6 Sol)
- **Jeff 🛡️** — Security reviewer (Claude Opus)

### Dispatch Pattern

```bash
# Frontend work
sessions_spawn agentId=dash model=openai/gpt-5.6-sol task="..."

# Backend work
sessions_spawn agentId=helen model=openai/gpt-5.6-sol task="..."

# Code review
sessions_spawn agentId=elon model=grok-4.5 runtime=acp task="..."
```

## License

MIT

## Credits

- **Liquid Glass UI kit** — Fable 5 (GLM)
- **System stats** — Fable 5 SystemStats
- **Container detection** — Fable 5 ContainerList
- **Agent roster** — Fable 5 AgentRoster
- **Provider quotas** — Ported from [openclaw-monitor](https://github.com/tuxclaw/openclaw-monitor)
- **CodexBar** — Reference for OpenAI usage API
