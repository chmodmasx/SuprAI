# SuprAI Project State

```yaml
milestone: M0
status: complete_plus_deep_research
last_verified_commit: 78f27a967480b4a15c7748f7cb7b3a75d224e666

working:
  - repository exists and is writable
  - README establishes Linux-native Qt/QML direction
  - AGENTS.md establishes AI continuation contract
  - architecture defines UI/core/native-agent/platform boundaries
  - proof-driven roadmap exists
  - NativeSuprAIRuntime is the canonical production agent runtime
  - MockRuntime is test-only
  - Hermes/OpenClaw/Goose/GPT4All/OpenCode/Codex/ACP are research references only
  - deep runtime research is recorded under docs/research/
  - advanced execution research is recorded under docs/research/
  - AppImage selected as first portable artifact
  - canonical conversation state is SuprAI-owned
  - runtime, UI and persistence have explicit thread ownership
  - context/token/compaction/cache semantics are documented
  - Linux desktop integration is freedesktop-first
  - Session/Input/Turn/Run/Attempt/Task identities are distinct
  - queued input and steering semantics are documented
  - subagent lifecycle is modeled through Task-owned child Sessions
  - unified durable TaskManager architecture is defined
  - restart recovery uses owner generations and replay-safe reconciliation
  - schedules/automations are separate from execution Tasks
  - documentation policy is canonical and living; stale decisions are replaced/removed rather than accumulated
  - contradiction scans remove stale architecture/terminology instead of preserving it
  - context folding / reasoning isolation is unified with the subagent architecture
  - deliberation is a subagent purpose, not a separate runtime subsystem
  - ReasoningWorkspace is ephemeral Run-local scratch state
  - ReturnCapsule is the compact child-to-parent merge boundary

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
  - ADR-0021 distinct Session/Input/Turn/Run/Attempt/Task identities
  - ADR-0022 queued input + steering semantics
  - ADR-0023 subagents as Task-owned child Sessions
  - ADR-0024 unified durable Task registry
  - ADR-0025 owner-generation-aware restart recovery
  - ADR-0026 schedules separate from Task execution
  - ADR-0028 isolated deliberation reuses subagent infrastructure

proposed_adrs:
  - ADR-0009 jsoncons as isolated JSON Schema 2020-12 validator
  - ADR-0010 Qt/CMake-owned AppDir staging for AppImage
  - ADR-0014 safe native transcript renderer
  - ADR-0016 QtKeychain SecretStore implementation
  - ADR-0027 optional systemd transient user-service process backend

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
  - Session/Input/Turn/Run/ProviderAttempt/ToolInvocation/Task identities are distinct
  - input acceptance is separate from foreground-turn completion
  - foreground Session state is separate from background Task activity
  - a Turn may span multiple Run generations
  - retries/regeneration/branches create lineage rather than rewriting completed history
  - every accepted tool call receives a terminal outcome item
  - steering does not preempt an already-running tool
  - skipped-by-steering calls still receive synthetic terminal results
  - accepted steering is not equivalent to delivered steering
  - subagents execute through NativeSuprAIRuntime in child Sessions
  - subagent purpose is metadata/policy, not a separate runtime type
  - baseline subagent purposes include delegation, deliberation, verification, research and coding
  - reasoning isolation/context folding reuses subagents; there is no standalone standalone deliberation-branch runtime
  - subagent context is isolated/explicit by default
  - child context modes are full/scoped/compacted
  - ReasoningWorkspace is ephemeral Run-local scratch state, not canonical history
  - deliberation children return a compact ReturnCapsule instead of raw chain-of-thought
  - deliberation children are read-mostly by default
  - a child may use the same model with different reasoning effort or another configured model/profile
  - logical subagent concurrency does not imply simultaneous GPU generation
  - ExecutionScheduler controls physical child execution according to provider/model/hardware capacity
  - child authority is an intersection and can never widen parent authority
  - attached and detached child work have explicit lifecycle semantics
  - subagent/task completion is push/event driven; model polling loops are forbidden
  - TaskManager unifies subagent/process/MCP/scheduled background work
  - Task execution state and result-delivery state are separate
  - cancel_requested is not the same as confirmed cancelled
  - task tool side effects are journaled before execution
  - ambiguous crash-time mutating effects become outcome_unknown and are never blindly replayed
  - persisted running state is not proof of liveness
  - recovery uses exact owner generations and fences stale late events
  - recovery attempts are bounded
  - schedules are trigger definitions; each occurrence creates a Task
  - schedule misfire and overlap policy are explicit
  - scheduled execution revalidates current policy
  - canonical tool schemas use JSON Schema 2020-12
  - tool authorization is allow/ask/deny with scoped rules
  - approval is not sandboxing
  - containment is feature-probed Landlock/bubblewrap/none
  - MCP targets current final 2026-07-28 semantics
  - MCP Tasks are an external Task source, not SuprAI's internal task model
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
  - QProcess is baseline for process tools/tasks
  - systemd transient user services are only a proposed optional durability backend
  - context-folding research confirms isolated temporary work -> compact return as a real long-horizon pattern
  - SuprAI implements that pattern through deliberation subagents rather than a second branch manager
  - NInfer preserve_thinking semantics are provider/template-specific and must not be assumed globally
  - provider reasoning-history controls are optimizations; canonical context retention is SuprAI-owned
  - current documentation contains the best current truth; Git history is the archive
  - obsolete ADRs/research/docs are rewritten, merged, or deleted when better evidence replaces them
  - contradictory old/new documentation is not permitted

open_questions:
  - exact visual language and component system
  - exact safe transcript renderer implementation after benchmark
  - QtKeychain proof on KDE/GNOME/AppImage
  - exact first built-in tool set
  - default persisted permission UX
  - exact initial steering UX exposed in M3/M10
  - exact concurrency scheduler limits/defaults
  - exact Task persistence schema/migrations
  - default child TaskBrief fields and output contract
  - exact attached-vs-detached subagent default
  - exact recovery freshness windows by Task/Run type
  - Landlock/bubblewrap effective-default policy after prototype
  - jsoncons proof results and exact dependency pin
  - exact Qt 6.12 toolchain source for Ubuntu-22.04-compatible release builds
  - final AppImage finalizer/tool
  - exact MCP legacy 2025-era compatibility scope
  - MCP Tasks extension implementation timing
  - memory mutation/review UX
  - SuprAI project license and distribution notices
  - semantic/vector retrieval only if FTS5 measurements justify it
  - systemd transient-process backend output/log/containment proof
  - deliberation-subagent full vs scoped vs compacted context benchmark
  - direct vs isolated vs auto deliberation policy thresholds
  - ReturnCapsule exact schema and merge policy
  - parent-vs-child reasoning effort/profile policy
  - multiple deliberator/verifier scheduling policy
  - generic OpenAI-compatible reasoning-history capability detection

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
  - in M3 implement Session/Input/Turn/Run identities before provider transport
  - implement followup queue + explicit interrupt before advanced steering
  - keep TaskManager schema-compatible even if background work lands later
  - keep subagent purpose/context-mode fields schema-compatible before orchestration lands
  - later benchmark direct reasoning vs deliberation subagents on NInfer/llama.cpp/vLLM
  - then implement normalized inference types and NativeSuprAIRuntime state machine

verification_commands:
  - none_yet_no_code
```

Update this file at every milestone handoff.
