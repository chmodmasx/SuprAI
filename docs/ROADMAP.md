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
- architecture has explicit UI/core/backend/platform boundaries;
- first backend strategy selected;
- first packaging strategy selected.

Status: IN PROGRESS.

## M1 — Native shell skeleton

Goal: prove the Qt/AppImage foundation before agent complexity.

Implement:
- CMake project;
- C++20 executable;
- Qt Quick/QML application;
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
- QML loads without runtime warnings considered fatal by project policy;
- clean shutdown.

Exit:
- native shell runs without Node/Electron/Python;
- architecture structure exists in source tree.

## M2 — Domain model + MockBackend

Goal: prove the agent UI without relying on any external agent.

Implement:
- AgentBackend abstract interface;
- BackendCapabilities;
- session/message/tool/request domain types;
- event dispatcher;
- MockBackend scripted fixture;
- transcript model;
- composer;
- streaming delta path;
- tool cards;
- approval/clarification component;
- cancellation;
- reconnect/error state UI.

Tests:
- deterministic streaming;
- out-of-order/stale update rejection;
- tool lifecycle;
- user action request lifecycle;
- cancellation;
- backend disconnect/reconnect.

Exit:
- a complete fake agent turn can be exercised from UI.

## M3 — Hermes protocol adapter

Goal: make SuprAI useful with an existing mature agent runtime.

Research before implementation:
- current Hermes gateway startup contract;
- auth contract;
- WebSocket endpoint;
- JSON-RPC method/event catalog;
- session identities;
- prompt submission;
- cancellation;
- approvals;
- profiles/models/tools/skills;
- compatibility/version probing.

Implement:
- HermesRpcClient;
- request ID management;
- bidirectional RPC;
- HermesMapper;
- HermesBackend;
- local existing-gateway connection;
- remote gateway connection;
- capability probe;
- version compatibility policy.

Then:
- optional managed local Hermes process through QProcess.

Exit:
- connect;
- list/resume sessions;
- create session;
- submit prompt;
- stream reply;
- show tool calls;
- answer approval/clarification;
- cancel turn;
- reconnect without duplicating a turn.

## M4 — Projects + files

Goal: make the desktop useful as a work environment.

Implement:
- Project model;
- project directories;
- session/project association;
- file tree;
- file preview for safe native formats;
- drag/drop attachments;
- backend/local filesystem distinction;
- git repository recognition.

Do NOT add Qt WebEngine merely for this milestone.

Exit:
- local project flow works;
- remote backend cannot accidentally present remote paths as local paths.

## M5 — Linux desktop integration

Implement:
- tray/status notifier;
- notifications;
- desktop entry/icons;
- deep link;
- secure secret storage;
- portal-aware file operations;
- optional global shortcut capability;
- optional autostart;
- polished close-to-tray behavior.

Validate:
- KDE Plasma Wayland;
- GNOME Wayland;
- X11 fallback.

Exit:
- feature availability is probed, not assumed.

## M6 — AppImage release pipeline

Implement:
- release CMake install layout;
- AppDir generation;
- Qt/QML deployment;
- AppImage generation;
- CI artifact;
- clean VM smoke test;
- dependency report;
- SBOM if tooling is practical;
- release checksum.

Baseline:
- choose a deliberately old-enough Linux build image;
- record glibc and GLIBCXX requirements from built artifact.

Exit:
- downloaded AppImage launches on supported clean systems.

## M7 — Voice and rich previews

Voice:
- microphone capture;
- backend capability mapping;
- STT/TTS interface;
- interruption/cancel semantics.

Preview:
- PDF/image/text first;
- evaluate Qt WebEngine separately for browser/artifact previews.

Qt WebEngine requires ADR due to package/security cost.

## M8 — Profiles, skills, advanced agent UX

Implement if backend supports:
- profiles/agents;
- models/providers;
- skills;
- tool enable/disable;
- session branching;
- background jobs;
- richer inspector;
- terminal surface.

Do not fake unsupported capability in UI.

## M9 — Native SuprAI agent runtime

Only start after desktop/backend contract is stable.

First native runtime scope:
- OpenAI-compatible provider;
- local endpoint support;
- streaming;
- tool registry;
- approval policy;
- MCP;
- SQLite sessions;
- context handling.

Later:
- memory;
- skills;
- subagents;
- background jobs;
- routine/automation model.

Exit:
- NativeSuprAIBackend satisfies the same backend contract used by HermesBackend.

## Priority rule

If forced to choose:
1. correctness of backend/session lifecycle;
2. recoverability;
3. native Linux behavior;
4. interaction quality;
5. visual polish;
6. feature count.

## Explicitly deferred

- Windows/macOS;
- mobile;
- custom model inference engine;
- custom browser engine;
- plugin ABI before two real consumers need it;
- full Hermes feature parity before core lifecycle is stable.
