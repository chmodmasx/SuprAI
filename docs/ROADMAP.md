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
- QML loads cleanly;
- clean shutdown.

Exit:
- native shell runs without Node/Electron/Python;
- architecture source structure exists.

## M2 — Domain model + MockRuntime

Goal: prove the complete agent-facing UI contract independently from model/network behavior.

Implement:
- AgentRuntime abstract interface;
- runtime capabilities;
- session/message/tool/request domain types;
- runtime event bus;
- MockRuntime scripted fixture;
- transcript model;
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
- explicit turn state machine;
- provider-neutral InferenceRequest/InferenceEvent types;
- ProviderRegistry;
- OpenAI Responses-compatible transport;
- Chat Completions compatibility transport;
- SSE/stream parsers;
- provider capability resolution;
- model configuration;
- local endpoint support;
- SessionStore;
- MessageStore;
- cancellation;
- runtime event conversion.

First real targets:
- llama.cpp;
- vLLM;
- NInfer;
- then compatible remote APIs.

Responses is preferred where supported; fallback to Chat must not hide arbitrary request/configuration errors.

Exit:
- user prompt -> provider -> streamed assistant response through NativeSuprAIRuntime;
- sessions persist and resume;
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
- token accounting;
- context-window policy;
- transcript compaction/summarization;
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
- tray/status notifier;
- notifications;
- desktop entry/icons;
- deep link;
- secure secret storage;
- portal-aware file operations;
- optional global shortcut;
- optional autostart;
- polished close-to-tray behavior.

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

## M10 — Advanced agent UX

Candidates:
- multiple profiles/agents;
- multiple providers/models;
- skills manager;
- tool permissions UI;
- session branching;
- background jobs;
- terminal;
- artifacts;
- voice/STT/TTS;
- PDF/image/browser previews;
- subagents.

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
- public plugin ABI before real consumers prove its shape;
- compatibility adapters for Hermes/OpenClaw.
