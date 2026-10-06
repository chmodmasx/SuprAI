# SuprAI Project State

```yaml
milestone: M0
status: complete
last_verified_commit: a29584af1fe0e8578de931ff5be48f18c08c2b5c

working:
  - repository exists and is writable
  - README establishes Linux-native Qt/QML direction
  - AGENTS.md establishes AI continuation contract
  - architecture v0 defines UI/core/backend/platform boundaries
  - proof-driven roadmap exists
  - Hermes selected as recommended first real backend adapter
  - MockBackend required before Hermes integration
  - AppImage selected as first portable artifact
  - native Qt stack decision recorded in ADR-0001
  - backend-neutral Hermes-first decision recorded in ADR-0002
  - upstream Hermes/OpenClaw/Qt/AppImage reference notes recorded

broken: []

decisions:
  - Linux first
  - Qt Quick/QML presentation
  - C++20 application core
  - CMake/Ninja
  - no Electron or Node runtime in shipped core app
  - backend-neutral AgentBackend interface
  - Hermes may be first real backend, but Hermes protocol must not leak into QML
  - MockBackend precedes HermesBackend
  - Wayland first, X11 compatibility where practical
  - Qt WebEngine is optional and requires explicit architectural decision
  - SQLite for SuprAI-owned metadata; secure desktop storage for secrets

open_questions:
  - exact visual language and component system
  - whether first Hermes integration only attaches or also manages a local process
  - exact secure-secret implementation
  - exact AppImage tooling after proof build
  - minimum supported distro / ABI floor
  - whether terminal is part of early MVP or later
  - whether OpenClaw adapter is useful after Hermes adapter exists

next_milestone: M1
next_exact_steps:
  - create CMake/Qt source skeleton
  - create C++ application bootstrap
  - create QML shell with left navigation, chat area and inspector
  - add XDG path helper and structured logging
  - add basic tests and CI build
  - verify Wayland and X11 launch paths before backend work

verification_commands:
  - none_yet
```

Update this file at every milestone handoff.
