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
| session/input/turn/item |   | tray/dbus/portal/etc. |
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
| production facade    |  | deterministic tests  |
+----------+-----------+  +----------------------+
           |
           v
+------------------------------------------------------+
| RuntimeOrchestrator                                  |
| sessions/runs | persistence | approvals | tasks     |
| steering | recovery | context policy | event map    |
+---------------------------+--------------------------+
                            |
                            v
+------------------------------------------------------+
| AgentEngine                                           |
| provider/tool iteration | engine state | cancellation|
+-------------+----------------------+-----------------+
              |                      |
              v                      v
      ProviderRegistry         ToolRegistry/Executor
              |
              +--> Context/RequestProjection
              +--> MCP/Skills/Memory through orchestrator ports
```

The interface exists to preserve clean boundaries and testability, not to make third-party agent runtimes the product.

### Thread ownership

Qt thread affinity is part of the architecture (ADR-0015).

```text
Main/UI thread
  QApplication
  QQmlApplicationEngine
  UI-facing QAbstractListModels/controllers
          |
          | queued commands/events
          v
Runtime thread
  NativeSuprAIRuntime facade
  RuntimeOrchestrator
  AgentEngine / TurnStateMachine
  Provider network objects
  QProcess/QTimer runtime objects
          |
          | queued persistence commands/results
          v
Persistence thread
  PersistenceWorker
  primary SQLite writer connection
```

QML never manipulates runtime-owned network/process/database QObjects directly.

High-frequency text deltas may be coalesced at the UI boundary, but state transitions and terminal events are not dropped.

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

## 5. Repository structure and modularity

SuprAI is a modular monolith (ADR-0029).

Each architecture-significant subsystem is an independently testable CMake target with:
- public contract;
- private implementation;
- explicit dependencies;
- boundary tests.

Current implemented structure:

```text
/
  CMakeLists.txt

  src/
    CMakeLists.txt

    app/
      main.cpp
      AppConfig.h
      AppSettings.*
      ApplicationBootstrap.*

    modules/
      domain/
        include/suprai/domain/
          ConversationItem.h

      providers/
        api/
          include/suprai/providers/
            Provider.h
          src/
            Provider.cpp

        openai/
          include/suprai/providers/
            OpenAIProviderFactory.h
          src/
            OpenAIChatProvider.*
            SseDecoder.*

      runtime/
        include/suprai/runtime/
          AgentRuntime.h
          RuntimeApplicationEvent.h
          RuntimeCapabilities.h
          RuntimeConfig.h
          RuntimeFactory.h
          RuntimeState.h
        src/
          AgentEngine.*
          RuntimeEventAdapter.*
          RuntimeOrchestrator.*
          NativeSuprAIRuntime.*
          MockRuntime.*
          RuntimeFactory.cpp

      persistence/
        include/suprai/persistence/
          PersistenceWorker.h
        src/
          PersistenceWorker.cpp

      platform/
        include/suprai/platform/
        src/

      ui/
        include/suprai/ui/
          ChatController.h
        src/
          ChatController.cpp
          TranscriptModel.*

    qml/
      Main.qml

  tests/
    tst_sse_decoder.cpp
    tst_transcript_model.cpp
    tst_mock_runtime.cpp
    tst_native_runtime.cpp
```

Future modules (tools/context/memory/MCP/persistence/skills/platform/services) are added under the same public-contract/private-implementation rule when their milestone begins.



### 5.1 CMake targets

Current targets:

```text
suprai_domain
suprai_provider_api
suprai_provider_openai
suprai_platform
suprai_persistence
suprai_runtime
suprai_ui
suprai
```

Important dependency enforcement:
- `suprai_runtime` links `suprai_provider_api`, not `suprai_provider_openai`;
- the concrete OpenAI adapter exposes only its factory include directory publicly;
- concrete runtime classes and TranscriptModel remain private implementation headers;
- tests that intentionally inspect an internal helper receive that private include path explicitly and locally;
- `ApplicationBootstrap` is the only current production composition point that chooses Mock vs native and constructs the concrete provider.

Future subsystem and adapter targets are added when implemented. Persistence and platform are already separate targets; tools/context/memory/MCP/skills/services remain future modules.

Do not create one giant library containing all application logic.

Use:
- target-scoped include paths;
- PUBLIC/PRIVATE link visibility;
- no global include directories;
- no cross-module inclusion of implementation headers.

### 5.2 Dependency rule

High-level direction:

```text
QML/UI
   |
   v
Application/controllers
   |
   v
Domain + public service/runtime contracts
   ^
   |
Runtime/orchestration
   |
   +--> provider ports
   +--> tool ports
   +--> context ports
   +--> memory ports
   +--> persistence/repository ports
   +--> MCP ports
   +--> platform-service ports
```

Concrete infrastructure depends on public contracts. Core/domain logic does not depend on infrastructure implementations.

Examples:
- TurnStateMachine does not include OpenAI/NInfer/vLLM implementation headers;
- QML does not call SQL/provider/MCP objects;
- MemoryService does not read conversation tables directly;
- provider modules do not reach into ToolExecutor internals;
- platform-neutral modules do not include Linux implementation headers.

### 5.3 Composition root

Concrete wiring happens only in the application bootstrap:

```text
ApplicationBootstrap
  -> create persistence adapters
  -> create platform services
  -> configure ProviderRegistry
  -> configure ToolRegistry
  -> create ContextManager
  -> create MemoryService
  -> create TaskManager
  -> create AgentEngine
  -> create RuntimeOrchestrator
  -> create NativeSuprAIRuntime facade
  -> expose application-facing controllers/models
```

Modules do not construct arbitrary concrete implementations from sibling modules.

Avoid a global service locator. Dependencies are explicit constructor/factory inputs or narrow references.

### 5.4 Extension families

Open-ended families use registries:

```text
ProviderRegistry
ToolRegistry
SkillRegistry
```

Adding a provider/tool/skill should not require editing central switch statements throughout the codebase.

Provider wire formats, MCP schemas and platform implementation details terminate at their adapter boundary.

### 5.5 Optional modules

Optional capability examples:
- MCP;
- tray;
- global shortcuts;
- secure persistent secrets;
- containment backends;
- PDF;
- voice;
- WebEngine preview.

Absence must be a valid state.

Compile-time feature switches remain at CMake/composition/adapter boundaries. Do not scatter `#ifdef` throughout domain/runtime code.

### 5.6 Data ownership

Each authoritative state family has one owner.

No module reads another module's SQLite tables directly.

Cross-module data access occurs through an explicit repository/query/service interface.

Examples:
- TaskManager owns Task lifecycle;
- MemoryService owns memory semantics;
- SecretStore owns secrets;
- canonical conversation persistence goes through repository ports;
- provider state never becomes canonical session authority.

### 5.7 Public plugin ABI

There is intentionally no public binary plugin ABI yet.

Initial modularity is source-level/interface/configuration modularity.

A future stable dynamic plugin ABI requires its own ADR after real external consumers prove the requirement.

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

It is the production implementation of the public AgentRuntime boundary, but it is not one monolithic object.

### RuntimeOrchestrator

RuntimeOrchestrator owns stateful execution semantics around the engine:
- Session/Input/Turn/Run identity and lineage;
- persistence boundaries;
- queued input and steering;
- approvals and user-action lifecycle;
- TaskManager/subagent coordination;
- recovery and owner-generation fencing;
- ContextManager policy;
- memory/project integration;
- mapping engine events to domain/application events.

### AgentEngine

AgentEngine is the comparatively stateless execution kernel for an active Run:
- execute provider/tool iteration;
- consume normalized inference events;
- emit typed AgentEngineEvent values;
- manage low-level cancellation and iteration/completion limits;
- accept prepared request/context input;
- consume already-authorized/scheduled tool results.

AgentEngine does not own SQLite/session persistence, Task registry, long-lived memory, Linux desktop integration or provider wire objects.

### Event projection

```text
provider wire events
  -> normalized InferenceEvent
  -> AgentEngineEvent
  -> RuntimeEventAdapter
  -> domain/application event
  -> QAbstractListModel / QML
```

Engine events are not UI contracts. Stable identities and terminal semantics survive translation.

Execution-critical interceptors are distinct from non-blocking observers. UI, telemetry and logging never synchronously gate the provider token stream.

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

Current prototype transport:
- streaming Chat Completions at `/v1/chat/completions`.

The current Chat transport is a vertical-slice adapter, not the final provider architecture. Responses remains the preferred planned transport where semantic compatibility is verified.

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

Provider/model capabilities are tri-state: `supported`, `unsupported`, or `unknown`. Missing metadata is never silently converted to unsupported. Capability provenance may come from explicit declarations, verified probes, cached observations or user/config overrides.

Transparent retry is allowed only before observable text, reasoning, media or tool-call output. Once observable generation exists, failure/incomplete state is explicit rather than replayed.

Do not encode agent policy inside provider classes and do not leak provider wire types above `providers/`.

Prototype reasoning behavior:
- if a compatible endpoint emits `reasoning_content`, the adapter exposes it separately;
- NativeSuprAIRuntime currently treats that raw reasoning as ephemeral status only;
- raw reasoning is not appended to the in-memory canonical chat history and is therefore not replayed into the next request;
- this invariant is covered by a fake-server integration test;
- providers that do not expose separated reasoning simply provide no reasoning stream; correctness must not depend on the extension.

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

Every approval is identity-bound to the exact Session/Turn/Run/Task/ToolInvocation/UserAction that owns it. There is no generic "approve current operation" authority.

File/workspace mutation uses a prepared ChangeSet contract:

```text
prepare ChangeSet
 -> validate expected base/version
 -> preview
 -> policy/approval
 -> revalidate base/version
 -> apply exact approved ChangeSet
 -> persist actual result/checkpoint
```

If the base changed after approval, the approved change is stale and must be reprepared/reapproved.

Tool/process output is independently bounded for memory, UI streaming, model projection and persisted log/artifact retention. A parallel-safe declaration is only scheduling input; policy/resource conflicts and scheduler capacity decide actual concurrency.

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
- compaction artifacts.

### Token budgeting

TokenBudgetService follows ADR-0018.

It budgets the final semantic provider request, including tool schemas, template overhead and media.

Effective context-limit precedence:
1. explicit user override;
2. provider/runtime-advertised effective limit;
3. verified cached probe for the exact provider/model configuration;
4. conservative configured fallback.

Current local runtimes expose useful effective limits:
- NInfer: `max_model_len`;
- vLLM: `max_model_len`;
- llama.cpp: effective `n_ctx`.

Counting uses the most exact provider capability:
1. final-request input-token endpoint;
2. provider render/tokenize path;
3. tokenizer count of known rendered prompt;
4. conservative estimate with uncertainty margin.

NInfer and current llama.cpp expose final-request input-token counting for Responses; current llama.cpp also exposes it for Chat Completions. Provider capability probing, not brand assumptions, selects the path.

Budget:

```text
effective_context
  - output_reserve
  - safety_margin
  = maximum_input_budget
```

Provider-side automatic truncation is not normal SuprAI context management.

Provider-reported actual input usage feeds back into budgeting as conservative calibration evidence when an earlier request required estimation. It may tighten uncertain estimates but never expand beyond a verified effective context limit.

### Compaction

Compaction follows ADR-0019.

Canonical conversation history is never replaced by a summary.

A CompactionArtifact records its covered source item IDs/hash, retained boundary, summary, summarizer identity/config version and before/after token counts.

Compaction:
- preserves recent canonical tail items;
- never splits tool-call/result structural pairs;
- reduces old oversized tool output only in the prompt representation;
- validates the generated summary;
- rebuilds and recounts the candidate provider request;
- commits only if the result is valid and meaningfully smaller.

Failed or larger compactions are discarded.

A provider-confirmed overflow triggers one bounded deterministic emergency-recovery path before requiring user intervention. Emergency recovery prefers deterministic removal/projection of derived or oversized context and does not require another successful summarizer call merely to fit the next request.

Large tool results are virtualized instead of injected wholesale:

```text
ToolResult
  +--> full canonical result / LargeResultArtifact
  +--> bounded model projection
         preview + structural metadata + artifact reference
```

Artifact reads are explicit, bounded and scoped to the owning session/project authority. Internal artifact IDs are not ambient filesystem paths.

Optional proactive compaction is budget-relative; there is no universal fixed percentage threshold.

### Prompt caching

Prompt/KV caching is an optional optimization (ADR-0020).

Stable prompt material is kept early when semantics permit, but correctness never depends on cache survival. Provider cache IDs, KV slots and server-side response stores are not session identity.

Requirements:
- deterministic semantic ordering;
- observable exact-vs-estimated token state;
- persistent original history separate from compacted context;
- cache loss/server restart changes performance only.

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
- long-running process controls including Stop / Continue while running;
- native file-diff previews for prepared ChangeSets;
- approvals;
- clarification prompts;
- attachments;
- image input when model supports it;
- stop/cancel;
- retry/branch when runtime semantics are implemented.

Do not expose hidden chain-of-thought. Reasoning status/summary may be shown only when the provider/runtime legitimately exposes such data.

### Transcript renderer

The native transcript follows ADR-0014 (currently proof-gated).

Direction:
- C++ `QAbstractListModel`;
- QML `ListView` with delegate reuse;
- no durable state stored inside recycled delegates;
- coalesced streaming updates;
- safe Markdown rendering with raw HTML/network resource loading disabled;
- external links opened only after explicit user action;
- code blocks as owned native components;
- no Qt WebEngine dependency for ordinary chat.

Qt Markdown support may be reused, but model-generated Markdown must not be able to trigger ambient network fetches.

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

For coding/project workflows, WorkspaceCheckpointService is a planned optional capability with conversation state and workspace filesystem state kept as separate axes. UI may restore conversation only, workspace only, or both.

Checkpoint restore must be transactional/recoverable and must never silently discard newer user Git commits. Untracked files and cleanup/retention are explicit concerns.

A later optional project execution mode may use an isolated Git worktree/branch instead of mutating the user's current working tree. This is not required for non-Git projects.

## 15. Persistence

SQLite is SuprAI's canonical durable state (ADRs 0011, 0012 and 0017).

Conversation history is an append-oriented stream of generalized items, not merely mutable `{role,text}` messages.

Canonical item classes include:
- message;
- reasoning metadata/summary when legitimately exposed;
- tool_call;
- tool_result;
- attachment;
- runtime annotation.

Stable SuprAI IDs exist independently of provider IDs.

Retry, regenerate, edit-and-resend and future branching create lineage; they do not rewrite completed canonical history.

### Persistence worker

The primary SQLite writer connection belongs to a dedicated PersistenceWorker/thread.

Implemented baseline:
- QSQLITE driver availability check;
- database under XDG state path;
- WAL enabled and verified;
- foreign keys enabled;
- 5000 ms busy timeout;
- synchronous=NORMAL;
- PRAGMA user_version migrations;
- startup/test quick_check and foreign_key_check;
- FTS5 capability probe in the actually shipped SQLite;
- schema v1 with sessions, inputs, turns, runs, generalized conversation_items and an FTS5 history table;
- Qt6Sql/QSQLiteDriverPlugin included in portable staging;
- no arbitrary SQL from QML/runtime components.

Still missing:
- asynchronous repository/command ports between RuntimeOrchestrator and PersistenceWorker;
- durable runtime writes/reads and resume path;
- crash reconciliation for active Runs;
- durable ToolInvocation side-effect journal.

### Side-effect journal

Mutating external tool invocations are durable before execution.

```text
prepared -> authorized -> executing -> succeeded|failed|cancelled

crash while executing -> outcome_unknown
```

An `outcome_unknown` mutating/non-idempotent action is never automatically replayed.

Store:
- sessions/turns/items;
- tool invocation journal;
- compaction artifacts;
- projects;
- profiles;
- provider configuration excluding raw secrets;
- tool/MCP configuration;
- memory metadata/content;
- UI associations;
- schema version.

Provider response IDs/cache/session state are optional metadata, never canonical conversation authority.

A newly opened empty chat may hold a transient Session identity without creating durable history. The first accepted Input makes the conversation durable; unsent drafts/attachments before that point are draft-owned state, not canonical history.

LargeResultArtifact metadata and optional workspace-checkpoint metadata are persisted through their owning services rather than stuffed into conversation text.

Secrets live in secure Linux secret storage when available.

## 16. Linux integration

Linux integration is freedesktop-first and capability-driven (ADR-0013).

Baseline services:
- application activation/single-instance/deep links via `org.freedesktop.Application` semantics over QtDBus;
- global shortcuts through XDG Desktop Portal GlobalShortcuts v2;
- notifications through Portal Notification v2 when available, with `org.freedesktop.Notifications` fallback;
- file/open/screenshot interactions through appropriate portals where Wayland/user-consent semantics matter;
- optional tray via C++ `QSystemTrayIcon`, capability-probed;
- secure secret storage through SecretStore; QtKeychain is the current implementation candidate (ADR-0016);
- clipboard and drag/drop through Qt.

The visible interface remains Qt Quick/QML even if `QApplication` + Qt::Widgets are linked for QSystemTrayIcon.

Do not use raw X11 key grabs as the Wayland global-shortcut design.

Tray, notification actions and shortcuts are optional capabilities; application correctness cannot depend on them.

QML consumes a `DesktopCapabilities` model rather than guessing GNOME/KDE/X11 from environment strings.

## 17. Process and thread model

Initial preference: one application process, modular internally.

```text
suprai
  ├─ main/UI thread
  │    └─ Qt Quick/QML + UI models
  ├─ runtime worker thread
  │    └─ NativeSuprAIRuntime
  │         ├─ HTTP provider connections
  │         ├─ optional MCP child processes
  │         └─ tool child processes when required
  └─ persistence worker thread
       └─ SQLite writer
```

A separate SuprAI daemon/gateway may be designed later for remote/headless use, but is not required to make the desktop functional.

This avoids prematurely reproducing another project's client/gateway split while still preventing model/network/database work from blocking the UI thread.

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

The current implemented Qt logging categories are:
- `suprai.app`;
- `suprai.runtime`;
- `suprai.provider`;
- `suprai.persistence`;
- `suprai.platform`.

Add narrower categories only when a subsystem exists and the split improves filtering; do not maintain speculative unused categories.

The default message pattern contains timestamp, severity, category and message. `QT_MESSAGE_PATTERN` remains an explicit user/developer override.

Secrets and sensitive tool arguments/results must be redacted according to policy.

## 20. XDG paths

`AppPaths` uses Qt `QStandardPaths`:
- `AppConfigLocation`;
- `AppDataLocation`;
- `CacheLocation`;
- `StateLocation`.

Startup creates the resolved directories explicitly and fails early if required application paths cannot be created.

Do not hard-code `~/.config`, `~/.local/share`, `~/.cache` or `~/.local/state`; the effective locations follow the user's XDG/Qt environment.

## 21. AppImage

ADR-0010 remains proposed for the final AppImage/ABI decision, but the CMake-owned staging mechanism is now implemented and CI-proven.

Current staging flow:
1. `cmake --install` creates the portable install tree.
2. `qt_deploy_qml_imports()` deploys required QML modules/plugins.
3. `qt_deploy_runtime_dependencies()` closes runtime dependencies and generates `qt.conf`.
4. Qt Widgets is staged explicitly because `QApplication` is a deliberate dependency and the Linux deploy scan did not include it automatically in the proof environment.
5. `Qt6::QWaylandIntegrationPlugin` is an explicit imported runtime artifact; `QWaylandIntegrationPlugin` and `QXcbIntegrationPlugin` are declared on the application target.
6. CI asserts both `plugins/platforms/libqwayland.so` and `libqxcb.so`.
7. The staged tree is launched with the Qt development environment removed under XCB/Xvfb and Wayland/Weston.

This proves staging mechanics and QPA closure. It does not yet prove:
- a final AppImage;
- the Ubuntu 22.04 ABI floor;
- cross-distribution compatibility;
- physical KDE/GNOME compositor behavior.

The eventual flow continues with:
1. add desktop/icon/AppRun metadata;
2. finalize AppImage;
3. inspect GLIBC/GLIBCXX and dependency closure;
4. smoke-test the actual artifact on the declared ABI floor/distribution matrix.

AppImage does not erase glibc/libstdc++ requirements.

Ubuntu 22.04 x86_64 remains a strong candidate build baseline because Qt 6.12 supports it, but this remains proof-gated. Official Qt Linux installer binaries used by current CI are not evidence of an Ubuntu-22.04-compatible ABI floor.

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

## 25. Execution identity model

Advanced execution follows ADR-0021.

```text
Session
  ├─ Inputs
  └─ Turns
      └─ Runs
          ├─ ProviderAttempts
          ├─ ToolInvocations
          └─ Tasks
```

Definitions:
- Session: durable conversation/workspace context;
- Input: durably admitted user/internal input;
- Turn: one logical foreground work episode;
- Run: one executable generation/segment of a Turn;
- ProviderAttempt: one inference request attempt;
- ToolInvocation: one durable tool execution;
- Task: detached/asynchronous/background work.

A Turn may span multiple Runs due to yield/resume or restart recovery.

Input admission is separate from Turn completion. The session may be foreground-idle while background Tasks continue.

## 26. Input queue and steering

Queued input follows ADR-0022.

Semantic modes:
- steer;
- followup;
- collect;
- interrupt.

Steering is consumed only at safe runtime boundaries. It never terminates an already-running tool merely to apply new guidance.

Sequential unstarted tool calls may be skipped when steering lands, but every skipped call receives a synthetic terminal ToolResult so canonical history remains structurally paired.

Track steering custody separately:
- accepted;
- delivered;
- missed;
- rejected;
- converted_to_followup.

An accepted steer is not proof that the model consumed it.

## 27. Subagents and isolated deliberation

Subagents follow ADR-0023. Isolated reasoning/context folding follows ADR-0028 and reuses the same subagent infrastructure.

A subagent is a `Task(source=subagent)` that owns a child Session and executes through NativeSuprAIRuntime.

Subagent purpose may include:

```text
delegation
deliberation
verification
research
coding
```

Purpose is policy/profile metadata, not a separate runtime type.

### Child context

Default child context is isolated and explicit through a TaskBrief.

Context modes:

```text
full
scoped
compacted
```

- `full`: complete parent snapshot when small enough;
- `scoped`: only selected relevant parent/project/evidence state;
- `compacted`: derived compacted snapshot governed by ADR-0019.

Do not implicitly clone the entire parent conversation by default.

### Deliberation subagent

Reasoning-heavy work uses:

```text
Parent Session
  -> Parent Turn
      -> Task(source=subagent, purpose=deliberation)
          -> Child Session
              -> Child Turn/Run
                  -> ReasoningWorkspace
                  -> optional read/search tools
                  -> ReturnCapsule
      -> compact result merged into parent
```

Do not create a standalone `standalone deliberation-branch manager`.

`ReasoningWorkspace` is ephemeral Run-local scratch state. It may contain provider-separated reasoning, temporary plans, hypotheses and provisional conclusions. It is not canonical conversation history and does not itself create context isolation.

`ReturnCapsule` is the compact parent merge boundary. It should distinguish facts, hypotheses, decisions, unresolved work and evidence references rather than returning raw chain-of-thought.

Raw child reasoning is excluded from parent canonical context by default.

A deliberation child is read-mostly by default and must not gain broader side-effect authority merely because it is reasoning deeply.

### Child authority

Child authority is the intersection of parent/requester authority, child profile restrictions, task-specific restrictions and current policy. A child can never widen privileges.

### Model and scheduling

A child may use:
- the same provider/model as the parent;
- the same model with different reasoning effort;
- a different configured model/profile later.

Logical child concurrency does not imply simultaneous GPU generation. ExecutionScheduler chooses physical scheduling according to provider/model/hardware capability.

### Lifecycle

Subagent work may be:
- attached: parent Turn depends on completion;
- detached: child may outlive the spawning Run/Turn.

A deliberation child used to answer the current Turn is normally attached.

Completion is push/event driven. Parent models do not poll child state. A runtime control equivalent to `yieldUntil(tasks)` may suspend a Turn without consuming model tokens and resume it through a new Run generation when required Tasks settle.

### Critical context limitation

Isolation prevents raw reasoning from becoming parent/future-context debt, but it does not make the child's own context window infinite.

For child starting context `P` and child reasoning/tool trace `R`, child peak remains approximately `P + R`.

Therefore full/scoped/compacted context selection and output/reasoning headroom remain required.

## 28. TaskManager and background work

TaskManager follows ADR-0024.

Canonical states:

```text
queued
running
waiting_input
cancel_requested
succeeded
failed
timed_out
cancelled
lost
outcome_unknown
```

Execution status and result-delivery status are separate.

Task sources include:
- subagent;
- process;
- MCP task;
- scheduled run;
- future remote worker.

A long-running foreground ToolInvocation may explicitly hand executor/process ownership to TaskManager ("Continue while running"). The foreground tool returns a bounded partial/incomplete result while the Task continues, logs into bounded artifacts and later emits completion. Loss of UI observation alone is not ownership transfer.

The runtime never spends model turns polling Tasks. TaskManager observes/polls external executors as infrastructure and emits meaningful state transitions.

MCP `io.modelcontextprotocol/tasks` maps into TaskManager but does not define SuprAI's internal task identity.

## 29. Recovery ownership

Restart recovery follows ADR-0025.

Persisted `running` is not proof of liveness.

Active work carries exact ownership/generation metadata. Recovery:
1. verifies the old owner/executor;
2. reconciles external state;
3. creates a new execution generation where safe;
4. fences late events from superseded owners.

Automatic replay is allowed only when semantics make it safe.

Interrupted mutating/non-idempotent effects without a durable outcome become `outcome_unknown` and are never replayed automatically.

Recovery attempts are bounded and can enter a blocked/tombstoned state requiring operator/user review.

## 30. Scheduling

Scheduling follows ADR-0026.

A Schedule is a trigger definition, not a permanently-running Task:

```text
Schedule
  -> Occurrence
  -> Task
  -> Session / Turn / Run
```

Schedules define explicit:
- timezone;
- misfire policy;
- overlap policy;
- capability envelope;
- result-delivery target.

Each occurrence has an idempotency identity.

Scheduled runs execute under current policy; creation-time permissions do not become permanent bypasses.

Fresh isolated task Sessions are the default execution context.

## 31. Linux durable process backend

ADR-0027 proposes an optional systemd transient user-service backend for process Tasks that should survive the GUI process.

This is not accepted baseline behavior until proven.

QProcess remains the baseline process backend.

Task durability must always state the effective class, e.g.:
- run-local;
- app-process;
- externally-supervised;
- remote.

Never claim restart durability unless the actual executor provides it.

## 32. Architecture proof complete when

- Qt shell boots;
- MockRuntime completes a scripted streaming/tool/approval turn;
- NativeSuprAIRuntime delegates real execution through RuntimeOrchestrator -> AgentEngine;
- a real OpenAI-compatible request executes without provider wire objects leaking above providers/;
- streamed response traverses AgentEngineEvent -> domain event -> UI projection without observer backpressure;
- session persists and resumes;
- one real tool round-trip succeeds;
- approval gate succeeds;
- MCP tool round-trip succeeds;
- cancellation works;
- AppImage launches on clean supported environment;
- no third-party agent runtime is required.
