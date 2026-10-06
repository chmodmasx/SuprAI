# SuprAI Project State

```yaml
milestone: M0
status: complete_plus_deep_research
last_verified_commit: 576dfc86da9f0bdb2a03cd48653a70a93a6076da

working:
  - repository exists and is writable
  - README establishes Linux-native Qt/QML direction
  - AGENTS.md establishes AI continuation contract
  - architecture defines UI/core/native-agent/platform boundaries
  - proof-driven roadmap exists
  - NativeSuprAIRuntime is the canonical production agent runtime
  - MockRuntime is test-only
  - Hermes/OpenClaw/Goose/GPT4All/OpenCode are research references only
  - deep runtime research is recorded under docs/research/
  - AppImage selected as first portable artifact

accepted_adrs:
  - ADR-0001 native Qt stack
  - ADR-0002 native SuprAI runtime
  - ADR-0003 normalized inference transports; Responses preferred + Chat compatibility
  - ADR-0004 explicit agent-turn state machine
  - ADR-0005 PolicyEngine separate from OS containment
  - ADR-0006 MCP 2026-07-28-first native client
  - ADR-0007 Agent Skills standard
  - ADR-0008 SQLite/FTS5 + bounded curated memory v1

proposed_adrs:
  - ADR-0009 jsoncons as isolated JSON Schema 2020-12 validator
  - ADR-0010 Qt/CMake-owned AppDir staging for AppImage

broken: []

decisions:
  - Linux first
  - Qt Quick/QML presentation
  - C++20 application core and agent runtime
  - CMake/Ninja
  - no Electron or Node runtime in shipped core app
  - NativeSuprAIRuntime is built from the start
  - no planned Hermes/OpenClaw runtime dependency or adapter
  - provider wire protocols terminate at provider transports
  - canonical internal inference types are SuprAI-owned
  - OpenAI Responses-compatible transport is preferred where supported
  - Chat Completions transport remains compatibility path
  - AgentLoop is an explicit state machine
  - canonical tool schemas use JSON Schema 2020-12
  - tool authorization is allow/ask/deny with scoped rules
  - approval is not sandboxing
  - containment is feature-probed Landlock/bubblewrap/none
  - MCP targets current final 2026-07-28 semantics
  - MCP roots/sampling/protocol-logging are not foundations for new design
  - skills use Agent Skills SKILL.md compatibility
  - canonical history and active memory are separate
  - SQLite FTS5 exists before semantic/vector memory
  - memory records carry provenance/trust outside recalled prose
  - Wayland first, X11 compatibility where practical
  - Qt WebEngine is optional and requires explicit architectural decision
  - secure desktop storage for secrets
  - initial process model is one modular native application
  - AppImage runtime ABI floor must be proven, not assumed

open_questions:
  - exact visual language and component system
  - exact secure-secret implementation
  - exact first built-in tool set
  - default persisted permission UX
  - Landlock/bubblewrap effective-default policy after prototype
  - jsoncons proof results and exact dependency pin
  - exact Qt 6.12 toolchain source for Ubuntu-22.04-compatible release builds
  - final AppImage finalizer/tool
  - exact MCP legacy 2025-era compatibility scope
  - memory mutation/review UX
  - SuprAI project license and distribution notices
  - semantic/vector retrieval only if FTS5 measurements justify it

next_milestone: M1
next_exact_steps:
  - create CMake/Qt source skeleton
  - pin initial development Qt version and minimum CMake/compiler
  - create C++ application bootstrap
  - create QML shell with left navigation, chat area and inspector
  - add XDG path helper and structured logging
  - add basic tests and CI build
  - verify Wayland and X11 launch paths
  - prove Qt CMake QML deployment into a staged directory
  - keep AppImage proof minimal at M1; full release pipeline remains later
  - then implement AgentRuntime + MockRuntime
  - then implement normalized inference types and NativeSuprAIRuntime state machine

verification_commands:
  - none_yet_no_code
```

Update this file at every milestone handoff.
