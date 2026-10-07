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

Status: **IN PROGRESS**.

Verified vertical slice at commit `98fb72265f38d375145206dbaa039e93d0b8a103`:
- CMake/C++20/Qt 6 application builds in CI with Qt 6.12.0;
- module PUBLIC/PRIVATE include boundaries are enforced by target-scoped CMake paths;
- `ApplicationBootstrap` is the concrete composition root;
- QML three-pane shell starts successfully in the offscreen smoke test;
- runtime runs on a dedicated QThread;
- persistence worker skeleton runs on a separate dedicated QThread and shuts down cleanly;
- XDG config/data/cache/state paths are resolved through QStandardPaths and created explicitly;
- logging uses stable suprai.app/runtime/provider/persistence/platform Qt categories;
- QML has explicit Chat and Configuración routes;
- org.freedesktop.Application-compatible Activate/Open/ActivateAction projection exists through QtDBus;
- a second process can forward activation to the registered primary when a session bus is available;
- environments without a session bus degrade explicitly instead of preventing startup;
- worker shutdown is independent of the already-stopped main Qt event loop and CI rejects shutdown timeouts;
- CMake install staging deploys QML imports, Qt runtime closure, qt.conf, Qt Widgets and explicit X11/Wayland QPA support;
- staged runtime launches successfully with Qt development environment variables removed;
- staged XCB path is smoke-tested under Xvfb;
- staged Wayland path is smoke-tested under headless Weston;
- MockRuntime streams a deterministic turn;
- a concrete OpenAI-compatible provider is injected through the provider port;
- NativeSuprAIRuntime is a thin facade over RuntimeOrchestrator -> AgentEngine;
- AgentEngine emits typed internal events translated by RuntimeEventAdapter;
- RuntimeOrchestrator owns in-memory history/current assistant state in the current prototype;
- a fake OpenAI-compatible integration test completes two streamed turns;
- a dedicated runtime-layering test verifies that raw reasoning stays outside orchestrator history;
- raw `reasoning_content` from the first turn is verified absent from the second request;
- 7/7 CTest tests pass;
- ordinary chat rendering remains plain text in this prototype.

Still required before M1 can be COMPLETE:
- physical KDE Wayland and X11 launch proof on a real desktop;
- GNOME Wayland proof when available.

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
- process starts under virtual/headless Wayland in CI and physical Wayland before M1 closes;
- process starts under virtual X11/XCB in CI and physical X11 before M1 closes;
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

Early prototype proof already exists for:
- AgentRuntime boundary;
- MockRuntime;
- transcript QAbstractListModel;
- composer;
- streaming deltas;
- cancellation;
- error display.

M2 is not complete: the durable generalized domain model, tool/approval/clarification UI, runtime capability model and safe Markdown proof remain.

Implement:
- AgentRuntime abstract interface;
- runtime capabilities;
- domain/application event contract distinct from future AgentEngine events;
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

An intentionally small M3 vertical slice was prototyped early:
- NativeSuprAIRuntime is already a production facade over RuntimeOrchestrator -> AgentEngine;
- AgentEngine already owns minimal provider execution;
- RuntimeOrchestrator already owns the prototype's stateful conversation coordination;
- typed AgentEngineEvent -> RuntimeEvent translation exists;
- OpenAI-compatible Chat Completions streaming, SSE parsing and cancellation path exist;
- provider implementation is injected through a narrow public port;
- ProviderCapabilities already models Unknown / Supported / Unsupported;
- the current Chat adapter reports effective adapter capabilities explicitly;
- provider transport configuration stays in the application composition layer;
- separated `reasoning_content` is ephemeral and is not replayed into the next prompt;
- these boundaries are covered by deterministic fake-provider/fake-server tests.

This is not the final M3 runtime. Durable Session/Input/Turn/Run state, Responses, capability probing, persistence, token budgeting and explicit turn-state-machine semantics remain.

Implement:
- retain/expand the implemented NativeSuprAIRuntime -> RuntimeOrchestrator -> AgentEngine split;
- evolve the current internal RuntimeEvent adapter into the full domain/application event mapping;
- preserve the blocking interceptor vs non-blocking observer boundary;
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
- expand the implemented supported/unsupported/unknown capability contract into provider/model resolution;
- capability provenance/probing/cache invalidation without treating missing metadata as denial;
- provider effective context-window discovery;
- TokenBudgetService capability ladder;
- model configuration;
- local endpoint support;
- canonical SQLite Session/Input/Turn/Run/Item stores;
- dedicated PersistenceWorker;
- WAL/foreign-keys/busy-timeout/migrations;
- provider-state-independent session reconstruction;
- cancellation request/confirmation semantics;
- retry only before observable generation;
- lazy persistence for untouched empty sessions;
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
- slow UI/telemetry/log observers cannot throttle provider streaming;
- missing provider capability metadata does not silently remove supported tools/images;
- no Hermes/OpenClaw/Cline runtime involved.

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
- independent caps for live output, model projection and persisted logs/artifacts;
- identity-bound approval requests;
- prepared ChangeSet -> preview -> approval -> exact-apply file mutation path;
- revalidation/invalidation when the target changes after preview;
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
- dangerous operations are never silently escalated;
- an approved file edit applies exactly the reviewed ChangeSet or is invalidated;
- runaway tool/process output cannot grow memory/UI/model context without bounds.

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
- configured MCP tools participate in the same NativeSuprAIRuntime -> RuntimeOrchestrator -> AgentEngine tool path as built-in tools;
- standard SKILL.md skills are progressively discoverable/loadable.

## M6 — Context management + memory

Implement:
- ContextManager;
- TokenBudgetService integration;
- provider-native exact counting where available;
- provider-reported actual usage feedback for conservative estimator calibration;
- explicit output reserve and safety margin;
- context-window policy;
- derived CompactionArtifact persistence;
- transcript compaction/summarization without rewriting canonical history;
- deterministic emergency overflow recovery;
- tool-call/result-safe compaction boundaries;
- LargeResultArtifact storage + bounded model-facing tool-result projection;
- explicit bounded artifact-read capability;
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
- project instructions/context;
- optional WorkspaceCheckpointService prototype for Git projects;
- compare/restore workspace state independently from conversation lineage;
- restore conversation only / workspace only / both semantics;
- transactional restore safeguards that do not silently discard newer user commits;
- evaluate isolated Git-worktree execution mode for coding tasks.

Exit:
- project-aware agent workflow works locally;
- checkpoint-enabled Git project can compare and safely restore workspace state independently of chat history.

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
- ToolInvocation -> Task ownership handoff for "Continue while running";
- bounded detached command logs/artifacts;
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
- Plan/Act-like planning/action behavior implemented as profiles/policies rather than a core mode boolean;
- LoopGuard for repeated no-progress tool/error cycles;
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
- a long-running foreground command can transfer ownership to TaskManager without becoming an orphan process or blocking the foreground Turn;
- a reasoning-heavy Turn can use an attached deliberation child and merge only a compact ReturnCapsule into the parent context;
- full/scoped/compacted child-context modes are benchmarked against direct reasoning;
- raw child reasoning does not become parent canonical-history debt;
- stale child/task completions cannot enter a replaced Session;
- scheduled occurrences are idempotent and obey current policy.

These extend NativeSuprAIRuntime; they do not introduce Hermes/OpenClaw as runtime dependencies.

## Priority rule

If forced to choose:
1. correctness of AgentEngine/tool/RuntimeOrchestrator/session lifecycle;
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
