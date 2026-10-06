# SuprAI Project State

```yaml
milestone: M0
status: complete_plus_deep_research
last_verified_commit: 2dcb2cb890fd76a2ff3255fbe0f5889c8ef8cd20

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
  - canonical conversation state is SuprAI-owned
  - runtime, UI and persistence have explicit thread ownership
  - context/token/compaction/cache semantics are documented
  - Linux desktop integration is freedesktop-first

accepted_adrs:
  - ADR-0001 native Qt stack
  - ADR-0002 native SuprAI runtime
  - ADR-0003 normalized inference transports; Responses preferred + Chat compatibility
  - ADR-0004 explicit agent-turn state machine
  - ADR-0005 PolicyEngine separate from OS containment
  - ADR-0006 MCP 2026-07-28-first native client
  - ADR-0007 Agent Skills standard
  - ADR-0008 SQLite/FTS5 + bounded curated memory v1
  - ADR-0011 canonical conversation state is local and provider-independent
  - ADR-0012 dedicated persistence worker + crash-safe side-effect journal
  - ADR-0013 freedesktop-first Linux desktop integration
  - ADR-0015 explicit Qt thread ownership
  - ADR-0017 append-oriented generalized conversation items
  - ADR-0018 provider-aware token budgeting
  - ADR-0019 auditable derived context compaction
  - ADR-0020 prompt/KV caches are optimization only

proposed_adrs:
  - ADR-0009 jsoncons as isolated JSON Schema 2020-12 validator
  - ADR-0010 Qt/CMake-owned AppDir staging for AppImage
  - ADR-0014 safe native transcript renderer
  - ADR-0016 QtKeychain SecretStore implementation

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
  - OpenAI Responses-compatible transport is preferred when semantically compatible
  - Chat Completions remains compatibility transport
  - provider-side conversation/response state is never canonical
  - every model request must remain reconstructible from SuprAI-owned state
  - AgentLoop is an explicit state machine
  - conversation history is append-oriented generalized items with stable SuprAI IDs
  - retries/regeneration/branches create lineage rather than rewriting completed history
  - every accepted tool call receives a terminal outcome item
  - tool side effects are journaled before execution
  - ambiguous crash-time mutating tool effects become outcome_unknown and are never blindly replayed
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
  - token budgeting uses effective runtime context, final-request accounting and output reserve
  - provider automatic truncation is not normal context management
  - compaction produces derived auditable artifacts and never rewrites canonical history
  - prompt/KV caches are performance-only
  - UI thread, runtime thread and persistence thread have explicit ownership
  - Wayland first, X11 compatibility where practical
  - global shortcuts use XDG Desktop Portal as primary path
  - application activation/single-instance uses freedesktop DBus semantics
  - QSystemTrayIcon is optional/capability-probed
  - notifications are portal/freedesktop capability-driven
  - Qt WebEngine is optional and not used for ordinary chat
  - secure desktop storage for secrets
  - initial process model is one modular native application
  - AppImage runtime ABI floor must be proven, not assumed

open_questions:
  - exact visual language and component system
  - exact safe transcript renderer implementation after benchmark
  - QtKeychain proof on KDE/GNOME/AppImage
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
  - exact background-job/subagent architecture after core turn model is proven

next_milestone: M1
next_exact_steps:
  - create CMake/Qt source skeleton
  - pin initial development Qt version and minimum CMake/compiler
  - use QApplication because tray integration may require Qt::Widgets while UI remains QML
  - create C++ application bootstrap
  - create QML shell with left navigation, chat area and inspector
  - add XDG path helper and structured logging
  - establish UI-facing model boundaries
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
