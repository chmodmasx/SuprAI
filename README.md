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
- AgentRuntime public contract;
- deterministic MockRuntime;
- provider port separated from concrete adapters;
- OpenAI-compatible Chat Completions streaming adapter;
- application composition root that injects the selected provider/runtime;
- NativeSuprAIRuntime implemented as a thin production facade;
- RuntimeOrchestrator owns stateful conversation/runtime coordination;
- AgentEngine owns the low-level provider execution kernel;
- typed AgentEngineEvent -> RuntimeEvent adapter boundary;
- raw provider `reasoning_content` treated as ephemeral and excluded from later reconstructed prompts;
- explicit public/private module include boundaries enforced by CMake targets;
- CI build, unit/integration tests and QML startup smoke test.

Not implemented yet:
- durable SQLite sessions/Turns/Runs;
- Responses transport;
- tool calling and approvals;
- MCP;
- memory;
- subagents/deliberation;
- projects/files;
- safe Markdown renderer;
- settings UI for editing provider configuration;
- AppImage release artifact;
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

The runtime split required by ADR-0002 is already implemented in the current vertical slice. The current AgentEngine still has only the minimal provider execution behavior needed by the prototype; durable Turn/Run state, tool iteration and the rest of M3 build on this boundary rather than replacing it.

This is deliberate: adding or replacing a provider should not require changes to QML or stateful orchestration, and session/persistence/task concerns must not accumulate inside the low-level agent execution kernel.

## Build

Requirements:
- CMake 3.24+;
- Ninja;
- C++20 compiler;
- Qt 6.8+.

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
