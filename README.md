# SuprAI

Linux-first native desktop AI workspace.

SuprAI is intended to provide a Hermes-Desktop-class experience without Electron: a Qt Quick/QML interface, a C++ application core, native Linux integration, and replaceable agent backends.

Status: architecture/planning only.

## Initial target

- Linux only.
- Qt 6 + QML UI.
- C++20 core.
- CMake + Ninja.
- AppImage as the first portable artifact.
- Local and remote agent backends.
- Streaming chat, tool activity, approvals, sessions, projects/workspaces, files, settings, tray and notifications.
- Wayland first; X11 compatibility retained where practical.
- No Node/Electron dependency in the shipped application.

## Architectural direction

The UI is not the agent.

```text
QML UI
  |
Application/Core layer
  |
AgentBackend interface
  |------------------------------|
Hermes gateway adapter       SuprAI native agent (later)
  |
JSON-RPC / WebSocket
```

The first useful backend may be Hermes Agent, because its gateway already exposes sessions, streaming, tool calls, approvals and persistence. SuprAI must not couple its domain model to Hermes-specific types; Hermes is an adapter, not the architecture.

OpenClaw and other agent desktops are reference implementations to study, not dependencies unless an ADR explicitly approves one.

## Documentation

AI agents should read in this order:

1. `AGENTS.md`
2. `docs/ARCHITECTURE.md`
3. `docs/ROADMAP.md`
4. `docs/PROJECT_STATE.md`
5. `docs/REFERENCES.md`

The documentation is intentionally optimized for machine continuation rather than tutorial-style prose.
