# AGENTS.md — SuprAI AI Engineering Contract

Purpose: make future AI work deterministic, resumable, and architecture-safe.

## 0. Read order

Before changing code:
1. Read this file.
2. Read `docs/DOCUMENTATION_POLICY.md`.
3. Read `docs/ARCHITECTURE.md`.
4. Read `docs/ROADMAP.md`.
5. Read `docs/PROJECT_STATE.md`.
6. Read any ADR or reference document relevant to the change.
7. Inspect the current code and tests.
8. If implementation and canonical documentation disagree, determine which is wrong and resolve the contradiction in the same change. Never preserve a known-stale document merely for history.

## 1. Product identity

SuprAI is a Linux-native desktop AI agent and workspace.

It is NOT:
- an Electron application;
- a browser dashboard wrapped in a window;
- a Hermes frontend;
- a Hermes fork;
- an OpenClaw frontend;
- an OpenClaw fork;
- a shell whose production agent logic lives in another agent project.

Primary implementation:
- C++20;
- Qt 6;
- Qt Quick/QML;
- CMake/Ninja;
- AppImage first.

Hermes/OpenClaw/other agents are research references only unless a future ADR explicitly changes that rule.

## 2. Core invariant: UI != agent runtime

Both are SuprAI, but they have separate authority.

### Presentation authority
Owned by QML/UI:
- visible navigation;
- temporary selection;
- panel geometry;
- focus;
- animations;
- unsent composer draft;
- purely visual state.

### Application/machine authority
Owned by C++ application core:
- process lifecycle;
- local filesystem integration;
- desktop integration;
- secret retrieval;
- notification/tray/global shortcut integration;
- provider endpoint configuration;
- runtime lifecycle.

### Agent authority
Owned through the NativeSuprAIRuntime production facade, with internal ownership split explicitly:

RuntimeOrchestrator owns:
- sessions and canonical conversation lineage;
- active Turn/Run lifecycle;
- persistence coordination;
- approvals/clarifications;
- Task/subagent coordination;
- context/memory/project orchestration;
- recovery and event projection.

AgentEngine owns:
- low-level provider/tool iteration for an active Run;
- normalized inference consumption;
- engine state-machine transitions;
- low-level cancellation/iteration/completion limits.

Provider/model wire objects, SQLite details and UI state do not become AgentEngine authority.

Never implement agent behavior in QML.

## 3. Runtime abstraction

Production agent behavior is SuprAI-owned.

Conceptual interface:

```text
AgentRuntime
  start()
  stop()
  capabilities()
  listSessions()
  createSession()
  resumeSession()
  submitPrompt()
  cancelTurn()
  answerRequest()
  listModels()
  listProfiles()
  listTools()
  listSkills()
  events()
```

Initial implementations:
- NativeSuprAIRuntime: production runtime.
- MockRuntime: deterministic test runtime.

Do not create a HermesBackend/OpenClawBackend as part of the planned product.

Any future external-runtime adapter requires an ADR and must not displace NativeSuprAIRuntime as the canonical implementation.

## 4. Native agent runtime minimum architecture

NativeSuprAIRuntime is the production AgentRuntime facade. Internally:

```text
NativeSuprAIRuntime
        |
        v
RuntimeOrchestrator
        |
        v
AgentEngine
        |
        +--> provider ports
        +--> tool execution ports
```

RuntimeOrchestrator composes:
- ContextManager;
- persistence/repository ports;
- ApprovalManager / PolicyEngine;
- TaskManager;
- MCPClientManager;
- SkillRegistry;
- MemoryService;
- event translation.

AgentEngine must remain comparatively small and must not grow into a session/persistence/task/UI god object.

Provider implementations are replaceable transports, not agent runtimes.

Initial provider target:
- OpenAI-compatible HTTP APIs, including local endpoints such as llama.cpp, vLLM and NInfer-compatible servers where protocol-compatible.

Later providers may include native Anthropic/Gemini/etc. transports if required.

## 5. Event model

Runtime traffic is layered before reaching UI:

```text
provider wire event
 -> normalized InferenceEvent
 -> AgentEngineEvent
 -> RuntimeEventAdapter
 -> SuprAI domain/application event
 -> UI projection
```

Engine/provider events are never QML contracts.

Examples:
- RuntimeReady
- RuntimeStopped
- SessionCreated
- SessionUpdated
- MessageAdded
- MessageDelta
- TurnStarted
- TurnFinished
- ReasoningStatusChanged
- ToolStarted
- ToolUpdated
- ToolFinished
- UserActionRequired
- ContextUpdated
- ErrorRaised

Requirements:
- ordered per session;
- stale async responses cannot overwrite newer state;
- terminal events flush immediately;
- cancel is explicit and observable;
- UI distinguishes idle, working, waiting-for-user, degraded, failed, and ready states;
- high-frequency observer delivery may be queued/coalesced after ordering semantics are preserved;
- UI/telemetry/log observers must never synchronously throttle provider token generation.

Blocking interceptors such as policy, approval, request preparation and tool-result validation are separate from observational subscribers.

## 6. State rules

Ask: "who is allowed to be correct about this state?"

- Agent/session truth is owned by RuntimeOrchestrator through NativeSuprAIRuntime.
- Active Run execution state is owned by AgentEngine/TurnStateMachine and projected upward explicitly.
- Machine truth is owned by application core.
- UI owns only presentation state.
- Persisted state must have explicit scope: global, profile, project, session, or window.
- Never use one global key for data that can differ by profile/project/session.
- Every optimistic mutation must have rollback behavior.

## 7. Execution identity and async-work rules

Do not collapse these concepts:
- Session = durable conversation context.
- Input = durably admitted user/internal input.
- Turn = logical foreground work episode.
- Run = one executable generation/segment of a Turn.
- ProviderAttempt = one model request attempt.
- ToolInvocation = one durable tool execution.
- Task = asynchronous/background work record.

Input acceptance is not Turn completion.

Foreground Session state is separate from background Task activity.

### Queued input

Steering, followup, collect and interrupt are distinct semantics.

Steering:
- never terminates an already-running tool merely to apply guidance;
- is consumed only at explicit safe boundaries;
- must preserve tool-call/result structural pairing;
- tracks accepted vs actually delivered/missed.

### Subagents

A subagent is a Task-owned child Session running NativeSuprAIRuntime.

Subagents:
- cannot widen parent/requester authority;
- receive explicit TaskBrief context by default;
- use bounded depth/concurrency;
- complete by event/push, not model polling;
- may be attached or detached, but that choice is explicit.

Subagent purposes include delegation, deliberation, verification, research and coding. Purpose is policy/profile metadata, not a separate runtime implementation.

Reasoning isolation/context folding reuses the subagent system. Do not create a parallel standalone deliberation-branch runtime.

A deliberation child:
- is normally attached to the parent Turn;
- is read-mostly by default;
- may use full/scoped/compacted child context;
- may use the same model with different reasoning effort;
- keeps raw reasoning in a Run-local ReasoningWorkspace;
- returns a compact ReturnCapsule to the parent;
- does not inject raw child reasoning into parent canonical context.

Logical subagent concurrency does not imply simultaneous GPU generation. ExecutionScheduler decides physical execution according to provider/model/hardware capacity.

### Tasks

The model must never poll in a loop just to discover whether background work finished.

TaskManager owns:
- background process/subagent/MCP/scheduled work tracking;
- external polling where required;
- completion/progress events;
- cancellation/reconciliation;
- explicit ownership transfer when a foreground ToolInvocation chooses "Continue while running".

A running process is not a Task merely because the UI stopped watching it. Ownership transfer must be explicit and durable.

Execution status and delivery status are separate.

`cancel_requested` is not equivalent to confirmed `cancelled`.

### Recovery

Persisted `running` state is not proof an executor is live.

Recovery requires exact ownership/generation checks.

Ambiguous mutating side effects become `outcome_unknown` and are never automatically replayed.

Schedules are trigger definitions; each firing creates execution work. A Schedule is not a long-lived Task.

## 8. Linux-native rules

Wayland is the primary display target.

Must not assume:
- global coordinates are always available;
- X11-only window APIs exist;
- arbitrary global keyboard hooks work without compositor/portal support;
- system tray behaves identically across desktops;
- one Secret Service implementation is present.

Platform capabilities must be probed.

Desktop integrations belong behind `platform/linux` interfaces, never scattered through QML.

## 9. Security rules

- No API keys/tokens/passwords in plaintext config.
- Use a secure desktop secret backend when available.
- Never expose arbitrary shell execution through a generic QML bridge.
- Native capabilities are explicit and narrow.
- Tool execution has an explicit policy and approval path.
- Every approval is bound to exact Session/Turn/Run/Task/ToolInvocation/UserAction identity.
- File mutation approvals bind to a prepared ChangeSet and expected base/version; stale approved changes are invalidated, not applied.
- Web/HTML previews are untrusted content.
- No popup/external navigation without an explicit policy.
- No hidden automatic privilege escalation.
- Any sudo/root flow must be explicit to the user.
- MCP servers are untrusted external capabilities until configured and approved.

## 10. Dependency and modularity policy

ADR-0029 is mandatory.

SuprAI is a modular monolith.

Every architecture-significant module must have:
- an explicit public contract;
- private implementation details;
- explicit allowed dependencies;
- its own CMake target where practical;
- boundary/contract tests.

Rules:
- never include another module's internal/private headers;
- never let QML call provider/SQL/MCP implementation objects directly;
- never let runtime depend on concrete provider/database/platform implementations;
- never read another module's SQLite tables directly;
- never instantiate sibling-module concrete implementations outside the composition root;
- avoid global service locators/singletons as hidden dependency injection;
- use registries for open-ended families such as providers, tools and skills;
- keep optional capability absence representable and safe;
- confine build-time feature switches to CMake/composition/adapter boundaries;
- do not scatter `#ifdef` through domain/runtime logic.

Before adding a dependency:
1. state what problem it solves;
2. explain why Qt/C++ standard library cannot solve it adequately;
3. record which module owns it;
4. record security/packaging impact;
5. record AppImage impact;
6. create/update an ADR for architecture-significant dependencies.

Avoid large runtime stacks.

Qt WebEngine is optional, not baseline. Do not introduce it only to render chat.

There is no public binary plugin ABI yet. Do not add one without a separate ADR and real consumers proving the need.

## 11. Performance rules

Hot paths:
- typing;
- streaming tokens;
- transcript scrolling;
- resizing;
- split-pane movement;
- tool progress;
- terminal output.

Rules:
- batch high-frequency updates where semantics permit;
- never await UI/telemetry/log observation in the provider token-stream critical path;
- do not rebuild the whole transcript for each token;
- use model/view boundaries for long lists;
- do not destroy expensive views only because they are hidden;
- profile realistic long conversations, not empty demos.

## 12. Packaging rules

Primary artifact: AppImage.

Build must:
- create a self-contained AppDir;
- include required Qt libraries/plugins/QML modules;
- retain host integration for things that should remain host-owned;
- avoid accidentally bundling incompatible graphics/Wayland driver stacks;
- run in CI on a deliberately chosen ABI baseline;
- record the resulting glibc/libstdc++ floor.

Later artifacts may include .deb.

## 13. Testing contract

At minimum:
- unit tests for domain/state;
- AgentEngine/TurnStateMachine tests;
- RuntimeOrchestrator event-mapping/lifecycle tests;
- provider transport tests;
- tool-call parsing/execution tests;
- approval policy tests;
- MCP tests;
- context-management tests;
- session persistence tests;
- QML component tests where behavior matters;
- integration tests with MockRuntime;
- integration tests with a deterministic fake OpenAI-compatible server;
- Linux packaging smoke test;
- Wayland smoke test;
- X11 compatibility smoke test.

A feature crossing a boundary requires a test at that boundary.

## 14. Documentation contract

`docs/DOCUMENTATION_POLICY.md` is mandatory.

For every meaningful change:
- update `docs/PROJECT_STATE.md`;
- update architecture docs if a boundary changed;
- update the relevant ADR when a decision improves or changes;
- remove/rewrite superseded ADRs or research notes when they would mislead;
- scan for contradictory references to renamed/obsolete concepts;
- leave explicit next steps when work is incomplete.

The repository documents the best current understanding, not a museum of old decisions.

Git history is the archive. Current files are the truth.

Do not:
- preserve obsolete architecture merely for historical context;
- create parallel "v2" docs while stale "v1" docs remain authoritative-looking;
- leave knowingly wrong technical claims with an "outdated" warning when they can be corrected or removed.

Do not rely on conversation memory as project state.

## 15. AI handoff format

Before ending a development milestone, update `docs/PROJECT_STATE.md` with:

```yaml
milestone:
status:
last_verified_commit:
working:
broken:
decisions:
open_questions:
next_exact_steps:
verification_commands:
```

The next AI should be able to continue from repository state alone.
