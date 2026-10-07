# SuprAI Roadmap

Roadmap type: proof-driven. Do not advance a milestone until its exit criteria are verified.

## M0 — Repository and architecture

Goal: make the project resumable by repository state alone.

Deliverables:
- README;
- AGENTS.md;
- architecture document;
- roadmap;
- mutable project state;
- reference/research notes;
- ADR directory.

Exit:
- explicit UI/core/agent/platform boundaries;
- native SuprAI runtime strategy selected;
- packaging strategy selected.

Status: COMPLETE.

## M1 — Native shell skeleton

Goal: prove the Qt foundation before agent complexity.

Implement:
- CMake project;
- C++20 executable;
- modular-monolith target layout from ADR-0029;
- separate CMake targets for domain/runtime/provider/tool/context/persistence/platform boundaries where present in M1;
- target-scoped PUBLIC/PRIVATE include/link dependencies;
- application composition root/bootstrap;
- no global include directories or hidden service locator;
- Qt Quick/QML application using QApplication/QQmlApplicationEngine;
- explicit UI-thread ownership;
- runtime/persistence worker-thread skeletons with clean shutdown;
- theme tokens;
- three-pane shell;
- left navigation;
- empty chat surface;
- inspector surface;
- settings route;
- XDG paths;
- structured logging;
- single-instance policy.

Tests:
- process starts under Wayland;
- process starts under X11;
- QML loads cleanly;
- clean shutdown;
- module targets compile with only declared dependencies;
- at least one fake adapter can replace a concrete boundary through composition without changing its consumer.

Exit:
- native shell runs without Node/Electron/Python;
- modular architecture source structure exists;
- cross-module private-header access is prevented by build layout;
- application bootstrap is the concrete composition point.

## M2 — Domain model + MockRuntime

Goal: prove the complete agent-facing UI contract independently from model/network behavior.

Implement:
- AgentRuntime abstract interface;
- runtime capabilities;
- Session/Input/Turn/Run/ConversationItem/ToolInvocation/Task domain value types as needed by the UI contract;
- runtime event bus;
- MockRuntime scripted fixture;
- append-oriented generalized conversation item model;
- C++ QAbstractListModel transcript projection;
- safe native Markdown rendering proof;
- composer;
- streaming delta path;
- tool cards;
- approval/clarification component;
- cancellation;
- error-state UI.

Exit:
- a complete deterministic fake agent turn works end-to-end.

## M3 — Native SuprAI runtime foundation

Goal: execute real model turns using SuprAI code only.

Implement:
- NativeSuprAIRuntime;
- durable Session/Input/Turn/Run identities;
- explicit turn state machine;
- foreground session state separate from background activity;
- followup queue and explicit interrupt path;
- owner-generation fencing for Runs;
- provider-neutral InferenceRequest/InferenceEvent types;
- ProviderRegistry;
- OpenAI Responses-compatible transport;
- Chat Completions compatibility transport;
- SSE/stream parsers;
- provider capability resolution;
- provider effective context-window discovery;
- TokenBudgetService capability ladder;
- model configuration;
- local endpoint support;
- canonical SQLite Session/Input/Turn/Run/Item stores;
- dedicated PersistenceWorker;
- WAL/foreign-keys/busy-timeout/migrations;
- provider-state-independent session reconstruction;
- cancellation request/confirmation semantics;
- runtime event conversion.

First real targets:
- llama.cpp;
- vLLM;
- NInfer;
- then compatible remote APIs.

Responses is preferred where supported; fallback to Chat must not hide arbitrary request/configuration errors.

Exit:
- user prompt -> provider -> streamed assistant response through NativeSuprAIRuntime;
- exact provider-native input-token count is used where supported;
- effective runtime context limit is discovered or explicitly configured;
- sessions persist/resume after provider restart without provider conversation state;
- runtime and persistence work do not block the UI thread;
- no Hermes/OpenClaw runtime involved.

## M4 — Tool calling + approvals

Implement:
- JSON Schema 2020-12 canonical tool schema;
- SchemaValidator boundary;
- ToolRegistry;
- ToolExecutor;
- provider tool-call translation;
- tool lifecycle events;
- PolicyEngine;
- allow/ask/deny decisions with resource scopes;
- ContainmentBackend capability probe;
- Landlock prototype;
- bubblewrap prototype;
- explicit no-containment fallback;
- cancellation/timeouts/output limits;
- durable side-effect journal;
- crash recovery with outcome_unknown for ambiguous mutating invocations;
- first safe built-in tools.

Initial built-in tool candidates:
- read file;
- list directory;
- write/edit file with explicit policy;
- controlled process execution;
- system information.

Exit:
- multi-step model -> tool -> result -> model loop works;
- dangerous operations are never silently escalated.

## M5 — MCP + skills

Implement:
- native MCP client targeting final 2026-07-28 semantics;
- stdio transport;
- Streamable HTTP transport;
- protocol-era/version handling;
- server registry/config;
- tool/resource/prompt mapping;
- structured tool results;
- approval/security boundaries;
- MRTR only as required by real integrations;
- legacy 2025-era adapter only if interoperability testing requires it;
- Agent Skills-compatible discovery/loader.

Do not build new behavior around deprecated MCP roots, sampling or protocol logging.

Research upstream agents only to compare ergonomics and failure modes.

Exit:
- configured MCP tools participate in the same NativeSuprAIRuntime AgentLoop as built-in tools;
- standard SKILL.md skills are progressively discoverable/loadable.

## M6 — Context management + memory

Implement:
- ContextManager;
- TokenBudgetService integration;
- provider-native exact counting where available;
- explicit output reserve and safety margin;
- context-window policy;
- derived CompactionArtifact persistence;
- transcript compaction/summarization without rewriting canonical history;
- tool-call/result-safe compaction boundaries;
- prompt/KV-cache optimization layer that is never correctness-critical;
- pinned project context;
- MemoryService;
- SQLite FTS5 history search;
- bounded curated active memory;
- user/profile/project memory scopes;
- provenance/trust metadata;
- explicit memory mutation/review policy.

Requirements:
- model context size is configurable/discovered;
- no hidden uncontrolled growth;
- summarization never silently becomes canonical history;
- memory retrieval never erases source/provenance;
- no vector/embedding dependency is required for v1.

Exit:
- long sessions remain usable with deterministic context policy;
- history search and active memory remain distinct and inspectable.

## M7 — Projects + files

Implement:
- Project model;
- project directories;
- session/project association;
- file tree;
- native safe previews;
- drag/drop attachments;
- git repository recognition;
- project instructions/context.

Exit:
- project-aware agent workflow works locally.

## M8 — Linux desktop integration

Implement:
- stable reverse-DNS application identity;
- org.freedesktop.Application activation/single-instance behavior via QtDBus;
- optional tray/status notifier through QSystemTrayIcon;
- Portal Notification v2 with freedesktop Notifications fallback;
- desktop entry/icons/deep links;
- SecretStore with QtKeychain candidate proof;
- portal-aware FileChooser/OpenURI/Screenshot operations;
- GlobalShortcuts Portal v2 capability;
- optional X11 fallback only behind the same shortcut interface;
- optional autostart;
- polished close-to-tray behavior.

Do not implement Wayland global shortcuts with raw X11 grabs.

Validate:
- KDE Plasma Wayland;
- GNOME Wayland;
- X11 fallback.

## M9 — AppImage release pipeline

Implement:
- release CMake install layout;
- AppDir generation;
- Qt/QML deployment;
- AppImage generation;
- CI artifact;
- clean VM smoke test;
- dependency report;
- checksums.

Record exact ABI floor.

## M10 — Orchestration, Tasks and advanced agent UX

Core orchestration:
- queued input modes: steer/followup/collect/interrupt;
- safe steering boundaries;
- unified durable TaskManager;
- task delivery/notification state;
- background process Tasks;
- subagent child Sessions;
- subagent purposes: delegation/deliberation/verification/research/coding;
- child context modes: full/scoped/compacted;
- ReasoningWorkspace as ephemeral Run-local scratch state;
- ReturnCapsule as compact child-to-parent merge boundary;
- isolated deliberation/context folding through subagents, not a separate branch runtime;
- direct/isolated/auto deliberation policy prototype;
- parent/child reasoning-effort/profile selection;
- read-mostly default policy for deliberation children;
- attached vs detached child semantics;
- push-based completion;
- yield/resume without model polling;
- exact cancellation scope;
- restart reconciliation and bounded recovery;
- MCP Tasks integration;
- schedule/automation definitions and occurrence execution.

Advanced candidates:
- multiple profiles/agents;
- multiple providers/models;
- skills manager;
- tool permissions UI;
- session branching;
- terminal;
- artifacts;
- voice/STT/TTS;
- PDF/image/browser previews;
- bounded nested subagents;
- multiple deliberators + verifier + parent synthesis;
- optional systemd transient-process backend after ADR-0027 proof.

Exit:
- a parent can spawn background work, remain interactive, receive completion without polling, cancel exact work, and recover/reconcile persisted Tasks after restart;
- a reasoning-heavy Turn can use an attached deliberation child and merge only a compact ReturnCapsule into the parent context;
- full/scoped/compacted child-context modes are benchmarked against direct reasoning;
- raw child reasoning does not become parent canonical-history debt;
- stale child/task completions cannot enter a replaced Session;
- scheduled occurrences are idempotent and obey current policy.

These extend NativeSuprAIRuntime; they do not introduce Hermes/OpenClaw as runtime dependencies.

## Priority rule

If forced to choose:
1. correctness of AgentLoop/tool/session lifecycle;
2. recoverability and security;
3. native Linux behavior;
4. interaction quality;
5. performance;
6. visual polish;
7. feature count.

## Explicitly deferred

- Windows/macOS;
- mobile;
- custom inference engine;
- custom browser engine;
- public binary plugin ABI before real external consumers prove its shape;
- compatibility adapters for Hermes/OpenClaw.
