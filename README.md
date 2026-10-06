# SuprAI

Linux-first native desktop AI agent and workspace.

SuprAI is a complete Linux-native AI application: its own Qt Quick/QML interface, its own C++ application core, and its own agent runtime. Hermes Agent, OpenClaw and similar projects are research references only.

Status: architecture/planning.

## Initial target

- Linux only.
- Qt 6 + QML UI.
- C++20 core and agent runtime.
- CMake + Ninja.
- AppImage as the first portable artifact.
- Local-first operation with optional remote model/API endpoints.
- OpenAI-compatible model providers, including local servers.
- Streaming chat, tool calling, approvals, sessions, projects/workspaces, files, settings, tray and notifications.
- MCP support.
- Wayland first; X11 compatibility retained where practical.
- No Node/Electron dependency in the shipped core application.

## Architectural direction

The UI and the agent runtime are separate modules, but both are SuprAI.

```text
QML UI
  |
Application/Core layer
  |
AgentRuntime interface
  |-----------------------------|
NativeSuprAIRuntime         MockRuntime
  |
providers + agent loop + tools + MCP + memory + sessions
```

There is no Hermes runtime dependency in the planned product.

Hermes Agent, Hermes Desktop, OpenClaw and other agent systems may be inspected to learn from solved problems such as lifecycle, tool execution, approvals, memory, session semantics, remote execution and desktop integration. Their protocols and implementations are not SuprAI's architecture.

## Documentation

AI agents should read in this order:

1. `AGENTS.md`
2. `docs/ARCHITECTURE.md`
3. `docs/ROADMAP.md`
4. `docs/PROJECT_STATE.md`
5. `docs/REFERENCES.md`

The documentation is intentionally optimized for machine continuation rather than tutorial-style prose.
