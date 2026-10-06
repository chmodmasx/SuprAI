# AGENTS.md — SuprAI AI Engineering Contract

Purpose: make future AI work deterministic, resumable, and architecture-safe.

## 0. Read order

Before changing code:
1. Read this file.
2. Read `docs/ARCHITECTURE.md`.
3. Read `docs/ROADMAP.md`.
4. Read `docs/PROJECT_STATE.md`.
5. Read any ADR or reference document relevant to the change.
6. Inspect the current code. Code wins over stale documentation; if they disagree, update the documentation in the same change.

## 1. Product identity

SuprAI is a Linux-native desktop AI workspace.

It is NOT:
- an Electron application;
- a browser dashboard wrapped in a window;
- a Hermes fork;
- an OpenClaw fork;
- a UI that directly owns agent state;
- a frontend tied permanently to one model provider.

Primary implementation:
- C++20;
- Qt 6;
- Qt Quick/QML;
- CMake/Ninja;
- AppImage first.

## 2. Core invariant: UI != agent

Three authority domains exist.

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
- backend discovery;
- connection lifecycle;
- local filesystem integration;
- desktop integration;
- secret retrieval;
- notification/tray/global shortcut integration;
- compatibility probing;
- backend capability mapping.

### Agent authority
Owned by the active AgentBackend:
- sessions;
- stored conversation history;
- active turns;
- model/tool execution;
- tool results;
- approvals/clarifications;
- persistent agent memory;
- agent profiles;
- skills/tools;
- token/model/provider truth.

Never duplicate agent behavior in QML.

## 3. Backend abstraction

All agent implementations must satisfy a SuprAI-owned interface.

Conceptual interface:

```text
AgentBackend
  connect()
  disconnect()
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

Initial adapters:
- HermesBackend: JSON-RPC/WebSocket gateway.
- MockBackend: deterministic tests.

Future:
- NativeSuprAIBackend.
- OpenClawBackend only if justified.

Hermes-specific JSON or identifiers must stop at the adapter boundary.

## 4. Event model

Backend traffic is converted to SuprAI domain events before reaching UI.

Examples:
- BackendConnected
- BackendDisconnected
- SessionCreated
- SessionUpdated
- MessageAdded
- MessageDelta
- TurnStarted
- TurnFinished
- ToolStarted
- ToolUpdated
- ToolFinished
- UserActionRequired
- CapabilityChanged
- ErrorRaised

Requirements:
- ordered per session;
- stale async responses cannot overwrite newer state;
- terminal events flush immediately;
- reconnect must not silently duplicate turns;
- UI must distinguish loading, reconnecting, degraded, failed, and ready states.

## 5. State rules

Ask: "who is allowed to be correct about this state?"

- Backend truth is cached, not owned by UI.
- Machine truth is owned by application core.
- UI owns only presentation state.
- Persisted state must have explicit scope: global, backend, profile, project, session, or window.
- Never use one global key for data that can differ by backend/profile/project.
- Every optimistic mutation must have rollback behavior.

## 6. Linux-native rules

Wayland is the primary display target.

Must not assume:
- global coordinates are always available;
- X11-only window APIs exist;
- arbitrary global keyboard hooks work without compositor/portal support;
- system tray behaves identically across desktops;
- one Secret Service implementation is present.

Platform capabilities must be probed.

Desktop integrations belong behind `platform/linux` interfaces, never scattered through QML.

## 7. Security rules

- No API keys/tokens/passwords in plaintext config.
- Use a secure desktop secret backend when available.
- Never expose arbitrary shell execution through a generic QML bridge.
- Native capabilities are explicit and narrow.
- Remote backend means tools execute where that backend runs unless a client capability explicitly states otherwise.
- Web/HTML previews are untrusted content.
- No popup/external navigation without an explicit policy.
- No hidden automatic privilege escalation.
- Any sudo/root flow must be explicit to the user.

## 8. Dependency policy

Before adding a dependency:
1. state what problem it solves;
2. explain why Qt/C++ standard library cannot solve it adequately;
3. record security/packaging impact;
4. record AppImage impact;
5. create/update an ADR for architecture-significant dependencies.

Avoid large runtime stacks.

Qt WebEngine is optional, not baseline. Do not introduce it only to render chat.

## 9. Performance rules

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
- do not rebuild the whole transcript for each token;
- use model/view boundaries for long lists;
- do not destroy expensive views only because they are hidden;
- profile realistic long conversations, not empty demos.

## 10. Packaging rules

Primary artifact: AppImage.

Build must:
- create a self-contained AppDir;
- include required Qt libraries/plugins/QML modules;
- retain host integration for things that should remain host-owned;
- avoid accidentally bundling incompatible graphics/Wayland driver stacks;
- run in CI on a deliberately chosen ABI baseline;
- record the resulting glibc/libstdc++ floor.

Later artifacts may include .deb.

## 11. Testing contract

At minimum:
- unit tests for domain/state reducers;
- JSON-RPC transport tests;
- backend adapter contract tests;
- reconnect/order/idempotency tests;
- QML component tests where behavior matters;
- integration tests with MockBackend;
- Linux packaging smoke test;
- Wayland smoke test;
- X11 compatibility smoke test.

A feature crossing a boundary requires a test at that boundary.

## 12. Documentation contract

For every meaningful change:
- update `docs/PROJECT_STATE.md`;
- update architecture docs if a boundary changed;
- record significant decisions in an ADR;
- leave explicit next steps when work is incomplete.

Do not rely on conversation memory as project state.

## 13. AI handoff format

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
