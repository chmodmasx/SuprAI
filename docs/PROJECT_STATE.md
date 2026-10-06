# SuprAI Project State

```yaml
milestone: M0
status: in_progress
last_verified_commit: 97c91e1d264bb8fdcdd9e352abd6aae73b26c420

working:
  - repository exists and is writable
  - README establishes Linux-native Qt/QML direction
  - AGENTS.md establishes AI continuation contract
  - architecture v0 defines UI/core/backend/platform boundaries
  - Hermes selected as recommended first real backend adapter
  - AppImage selected as first portable artifact

broken: []

decisions:
  - Linux first
  - Qt Quick/QML presentation
  - C++20 application core
  - CMake/Ninja
  - no Electron or Node runtime in shipped core app
  - backend-neutral AgentBackend interface
  - Hermes may be first backend, but Hermes protocol must not leak into QML
  - Wayland first, X11 compatibility where practical
  - Qt WebEngine is optional and requires explicit architectural decision
  - SQLite for SuprAI-owned metadata; secure desktop storage for secrets

open_questions:
  - exact visual language and component system
  - whether v1 manages a local Hermes process or only attaches first
  - exact secure-secret implementation
  - exact AppImage tooling after proof build
  - minimum supported distro / ABI floor
  - whether terminal is part of early MVP or later
  - whether OpenClaw adapter is useful after Hermes adapter exists

next_exact_steps:
  - finish M0 reference notes and ADRs
  - create CMake/Qt source skeleton
  - implement QML shell with no backend
  - establish CI build
  - then implement AgentBackend + MockBackend before Hermes integration

verification_commands:
  - none_yet
```

Update this file at every milestone handoff.
