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

### Agent turn state machine

Turn execution is an explicit state machine (ADR-0004), not one recursive `while(tool_calls)` function.

Baseline shape:

```text
Created
  -> PreparingContext
  -> RequestingModel
  -> StreamingModel
  -> EvaluatingToolCalls
       -> AwaitingApproval
       -> SchedulingTools
       -> ExecutingTools
       -> RecordingToolResults
       -> PreparingContext
  -> Finalizing
  -> Completed

active state -> Cancelling -> Cancelled
active state -> Failed
```

Rules:
- transitions/reducers do not perform external I/O;
- effects perform provider/tool/persistence I/O;
- turn, provider-attempt and tool-invocation IDs are stable;
- stale events from superseded attempts are ignored;
- terminal transitions are idempotent;
- persistence checkpoints bracket external side effects;
- retries classify errors instead of blindly resending;
- maximum iteration/turn limits are explicit.

Parallel tool calls are scheduled concurrently only when tool metadata, policy and resource scopes make that safe.

## 8. Providers

Providers supply model inference only.

Initial provider family:
- OpenAI-compatible HTTP.

Internal runtime semantics are NOT Chat Completions or Responses wire objects.

SuprAI owns normalized `InferenceRequest` / `InferenceEvent` types (ADR-0003).

Initial transports:
- `OpenAIResponsesTransport` — preferred when supported;
- `OpenAIChatCompletionsTransport` — compatibility.

This covers current local servers including llama.cpp, vLLM and NInfer-compatible endpoints, plus compatible remote APIs.

Auto-selection may try Responses first, but fallback to Chat only when endpoint non-support is established before observable generation. Do not use arbitrary HTTP 400 as a fallback signal.

Provider interface normalizes:
- models/capabilities;
- typed streamed output;
- tool definitions/calls;
- usage/token accounting;
- reasoning metadata where legitimately exposed;
- multimodal inputs;
- finish/incomplete/failure semantics.

Do not encode agent policy inside provider classes and do not leak provider wire types above `providers/`.

## 9. Tool system

Core types:
- ToolDefinition;
- ToolInvocation;
- ToolResult;
- ToolPolicy;
- ToolExecutionContext.

Canonical input/output schemas use JSON Schema 2020-12.

Execution classes:
- ToolRegistry;
- ToolExecutor;
- PolicyEngine / ApprovalManager;
- ContainmentBackend.

Policy outcomes:
- allow;
- ask;
- deny.

Authorization and containment are different layers (ADR-0005). Approval does not make a host process sandboxed.

Containment is feature-probed:
- Landlock where supported;
- bubblewrap where available and policy permits;
- none as an explicit fallback.

Built-in tools must be narrow. Avoid one generic unrestricted shell bridge as the foundation.

Process execution can exist under explicit policy, resource scopes, timeouts, output limits and descendant-cleanup behavior.

## 10. MCP

MCP extends the same ToolRegistry/agent loop.

MCP is not a second agent architecture.

Canonical protocol era: MCP `2026-07-28` (ADR-0006).

Design assumptions:
- modern stateless core;
- Multi Round-Trip Requests where needed;
- JSON Schema 2020-12;
- extension-aware capabilities.

Do not design new SuprAI behavior around deprecated roots, server sampling or MCP protocol logging.

Because the current official SDK matrix has no C++ SDK, the initial plan is a deliberately small SuprAI-owned MCP client:
- QProcess stdio transport;
- QtNetwork Streamable HTTP;
- protocol-era adapter;
- conformance fixtures.

MCPClientManager responsibilities:
- configured server lifecycle;
- capability/version handling;
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

## 12. Memory and skills

Memory v1 follows ADR-0008.

Separate:
1. canonical SQLite conversation history;
2. SQLite FTS5 searchable history;
3. bounded curated active memory with explicit scope/provenance/trust;
4. semantic/vector retrieval later if measurements justify it.

Candidate active-memory scopes:
- user;
- profile/agent;
- project.

Conversation persistence is not memory. Summaries never replace canonical history. Agent-generated memory cannot elevate its own trust by writing metadata into recalled prose.

Skills follow the Agent Skills `SKILL.md` standard (ADR-0007) with progressive disclosure. Project default location is `.agents/skills/`; user skills live under the SuprAI XDG data directory. Skill metadata never bypasses PolicyEngine.

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

Proposed build flow is tracked in ADR-0010:
1. CMake install into AppDir.
2. use Qt CMake/QML deployment APIs to stage executable, Qt runtime, plugins and QML imports;
3. verify QPA/Wayland/QML/SQLite/image-plugin closure;
4. add desktop/icon/AppRun metadata;
5. finalize AppImage;
6. smoke-test the actual artifact on the declared ABI floor.

AppImage does not erase glibc/libstdc++ requirements.

Ubuntu 22.04 x86_64 is a strong candidate build baseline because Qt 6.12 supports it, but this remains proof-gated. Official Qt Linux installer binaries are built on Ubuntu 24.04/glibc 2.39 and therefore cannot simply be assumed suitable for an older runtime floor.

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
