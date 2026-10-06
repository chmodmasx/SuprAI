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
- ProviderRegistry;
- OpenAI-compatible provider;
- SSE/stream parser;
- model configuration;
- local endpoint support;
- AgentLoop;
- SessionStore;
- MessageStore;
- cancellation;
- runtime event conversion.

First real target:
- any compliant local OpenAI-compatible endpoint;
- specifically validate common local servers such as llama.cpp/vLLM/NInfer-compatible endpoints when available.

Exit:
- user prompt -> provider -> streamed assistant response through NativeSuprAIRuntime;
- sessions persist and resume;
- no Hermes/OpenClaw runtime involved.

## M4 — Tool calling + approvals

Implement:
- normalized tool schema;
- ToolRegistry;
- ToolExecutor;
- provider tool-call translation;
- tool lifecycle events;
- approval policy;
- allow/ask/deny decisions;
- cancellation/timeouts;
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
- MCP client;
- stdio transport;
- HTTP/streamable transport as justified by current MCP spec;
- server registry/config;
- capability discovery;
- tool/resource/prompt mapping;
- approval/security boundaries;
- SuprAI skill format.

Research Hermes/OpenClaw here only to compare solved ergonomics and failure modes.

Exit:
- configured MCP tools participate in the same native AgentLoop as built-in tools.

## M6 — Context management + memory

Implement:
- ContextManager;
- token accounting;
- context-window policy;
- transcript compaction/summarization;
- pinned project context;
- MemoryService;
- explicit memory scopes;
- retrieval policy.

Requirements:
- model context size is configurable/discovered;
- no hidden uncontrolled growth;
- summarization never silently becomes canonical history.

Exit:
- long sessions remain usable with deterministic context policy.

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
