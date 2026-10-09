# SuprAI

Linux-first native desktop AI agent and workspace.

SuprAI is a complete Linux-native AI application: its own Qt Quick/QML interface, C++ application core, and native agent runtime. Hermes Agent, OpenClaw, Cline and similar projects are research references only.

**Status: functional modular prototype / M1 in progress.**

The current vertical slice compiles and is CI-verified with Qt 6.12. It can run a deterministic MockRuntime or stream a real OpenAI-compatible Chat Completions endpoint through NativeSuprAIRuntime.

## Current prototype

Implemented:
- native C++20 + Qt Quick/QML application;
- three-pane desktop shell;
- composer, streaming transcript, stop/cancel, new conversation and error state;
- dedicated runtime QThread so provider work does not run on the UI thread;
- dedicated persistence QThread with a thread-affine primary SQLite writer connection;
- XDG config/data/cache/state paths resolved through QStandardPaths and created at startup;
- structured Qt logging categories with stable suprai.* names;
- freedesktop single-instance/activation service over QtDBus with headless degradation;
- explicit Chat / Configuración routes in QML;
- AgentRuntime public contract;
- deterministic MockRuntime;
- provider port separated from concrete adapters;
- tri-state provider capability contract: Unknown / Supported / Unsupported;
- OpenAI-compatible Chat Completions streaming adapter with explicit effective capability reporting;
- application composition root that injects the selected provider/runtime;
- NativeSuprAIRuntime implemented as a thin production facade;
- RuntimeOrchestrator owns stateful conversation/runtime coordination;
- AgentEngine owns the low-level provider execution kernel;
- typed AgentEngineEvent -> RuntimeEvent adapter boundary;
- raw provider `reasoning_content` treated as ephemeral and excluded from later reconstructed prompts;
- explicit public/private module include boundaries enforced by CMake targets;
- CI build, 9/9 unit/integration tests and QML startup smoke test;
- CMake-owned portable staging of Qt libraries, QML imports and plugins;
- independent staged smoke tests through XCB/Xvfb and Wayland/Weston with Qt development environment variables removed;
- bundled X11 and Wayland QPA plugins verified in the stage;
- SQLite bootstrap with WAL, foreign keys, busy timeout, schema migrations, quick/integrity checks and FTS5 capability proof;
- schema v1 for sessions, inputs, turns, runs and generalized conversation items;
- typed PersistencePort command/ACK boundary between runtime and SQLite worker;
- native turn-start transaction persists Session/Input/Turn/Run/user item before provider inference begins;
- runtime waits for durable ACK before calling AgentEngine/provider;
- SQLite QSQLITE runtime/plugin included in the portable stage;
- clean worker shutdown enforced by CI;
- newest session restored from SQLite, prepared Runs reconciled as interrupted, and assistant terminal outcome committed before completion ACK;
- initial AppImage preview built from the staged Qt runtime, with bundled Qt/QML/XCB/Wayland/SQLite, SHA-256 file, and packaged XCB/Wayland CI smoke tests.

Not implemented yet:
- older-session selection and advanced ProviderAttempt/Task recovery;
- Responses transport;
- tool calling and approvals;
- MCP;
- memory;
- subagents/deliberation;
- projects/files;
- safe Markdown renderer;
- editable persisted provider settings and secure SecretStore;
- release-grade AppImage ABI-floor/cross-distro proof (initial alpha AppImage preview is available through the CI/release pipeline);
- physical KDE/GNOME Wayland and X11 smoke tests.

## Module shape

SuprAI is a modular monolith.

```text
QML
 |
 v
ChatController
 |
 v
AgentRuntime
 |----------------------|
 |                      |
MockRuntime      NativeSuprAIRuntime
                       |
                       v
              RuntimeOrchestrator
                       |
                       v
                  AgentEngine
                       |
                       v
                  Provider port
                       ^
                       |
             OpenAI-compatible adapter

ApplicationBootstrap
  wires concrete implementations
```

Current source modules expose only their public `include/suprai/...` surface. Concrete runtime/provider implementations live in private source directories and are not visible to unrelated consumers.

The runtime split required by ADR-0002 is implemented in the current vertical slice. SQLite commits the Session/Input/Turn/Run and the user item before inference, then atomically commits the terminal Run outcome and assistant item before the terminal UI ACK. On startup the native runtime restores the newest local session from SQLite and marks previously prepared Runs as interrupted without replaying them. Selectable older sessions, ProviderAttempt journaling, tool iteration and the remainder of M3 build on these boundaries rather than replacing them.

This is deliberate: adding or replacing a provider should not require changes to QML or stateful orchestration, and session/persistence/task concerns must not accumulate inside the low-level agent execution kernel.

## AppImage preview

SuprAI can be tested from one executable without installing build-time Qt/CMake dependencies. The experimental `SuprAI-0.1.0-alpha.1-x86_64.AppImage` is generated and verified in GitHub Actions on Ubuntu 24.04 with Qt 6.12.0. Packaged smoke tests run under XCB/Xvfb and headless Wayland/Weston. This does **not** yet prove compatibility with every Linux distro or the actual KDE/NVIDIA host.

The `appimage-preview` GitHub Actions workflow uploads the AppImage + `.sha256`; after verified `main` builds it publishes the same pair as a GitHub pre-release. To run it on a machine whose local inference server speaks OpenAI-compatible Chat Completions:

```bash
chmod +x SuprAI-0.1.0-alpha.1-x86_64.AppImage
SUPRAI_RUNTIME=native SUPRAI_BASE_URL=http://127.0.0.1:8090/v1 \
  SUPRAI_MODEL=qwen3.8-27b SUPRAI_API_KEY=no-key \
  ./SuprAI-0.1.0-alpha.1-x86_64.AppImage
```

If FUSE is unavailable, add `APPIMAGE_EXTRACT_AND_RUN=1` before the invocation. For a fake local test, use `SUPRAI_RUNTIME=mock`. Full build instructions follow for contributors.

## Build

Requirements:
- CMake 3.24+;
- Ninja;
- C++20 compiler;
- Qt 6.8+ with the WaylandClient component when building the Linux desktop target;
- Wayland client development headers/libraries on the build host.

CI currently verifies with Qt 6.12.0.

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DSUPRAI_BUILD_TESTS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Run the deterministic prototype:

```bash
SUPRAI_RUNTIME=mock ./build/src/suprai
```

Run against an OpenAI-compatible local endpoint:

```bash
SUPRAI_RUNTIME=native \
SUPRAI_BASE_URL=http://127.0.0.1:8090/v1 \
SUPRAI_MODEL=bonsai2-27b \
SUPRAI_API_KEY=no-key \
./build/src/suprai
```

Current configuration environment variables:
- `SUPRAI_RUNTIME`;
- `SUPRAI_BASE_URL`;
- `SUPRAI_MODEL`;
- `SUPRAI_API_KEY`;
- `SUPRAI_SYSTEM_PROMPT`.

Prototype rule: API keys are not persisted in QSettings. Secure persistent credentials wait for SecretStore.

## Target

- Linux only.
- Qt 6 + QML UI.
- C++20 core and agent runtime.
- CMake + Ninja.
- AppImage as the first portable artifact.
- Local-first with optional remote model/API endpoints.
- OpenAI-compatible providers first.
- Wayland first; X11 compatibility where practical.
- No Node/Electron dependency in the shipped core application.

## Documentation

AI agents should read in this order:

1. `AGENTS.md`
2. `docs/DOCUMENTATION_POLICY.md`
3. `docs/ARCHITECTURE.md`
4. `docs/ROADMAP.md`
5. `docs/PROJECT_STATE.md`
6. `docs/REFERENCES.md`

Documentation is living and canonical. When research or implementation proves an older decision inferior or wrong, current documentation is replaced or corrected. Git history is the archive.
