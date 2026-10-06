# SuprAI Architecture v0

Status: accepted baseline.

## 1. Product

SuprAI is a complete Linux-native AI desktop application.

It owns:
- desktop UI;
- application core;
- agent runtime;
- session persistence;
- provider integration;
- tool execution;
- approvals;
- MCP;
- context management;
- memory;
- Linux integration.

Hermes Agent, Hermes Desktop, OpenClaw and similar projects are research references only. They are not runtime dependencies and are not planned production backends.

## 2. Architectural shape

```text
+------------------------------------------------------+
|                    QML Presentation                  |
| chat | sessions | projects | files | settings | UI |
+---------------------------+--------------------------+
                            |
                            v
+------------------------------------------------------+
|                C++ Application Layer                 |
| navigation | commands | settings | platform glue   |
+-------------+----------------------+-----------------+
              |                      |
              v                      v
+-------------------------+   +------------------------+
| SuprAI Domain Models    |   | Linux Platform Layer   |
| session/message/tool/...|   | tray/dbus/portal/etc. |
+-------------+-----------+   +------------------------+
              |
              v
+------------------------------------------------------+
|                 AgentRuntime interface               |
+---------------------+--------------------------------+
                      |
          +-----------+-----------+
          |                       |
          v                       v
+----------------------+  +----------------------+
| NativeSuprAIRuntime  |  | MockRuntime          |
| production           |  | deterministic tests  |
+----------+-----------+  +----------------------+
           |
           +--> AgentLoop
           +--> ProviderRegistry
           +--> ContextManager
           +--> ToolRegistry / ToolExecutor
           +--> ApprovalManager
           +--> MCPClientManager
           +--> SkillRegistry
           +--> MemoryService
           +--> SessionStore / MessageStore
```

The interface exists to preserve clean boundaries and testability, not to make third-party agent runtimes the product.

## 3. Why Qt Quick/QML

Use QML for presentation and C++ for application/runtime logic.

Reasons:
- native Linux process/window integration;
- strong Wayland support through Qt platform plugins;
- GPU-accelerated scene graph;
- declarative UI appropriate for streaming/dynamic agent surfaces;
- direct C++ integration with networking, D-Bus, filesystem, processes and desktop services;
- no Chromium runtime required for core chat;
- CMake/AppImage-friendly.

Baseline Qt modules:
- Qt::Core
- Qt::Gui
- Qt::Qml
- Qt::Quick
- Qt::QuickControls2
- Qt::Network
- Qt::Sql
- Qt::DBus
- Qt::Concurrent
- Qt::Svg

Optional:
- Qt::WebSockets
- Qt::Multimedia
- Qt::Pdf
- Qt::WebEngineQuick

Qt WebEngine is not baseline.

## 4. C++ standard

C++20 initially.

Raise only when a concrete feature justifies a newer compiler/toolchain floor.

## 5. Repository structure

Proposed:

```text
/
  AGENTS.md
  CMakeLists.txt
  cmake/
  src/
    app/
      main.cpp
      Application.*
    domain/
      Session.*
      Message.*
      ToolCall.*
      AgentEvent.*
      RuntimeCapabilities.*
    runtime/
      AgentRuntime.*
      native/
        NativeSuprAIRuntime.*
        AgentLoop.*
        RuntimeEventBus.*
      mock/
        MockRuntime.*
    providers/
      Provider.*
      ProviderRegistry.*
      openai/
        OpenAICompatibleProvider.*
        SSEParser.*
    tools/
      Tool.*
      ToolRegistry.*
      ToolExecutor.*
      ApprovalManager.*
      builtin/
    mcp/
      MCPClientManager.*
      transports/
    context/
      ContextManager.*
      TokenCounter.*
      CompactionPolicy.*
    memory/
      MemoryService.*
    skills/
      SkillRegistry.*
    services/
      SessionService.*
      ProjectService.*
      SettingsService.*
      SecretService.*
    persistence/
      AppDatabase.*
      SessionStore.*
      MessageStore.*
      migrations/
    platform/
      linux/
        LinuxDesktopIntegration.*
        LinuxNotifications.*
        LinuxTray.*
        LinuxPortal.*
        LinuxSecretStore.*
    qml/
      Main.qml
      shell/
      chat/
      sessions/
      projects/
      files/
      settings/
      components/
      theme/
  tests/
    unit/
    runtime/
    provider/
    tools/
    mcp/
    integration/
  packaging/
    appimage/
    desktop/
  docs/
    ARCHITECTURE.md
    ROADMAP.md
    PROJECT_STATE.md
    REFERENCES.md
    adr/
```

## 6. AgentRuntime boundary

The UI talks to SuprAI domain/runtime APIs, never directly to providers.

Conceptual API:

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

Production implementation:
- NativeSuprAIRuntime.

Test implementation:
- MockRuntime.

Provider-specific request/response types must stop at the provider boundary.

## 7. NativeSuprAIRuntime

NativeSuprAIRuntime is part of the initial product, not a future replacement.

Responsibilities:
- own active runtime state;
- execute the agent loop;
- build context;
- call providers;
- normalize streaming;
- parse/dispatch tool calls;
- request approvals;
- execute tools;
- call MCP servers;
- persist sessions/messages;
- publish domain events;
- manage cancellation;
- coordinate memory/skills.

### Agent loop

Conceptual flow:

```text
user prompt
   ↓
persist message
   ↓
build context
   ↓
provider request
   ↓
stream assistant output
   ↓
tool calls?
   ├─ no  -> finalize turn
   └─ yes
        ↓
   policy / approval
        ↓
   execute tool(s)
        ↓
   persist result
        ↓
   next provider turn
```

The loop must have:
- explicit maximum iteration policy;
- cancellation;
- timeouts;
- observable tool lifecycle;
- deterministic persistence boundaries;
- protection against stale concurrent turn updates.

## 8. Providers

Providers supply model inference only.

Initial provider:
- OpenAI-compatible HTTP API.

This allows SuprAI to work with:
- local llama.cpp servers;
- vLLM;
- NInfer-compatible OpenAI endpoints;
- compatible remote APIs.

Provider interface should normalize:
- models/capabilities;
- chat/responses request;
- streaming;
- tool definitions;
- tool calls;
- usage/token accounting;
- reasoning metadata where exposed;
- image/multimodal inputs when supported.

Do not encode agent policy inside provider classes.

## 9. Tool system

Core types:
- ToolDefinition;
- ToolInvocation;
- ToolResult;
- ToolPolicy;
- ToolExecutionContext.

Execution classes:
- ToolRegistry;
- ToolExecutor;
- ApprovalManager.

Policy outcomes:
- allow;
- ask;
- deny.

Built-in tools must be narrow. Avoid one generic unrestricted shell bridge as the foundation.

Process execution can exist, but under an explicit tool/policy boundary.

## 10. MCP

MCP extends the same ToolRegistry/agent loop.

MCP is not a second agent architecture.

MCPClientManager responsibilities:
- configured server lifecycle;
- capability discovery;
- tool/resource/prompt mapping;
- transport;
- timeout/failure handling;
- security scope.

Server-provided capabilities are untrusted until configured/approved.

## 11. Context management

ContextManager owns what enters each model request.

Inputs may include:
- system/runtime instructions;
- project instructions;
- session history;
- tool definitions;
- memory retrieval;
- attachments;
- compacted summaries.

Requirements:
- explicit context budget;
- provider/model-aware token limits;
- deterministic ordering;
- observable compaction;
- persistent original history remains separate from temporary compacted context.

## 12. Memory

Memory must be scoped and explicit.

Candidate scopes:
- global;
- profile/agent;
- project;
- session.

Do not conflate conversation persistence with memory retrieval.

MemoryService comes after core turn/tool correctness.

## 13. UI information architecture

Initial shell:

```text
+--------------------------------------------------------------+
| project            model/provider                    status  |
+--------------+--------------------------------+--------------+
| left rail    | main chat                      | inspector     |
|              |                                |              |
| new chat     | transcript                     | tool details |
| sessions     |                                | files        |
| projects     |                                | preview      |
| agents       |                                | artifacts    |
| skills       | composer                       | terminal     |
+--------------+--------------------------------+--------------+
```

Chat supports:
- Markdown;
- code;
- streaming;
- tool cards;
- approvals;
- clarification prompts;
- attachments;
- image input when model supports it;
- stop/cancel;
- retry/branch when runtime semantics are implemented.

Do not expose hidden chain-of-thought. Reasoning status/summary may be shown only when the provider/runtime legitimately exposes such data.

## 14. Projects

Project is a SuprAI-owned workspace.

May contain:
- name;
- directories;
- repositories;
- preferred profile/provider/model;
- sessions;
- project instructions;
- layout metadata.

A session may exist without a project.

## 15. Persistence

Use SQLite for SuprAI durable state.

Store:
- sessions;
- messages;
- projects;
- profiles;
- provider configuration excluding raw secrets;
- tool/MCP configuration;
- memory metadata/content where appropriate;
- UI associations;
- schema version.

Secrets live in secure Linux secret storage when available.

## 16. Linux integration

Desired:
- system tray/status notifier;
- notifications;
- single instance;
- desktop file;
- deep links;
- file chooser;
- clipboard;
- drag/drop;
- portals;
- optional global shortcut;
- optional autostart.

Wayland first.

Capabilities are probed, not assumed.

## 17. Process model

Initial preference: one application process, modular internally.

```text
suprai
  ├─ Qt/QML UI
  ├─ application core
  └─ NativeSuprAIRuntime
       ├─ HTTP provider connections
       ├─ optional MCP child processes
       └─ tool child processes when required
```

A separate SuprAI daemon/gateway may be designed later for remote/headless use, but is not required to make the desktop functional.

This avoids prematurely reproducing Hermes' client/gateway split when SuprAI's first target is one native Linux app.

## 18. Runtime state

Representative states:

```text
Stopped
Starting
Ready
Working
WaitingForUser
Cancelling
Degraded
Failed
```

Provider connectivity is separate from runtime state.

Do not compress every failure into "offline".

## 19. Logging

Categories:
- app.lifecycle
- runtime
- provider
- agent.loop
- tools
- mcp
- context
- memory
- session
- platform
- persistence
- packaging

Secrets and sensitive tool arguments/results must be redacted according to policy.

Use XDG state locations.

## 20. XDG paths

Suggested:
- config: `$XDG_CONFIG_HOME/suprai/`
- data: `$XDG_DATA_HOME/suprai/`
- cache: `$XDG_CACHE_HOME/suprai/`
- state/logs: `$XDG_STATE_HOME/suprai/`

## 21. AppImage

Build flow:
1. CMake install into AppDir.
2. deploy executable and required Qt runtime/plugins/QML imports.
3. add desktop/icon/AppRun metadata.
4. build AppImage.
5. smoke-test actual artifact on declared ABI floor.

Do not blindly bundle host graphics/Wayland driver stacks.

## 22. Compatibility

Initial:
- Linux x86_64;
- KDE Plasma Wayland;
- GNOME Wayland;
- X11 fallback;
- distributions meeting declared AppImage ABI floor.

arm64 later.

## 23. Security boundaries

Trust boundaries:
1. user;
2. QML presentation;
3. C++ application;
4. NativeSuprAIRuntime;
5. local tools/processes;
6. MCP servers;
7. model providers;
8. untrusted preview content.

Rules:
- no generic privileged QML escape hatch;
- no hidden privilege escalation;
- dangerous tools require visible policy;
- provider output never grants authority by itself;
- MCP/model content is data, not trusted instruction outside agent policy;
- secret requests/actions are attributable to a session/tool.

## 24. Upstream research policy

Allowed:
- inspect Hermes/OpenClaw/other agent source;
- document patterns;
- compare behavior;
- learn failure modes;
- adapt general architectural ideas where licensing permits.

Not planned:
- launch Hermes as SuprAI's agent;
- launch OpenClaw as SuprAI's agent;
- depend on their gateway protocols;
- define SuprAI domain semantics from their private implementation details.

## 25. Architecture proof complete when

- Qt shell boots;
- MockRuntime completes a scripted streaming/tool/approval turn;
- NativeSuprAIRuntime sends a real request to an OpenAI-compatible model;
- streamed response appears through the domain event model;
- session persists and resumes;
- one real tool round-trip succeeds;
- approval gate succeeds;
- MCP tool round-trip succeeds;
- cancellation works;
- AppImage launches on clean supported environment;
- no third-party agent runtime is required.
