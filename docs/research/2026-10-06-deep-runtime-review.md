# Deep Runtime Architecture Review — 2026-10-06

Status: research baseline for M1-M6.
Purpose: preserve evidence and conclusions from upstream/spec research. Optimized for future AI continuation.

## Executive conclusions

1. SuprAI should own a normalized inference model. OpenAI wire formats must terminate at provider transports.
2. Prefer OpenAI Responses-compatible transport when available; keep Chat Completions as compatibility transport.
3. Agent execution should be an explicit state machine, not a recursive or monolithic tool loop.
4. Tool authorization and OS containment are separate layers. Approval alone is not sandboxing.
5. Canonical tool schemas should use JSON Schema 2020-12.
6. MCP implementation must target the current final protocol revision 2026-07-28, not copy older stateful assumptions.
7. There is no official C++ MCP SDK in the current official support matrix. Start with a deliberately small SuprAI C++ client unless an audited SDK becomes clearly superior.
8. Skills should implement the Agent Skills SKILL.md specification instead of inventing a SuprAI-only format.
9. Memory v1 should separate immutable history, searchable history, and bounded curated memory. Semantic vectors are not required for v1.
10. Qt 6.12 is the current LTS line, but immediate access to later LTS patches is a commercial-LTS distinction. Do not incorrectly promise five years of open-source binary patches.
11. Qt's own CMake/QML deployment APIs should stage the AppDir. AppImage tooling should package/finalize that staged tree rather than become the source of truth for Qt dependency discovery.
12. Build/test the portable artifact against a deliberate ABI baseline; do not assume "AppImage" means distro-independent.

## 1. Reference systems inspected

### Hermes Agent / Desktop
Use for:
- authority/state boundaries;
- bounded curated memory;
- SQLite + FTS5 conversation search;
- explicit approval/user-action flows;
- project/session semantics.

Do not use as runtime dependency.

### OpenClaw
Use for:
- Linux companion lifecycle lessons;
- trust-boundary separation;
- packaging regression tests;
- memory provenance concepts.

Do not use as runtime dependency.

### Goose
Repository:
- https://github.com/aaif-goose/goose

Relevant current source structure includes:
- crates/goose/src/agents/state_machine/
- crates/goose/src/context_mgmt/
- crates/goose/src/execution/
- crates/goose/src/agents/mcp_client.rs
- crates/goose/src/action_required_manager.rs

Important observation:
Goose has evolved into an explicit operation/state-machine architecture. Current operations include LLM invocation, tool calling, tool approval, retry, compaction, maximum-turn handling, steering, subagent execution and skills.

Lesson:
The complexity is real. SuprAI should model turn execution as state transitions/effects from the beginning instead of allowing one AgentLoop method to become a god function.

### GPT4All
Repository:
- https://github.com/nomic-ai/gpt4all

Relevant native desktop code:
- gpt4all-chat/src/chatllm.*
- gpt4all-chat/src/chatmodel.*
- gpt4all-chat/src/tool.*
- gpt4all-chat/src/toolcallparser.*
- gpt4all-chat/src/database.*
- gpt4all-chat/CMakeLists.txt

Observation:
It is a concrete production precedent for Qt/QML/C++ AI desktop software.

Lesson to adopt:
- Qt/QML is technically viable;
- QThread/atomic cancellation patterns are proven useful for model work;
- CMake/QML native desktop packaging is practical.

Lesson to avoid:
Do not allow one ChatLLM-like class to own inference, conversation semantics, persistence, context, tools and UI-facing state simultaneously.

### OpenCode
Current permission documentation:
- https://opencode.ai/v2/docs/permissions

Useful pattern:
- allow / ask / deny;
- resource-pattern scopes;
- special external-directory boundary.

Critical warning:
Its shell documentation correctly notes that shell execution retains host-user filesystem, process and network authority. SuprAI must not call an approved shell execution "sandboxed" unless containment was actually applied.

## 2. Inference transport

### Current local ecosystem

llama.cpp:
- OpenAI-compatible Chat Completions;
- OpenAI-compatible Responses route;
- function/tool calling;
- multimodal;
- SSE.
Source:
https://github.com/ggml-org/llama.cpp/blob/master/tools/server/README.md

vLLM:
- Responses API implementation and typed streaming infrastructure;
- tool/reasoning parsers.
Source:
https://docs.vllm.ai/en/stable/api/vllm/entrypoints/openai/responses/

NInfer:
- /v1/chat/completions;
- /v1/responses;
- typed Response Items and semantic SSE events;
- function calls;
- separate reasoning;
- local response continuation/storage;
- explicit unsupported-field errors.
Source:
https://github.com/Neroued/ninfer/blob/master/docs/serving.md

### Decision shape

Internal API:

```text
InferenceRequest
  model
  instructions
  input items
  tools
  tool policy hints
  reasoning options
  sampling options
  max output
  attachments

InferenceEvent
  ResponseStarted
  OutputItemStarted
  TextDelta
  ReasoningDelta
  ToolCallDelta
  ToolCallCompleted
  UsageUpdated
  ResponseCompleted
  ResponseIncomplete
  ResponseFailed
```

Provider transports map wire protocol <-> these types.

Initial transports:
- OpenAIResponsesTransport (preferred)
- OpenAIChatCompletionsTransport (compatibility)

Do not expose OpenAI SDK/wire structs above provider/.

### Endpoint selection

Configuration modes:
- responses
- chat_completions
- auto

Auto policy:
1. prefer Responses;
2. fallback to Chat only when endpoint absence/incompatibility is proven before observable generation;
3. do not fallback on arbitrary HTTP 400;
4. never retry a request in a way that can duplicate side effects/tool turns;
5. cache the resolved endpoint capability per provider configuration, with manual override.

## 3. Turn state machine

Recommended initial state model:

```text
Created
  -> PreparingContext
  -> RequestingModel
  -> StreamingModel
  -> ModelOutputComplete
       -> Finalizing
       OR
       -> EvaluatingToolCalls
            -> AwaitingApproval
            -> SchedulingTools
            -> ExecutingTools
            -> RecordingToolResults
            -> PreparingContext
  -> Completed

Any active state:
  -> Cancelling -> Cancelled
  -> Failed
```

Principles:
- a transition is driven by an event and may produce effects;
- effects do I/O; state reducers do not;
- each turn has a stable ID;
- each model request has an attempt ID;
- tool invocation IDs are durable;
- terminal states are idempotent;
- stale events from old attempts are ignored;
- persistence checkpoints occur before external side effects and after their results;
- retries classify transport/transient/provider/model errors instead of blindly resending.

### Parallel tool calls

Model declaration of parallel calls does not imply unrestricted concurrent execution.

Tool metadata should declare:
- parallel_safe;
- read/write path scopes;
- network requirement;
- process requirement;
- mutating;
- idempotent.

Scheduler may parallelize only when policy and resource scopes do not conflict.

## 4. Tool schemas

Canonical schema dialect:
- JSON Schema 2020-12.

Reason:
MCP 2026-07-28 explicitly moved tool input/output schemas to full JSON Schema 2020-12.

SuprAI ToolDefinition should contain:
- stable ID;
- name;
- description;
- input schema;
- optional output schema;
- origin;
- risk metadata;
- execution requirements.

Provider adapters may downgrade/transform schemas for weaker model APIs, but the canonical definition remains richer.

Schema validation:
- validate tool arguments before policy/execution;
- bound schema depth and validation time;
- do not automatically dereference arbitrary external $ref URIs.

## 5. Tool authorization vs containment

These are independent.

### Layer A — PolicyEngine
Always present.

Decision:
- allow
- ask
- deny

Scope dimensions:
- tool;
- origin (builtin/MCP/skill);
- project;
- path;
- command family;
- host/network target;
- session;
- one-shot vs persisted permission.

Suggested risk labels:
- read_only
- local_mutation
- process_execution
- network_access
- credential_access
- privilege_escalation
- destructive

### Layer B — ContainmentBackend
Capability-based.

Candidate implementations:
- Landlock: filesystem/self-restriction where kernel supports it;
- bubblewrap: stronger namespace/mount/network process sandbox where available and permitted;
- none: explicit fallback.

Never represent `none` as sandboxed.

bubblewrap caveat:
It is a low-level sandbox constructor, not a security policy. Ubuntu 24.04+ also restricts unprivileged user namespaces through AppArmor, so portable AppImage execution cannot assume bwrap works everywhere.

Landlock caveat:
It can reduce rights for unprivileged processes and composes with existing LSMs, but feature availability depends on kernel/ABI. Probe it.

Initial tool execution should:
- clear/minimize environment;
- explicitly pass required environment variables;
- create a new process group/session;
- enforce timeout;
- cap output;
- kill descendants on cancellation where possible;
- default filesystem visibility to project/workspace scope;
- treat network access as separate authority.

## 6. MCP

Target final revision:
- 2026-07-28.

Primary sources:
- https://blog.modelcontextprotocol.io/posts/2026-07-28/
- https://plan.modelcontextprotocol.io/matrix

Important 2026 changes:
- stateless protocol core;
- modern request routing headers;
- MRTR for interactions requiring more client input;
- Tasks as extension;
- roots, sampling and protocol logging deprecated;
- ping removed in modern era;
- full JSON Schema 2020-12 for tool schemas;
- formal feature lifecycle/deprecation.

Do NOT base new architecture on:
- client roots as a core workspace contract;
- server-initiated sampling;
- MCP protocol logging;
- old stateful session assumptions.

### C++ strategy

Current official SDK matrix does not list C++.

Preferred initial implementation:
- small SuprAI-owned MCP client module;
- Qt JSON;
- QProcess for stdio;
- QtNetwork for Streamable HTTP;
- strict version/era layer;
- protocol conformance fixtures.

Do not implement the entire protocol before needed.

MVP client capabilities:
- server discovery/negotiation required by current/legacy interoperability strategy;
- tools/list;
- tools/call;
- resources/list/read if useful;
- prompts/list/get if useful;
- structured tool results;
- cancellation/timeouts;
- MRTR only when a real server requires it.

Compatibility:
- modern 2026-07-28 first;
- legacy 2025-11-25 compatibility can be added behind a protocol-era adapter;
- never let legacy behavior contaminate the core agent model.

## 7. Skills

Adopt Agent Skills specification:
- https://agentskills.io/specification

Canonical unit:
```text
skill-name/
  SKILL.md
  scripts/
  references/
  assets/
```

Required SKILL.md frontmatter:
- name
- description

Support standard optional fields where applicable:
- license
- compatibility
- metadata
- allowed-tools (currently experimental; do not treat as trusted authorization)

Progressive disclosure:
1. index name + description;
2. load SKILL.md body on activation;
3. load references/assets/scripts on demand.

Suggested locations:
- project: .agents/skills/
- user: $XDG_DATA_HOME/suprai/skills/

SuprAI-specific metadata should use namespaced metadata keys rather than fork the format.

Important:
A skill's `allowed-tools` is descriptive/pre-approval metadata, not a bypass around SuprAI's PolicyEngine.

## 8. Memory v1

Separate four concepts:

### A. Canonical conversation history
SQLite.
Append/durable message and item records.
Never replace canonical history with a summary.

### B. Searchable history
SQLite FTS5 from the start.
No embeddings required for baseline search.

### C. Curated active memory
Small bounded set injected into context.
Scopes:
- user;
- agent/profile;
- project.

Each memory record should have metadata outside the recalled text:
- id;
- scope;
- provenance;
- created_at;
- updated_at;
- trust class;
- source session/message;
- supersedes;
- status;
- importance optional.

Suggested trust classes:
- explicit_user
- agent_inferred
- imported
- untrusted_external
- system

An agent-generated memory should not be able to rewrite its own provenance/trust by embedding metadata in prose.

### D. Retrieval memory
Later.
Hybrid FTS/vector retrieval can be added when evidence shows FTS is inadequate.
Do not make an embedding model a permanent runtime dependency in v1.

### Context/cache behavior

Use a stable rendered memory snapshot within a turn/session epoch where possible. Changes are persisted immediately, but context updates should be explicit rather than silently mutating an in-flight prompt prefix.

## 9. Qt / Linux deployment

Current line:
- Qt 6.12.0 released 2026-09-30;
- 6.12 is designated LTS;
- extended immediate LTS patch access has commercial-license nuances.

Do not write documentation implying all open-source users automatically receive the full commercial LTS patch stream immediately.

### Deployment

Prefer Qt CMake APIs:
- qt_add_qml_module
- qt_generate_deploy_qml_app_script
- qt_deploy_runtime_dependencies / QML deployment machinery

Stage a self-contained AppDir through CMake/Qt first.

Then package/finalize:
- appimagetool, or
- linuxdeploy used narrowly as packaging/finalization if useful.

Reason:
Qt itself can inspect runtime dependencies, deploy Qt plugins, QML modules and generate qt.conf. This keeps the authoritative install tree under CMake instead of a third-party Qt plugin heuristic.

### QML
Use qrc/qt_add_qml_module for SuprAI-owned QML/components/resources where appropriate.
External Qt QML modules still need deployment.

### ABI
The AppImage build environment determines the minimum glibc/libstdc++ compatibility.
Candidate baseline must be proven in CI; do not finalize it in documentation before artifact tests.

## 10. Qt licensing gate

Before first public binary:
- select/document SuprAI license;
- audit every Qt module;
- prefer dynamic/shared Qt libraries in AppImage;
- include required Qt/LGPL notices;
- provide the required Qt source-code availability mechanism;
- ensure relinking/replacement is not technically prohibited;
- avoid GPL-only Qt modules unless SuprAI licensing intentionally permits them.

Qt WebEngine remains a separate dependency/security/licensing review.

## 11. Immediate architecture changes recommended

Accepted/strong candidates:
- normalized inference types;
- Responses-first + Chat fallback transports;
- explicit agent state machine;
- JSON Schema 2020-12 canonical tool schemas;
- PolicyEngine separate from containment;
- MCP 2026-07-28-first client;
- Agent Skills compatibility;
- SQLite + FTS5 memory foundation with provenance.

Proof-required before locking:
- exact AppImage builder/finalizer;
- exact minimum distro ABI;
- Landlock vs bwrap default containment;
- exact JSON Schema validator library;
- Qt patch/toolchain pin;
- semantic/vector memory backend.
