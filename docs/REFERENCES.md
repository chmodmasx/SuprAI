# Reference Notes

Purpose: preserve architecture lessons and current external contracts without coupling SuprAI to another agent implementation.

Last major research pass: 2026-10-06.

## Research policy

Reference projects are used to:
- discover solved engineering problems;
- compare failure modes;
- validate architecture choices;
- avoid reinventing established interoperable formats.

They are not production runtime dependencies unless a future ADR explicitly approves one.

Never copy source merely because a pattern is useful. Check the source license and reimplement/adapt deliberately.

## Hermes Agent / Hermes Desktop

Upstream:
- https://github.com/NousResearch/hermes-agent

Useful lessons:
- strong authority boundaries between UI/machine/backend state;
- explicit user-action/approval flows;
- bounded curated memory;
- SQLite + FTS5 conversation search;
- project/session identity;
- local/remote execution must be explicit;
- compatibility probing.

Do not copy:
- Hermes runtime dependency;
- Electron/React desktop stack;
- Hermes gateway protocol as SuprAI's internal domain.

Relevant source paths:
- apps/desktop/AGENTS.md
- apps/desktop/DESIGN.md
- tui_gateway/
- website/docs/user-guide/features/memory.md

## OpenClaw

Upstream:
- https://github.com/openclaw/openclaw

Useful lessons:
- Linux lifecycle and packaging are architectural concerns;
- trust boundaries for privileged native bridges;
- Wayland/X11 need separate proof;
- memory provenance/trust metadata;
- external content cannot inherit native authority.

Do not copy:
- Tauri/WebKit frontend;
- OpenClaw runtime/gateway as SuprAI's engine.

## Goose

Upstream:
- https://github.com/aaif-goose/goose

Relevant source:
- crates/goose/src/agents/state_machine/
- crates/goose/src/context_mgmt/
- crates/goose/src/execution/
- crates/goose/src/agents/mcp_client.rs

Useful lesson:
A mature agent loop becomes a state machine. Current source separates LLM execution, tool calling, approvals, retries, compaction, max-turn control, steering, subagents and skills.

SuprAI response:
- ADR-0004 requires explicit turn state transitions/effects.

## GPT4All

Upstream:
- https://github.com/nomic-ai/gpt4all

Relevant source:
- gpt4all-chat/src/
- gpt4all-chat/CMakeLists.txt

Useful lesson:
Qt/QML/C++ is proven for a substantial native local-AI desktop.

Caution:
Avoid accumulating inference, persistence, context, tools and UI behavior into one ChatLLM-style god object.

## OpenCode

Documentation:
- https://opencode.ai/v2/docs/permissions

Useful lesson:
- allow / ask / deny;
- resource-pattern permission scopes;
- explicit external-directory boundary.

Critical lesson:
Approval does not reduce host authority. Shell execution still has the user's filesystem/process/network access unless OS containment is applied.

SuprAI response:
- ADR-0005 separates PolicyEngine from ContainmentBackend.

## Model Context Protocol

Current final revision:
- 2026-07-28.

Primary:
- https://blog.modelcontextprotocol.io/posts/2026-07-28/
- https://plan.modelcontextprotocol.io/matrix
- https://ts.sdk.modelcontextprotocol.io/v2/protocol-versions

Important:
- stateless core;
- MRTR;
- full JSON Schema 2020-12 tool schemas;
- formal extensions;
- roots/sampling/protocol logging deprecated;
- modern ping removed;
- protocol-era compatibility matters.

Current official SDK matrix does not list C++.

SuprAI response:
- ADR-0006 targets 2026-07-28 and initially uses a small native client over Qt/QProcess/QtNetwork.

## Agent Skills

Specification:
- https://agentskills.io/specification

Important:
- SKILL.md required;
- YAML name + description;
- optional scripts/references/assets;
- progressive disclosure;
- optional compatibility/license/metadata;
- allowed-tools is experimental.

SuprAI response:
- ADR-0007 adopts the standard.
- allowed-tools never bypasses PolicyEngine.

## Local inference servers

### llama.cpp
- https://github.com/ggml-org/llama.cpp/blob/master/tools/server/README.md

Current relevant capabilities:
- Chat Completions;
- Responses;
- SSE;
- tool/function calling;
- multimodal;
- schema-constrained output support.

### vLLM
- https://docs.vllm.ai/en/stable/api/vllm/entrypoints/openai/responses/

Relevant:
- Responses implementation;
- typed streaming events;
- reasoning/tool parsing ecosystem.

### NInfer
- https://github.com/Neroued/ninfer/blob/master/docs/serving.md

Relevant:
- /v1/chat/completions;
- /v1/responses;
- typed Items/SSE;
- reasoning separated from answer text;
- function calls;
- input-token endpoint;
- local response state/continuations;
- explicit rejection of unsupported fields.

SuprAI response:
- ADR-0003 uses normalized inference types, Responses preferred, Chat compatibility.

## JSON Schema

Current canonical dialect for SuprAI tools:
- JSON Schema 2020-12.

Primary spec:
- https://json-schema.org/draft/2020-12/json-schema-core.html

MCP 2026-07-28 also uses full JSON Schema 2020-12 for tool input/output schemas.

### C++ validator research

jsoncons:
- https://github.com/danielaparker/jsoncons
- documents Draft 2020-12 support;
- header-only;
- Boost Software License;
- documents required-test-suite compliance for implemented keywords.

Valijson:
- current stated target is draft-7.

pboettch/json-schema-validator:
- current stated target is draft-7.

SuprAI response:
- ADR-0009 proposes jsoncons behind a narrow SchemaValidator interface, pending proof.

## Linux containment

### Landlock
- https://landlock.io/

Useful:
- unprivileged self-restriction;
- stackable LSM;
- reduces ambient rights.

### bubblewrap
- https://github.com/containers/bubblewrap

Useful:
- mount/user/PID/network namespace construction;
- seccomp support;
- no-new-privileges.

Critical:
- bubblewrap explicitly states it is a low-level sandbox construction tool, not a complete security policy;
- Ubuntu 24.04+ restricts unprivileged user namespaces through AppArmor;
- AppImage cannot assume bwrap is always usable.

Ubuntu references:
- https://documentation.ubuntu.com/release-notes/24.04/
- https://documentation.ubuntu.com/security/security-features/privilege-restriction/apparmor/

SuprAI response:
- PolicyEngine always;
- containment feature-probed;
- Landlock/bwrap/none are distinct effective states.

## Qt

Current target family:
- Qt 6.12.

Primary:
- https://doc.qt.io/qt-6.12/supported-platforms.html
- https://doc.qt.io/qt-6/qt-releases.html
- https://doc.qt.io/qt-6/qt-generate-deploy-qml-app-script.html
- https://doc.qt.io/qt-6/linux-deployment.html

Current facts:
- Qt 6.12.0 released 2026-09-30;
- Qt 6.12 is an LTS line;
- immediate extended LTS patch access has commercial-license distinctions;
- Ubuntu 22.04 x86_64/GCC 11 is a supported configuration;
- official Linux Online Installer binaries are built on Ubuntu 24.04/glibc 2.39;
- CMake deployment APIs can stage Qt runtime dependencies/plugins/QML.

Licensing:
- https://www.qt.io/development/open-source-lgpl-obligations

Before distribution, audit each module and satisfy LGPL/GPL obligations.

## AppImage

Primary guidance:
- https://docs.appimage.org/reference/best-practices.html
- https://docs.appimage.org/introduction/concepts.html

Important:
- build on a base no newer than the oldest supported target;
- AppImage does not remove glibc/libstdc++ ABI floors;
- host graphics/system libraries require deliberate exclusion/inclusion decisions.

linuxdeploy Qt plugin:
- https://github.com/linuxdeploy/linuxdeploy-plugin-qt

It supports Qt 6 and QML, but Wayland/QML edge cases make it unsuitable as the sole dependency-discovery authority for a Wayland-first application without proof.

SuprAI response:
- ADR-0010 proposes Qt/CMake-owned AppDir staging and AppImage finalization afterward.

## Memory references

Hermes:
- bounded curated MEMORY/USER data;
- SQLite+FTS5 session history.

OpenClaw:
- curated files plus indexed history;
- provenance/trust stored separately from recalled prose;
- optional vector layer.

SuprAI response:
- ADR-0008 starts with SQLite canonical history + FTS5 + bounded curated memory with provenance;
- vector retrieval is deferred.

## Remaining research backlog

Before M1 completion:
- prove exact Qt 6.12 build toolchain on candidate Ubuntu 22.04 baseline;
- audit minimum Qt modules and licenses;
- benchmark QML transcript rendering strategy.

Before M3:
- define normalized InferenceRequest/InferenceEvent contract exactly;
- build fake SSE compatibility corpus from llama.cpp/vLLM/NInfer;
- decide provider capability persistence/probing rules.

Before M4:
- prototype Landlock capability probe;
- prototype bubblewrap availability/failure behavior;
- define initial built-in tool set and risk taxonomy.

Before M5:
- implement MCP protocol-era conformance fixtures;
- verify actual third-party MCP servers against 2026-07-28 + legacy fallback.

Before M6:
- define memory mutation approval UX;
- measure FTS5 quality before considering vectors.

Before release:
- choose SuprAI project license;
- perform Qt/module/dependency license audit;
- produce notices/source-offer/relinking documentation as required.


## Token accounting and effective context

### NInfer
Current serving docs expose:
- effective `max_model_len` in `/v1/models`;
- `POST /v1/responses/input_tokens` through the same prompt path as generation, including tools/media/template behavior.

### llama.cpp
Current server exposes:
- `/v1/responses/input_tokens`;
- `/v1/chat/completions/input_tokens`;
- `/tokenize`, `/detokenize`, `/apply-template`;
- effective runtime `n_ctx` in `/v1/models.meta.n_ctx` and `/props`.

Important:
`n_ctx_train` is training/model metadata; `n_ctx` is the effective configured slot context and is the relevant runtime ceiling.

### vLLM
Current direct `/v1/models` exposes `max_model_len`.
vLLM also exposes generic `/tokenize` and records prompt usage after actual rendering/generation.

Do not assume generic text tokenization equals the full final agent request when chat templates, tool schemas and multimodal content are involved.

### SuprAI response
ADR-0018:
- discover effective runtime context;
- count the final candidate provider request through the best capability;
- reserve output before sending;
- use conservative margins for estimates;
- compact/recount rather than relying on provider truncation.

## Canonical conversation ownership

Local server state differs:
- llama.cpp Responses does not provide universal provider-side continuation semantics;
- NInfer stored Responses are local/bounded/process-lifetime state;
- vLLM response storage is configuration-dependent.

SuprAI response:
- ADR-0011 makes local SQLite canonical;
- provider IDs/state are optimization metadata only;
- every request remains reconstructible after inference-server restart.

## Prefix/prompt caching

### llama.cpp
Current server:
- reuses common prompt prefixes;
- exposes prompt/cache reuse controls;
- reports cached/processed token timing information.

### vLLM
Automatic Prefix Caching reuses identical token prefixes.
Current security documentation identifies cross-tenant timing side channels and provides per-request `cache_salt` isolation.

### NInfer
Supports OpenAI-style prompt cache hints/breakpoints. Documentation explicitly treats them as optimization hints rather than semantic session identity.

SuprAI response:
- ADR-0020 makes all provider caches performance-only;
- ContextBuilder keeps stable prefix material early when semantics permit;
- shared/multi-tenant providers may use secret cache isolation where supported;
- cache eviction/restart must never affect correctness.

## Context compaction

Useful upstream lessons:

OpenClaw:
- persists compaction as transcript/context metadata;
- preserves recent tail;
- preserves tool-call/result pairs across split boundaries;
- distinguishes estimated token pressure from measured request size;
- rejects/guards bad compaction output.

Hermes:
- protects recent messages;
- keeps canonical archived/searchable history;
- prunes old verbose tool results before expensive summarization;
- refuses compression when a summary would grow the request.

SuprAI response:
- ADR-0019 defines CompactionArtifact as derived state;
- canonical history is never rewritten by compaction;
- artifact records source coverage, summarizer identity and before/after counts;
- candidate request is rebuilt and recounted before commit.

## Persistence and crash recovery

SQLite:
- WAL supports concurrent readers but still serializes writes;
- Qt SQL database connections are thread-affine.

SuprAI response:
- ADR-0012 gives PersistenceWorker the primary writer;
- external mutating tool invocations are journaled before execution;
- process crash while an external effect is executing becomes `outcome_unknown`;
- non-idempotent ambiguous effects are not replayed automatically.

## Conversation item model and steering

OpenClaw research reinforces:
- append-oriented transcripts;
- structural tool-call/result pairing;
- explicit run IDs/terminal reconciliation;
- steering/user-input boundaries;
- completed tool work persisted before final answer.

SuprAI response:
- ADR-0017 stores generalized ordered items with stable SuprAI IDs;
- retries/regeneration/branches create lineage instead of rewriting completed history;
- every accepted tool call eventually has a terminal outcome item.

## Qt threading

Primary Qt documentation:
- QObject thread affinity;
- QThread worker-object pattern;
- queued signal/slot delivery;
- thread-affine network/process/timer/database objects.

SuprAI response:
- ADR-0015: UI thread + runtime worker thread + persistence worker thread;
- UI-facing models stay on the main thread;
- runtime owns provider network/process objects;
- persistence owns its QSQLITE connection.

## Linux desktop integration

### Global shortcuts
XDG Desktop Portal GlobalShortcuts v2 is the Wayland-first route.
Do not use X11 grabs as the primary design.

### Application activation
`org.freedesktop.Application` provides Activate/Open/ActivateAction semantics suitable for single-instance activation and deep links.

### Tray
Qt 6.12 `QSystemTrayIcon` uses Linux StatusNotifierItem where available and can explicitly probe tray availability.
It lives in Qt Widgets, so SuprAI may use `QApplication`/Qt::Widgets while the visible UI stays Qt Quick/QML.

### Notifications
Prefer XDG Portal Notification v2; support `org.freedesktop.Notifications` fallback.
Action support is capability-dependent.

### Secrets
QtKeychain 0.17.x is the current candidate:
- Qt 6 default;
- libsecret/GNOME Keyring;
- KWallet fallback;
- Modified BSD;
- no insecure plaintext fallback unless explicitly requested.

SuprAI response:
- ADR-0013 accepted freedesktop-first integration;
- ADR-0016 keeps QtKeychain proposed until KDE/GNOME/AppImage tests pass.

## Native transcript rendering

Qt Quick facts:
- ListView delegates are virtualized and can be reused;
- state must not live only in recycled delegates;
- variable-height delegates require cache/performance tuning;
- Qt Text Markdown supports CommonMark/GitHub-style features;
- rich/Markdown content can load external image resources unless controlled.

SuprAI response:
- ADR-0014 proposes a C++ QAbstractListModel transcript;
- safe Markdown resource policy;
- no ambient remote-image fetch;
- explicit external-link activation;
- native code-block components;
- no Qt WebEngine for ordinary chat.


## Advanced execution / orchestration references

### OpenClaw steering

Primary:
- https://docs.openclaw.ai/concepts/queue-steering

Current useful semantics:
- steer/followup/collect/interrupt are distinct;
- steering does not interrupt an already-running tool;
- sequential unstarted tool calls can be skipped after steering lands;
- skipped calls still receive synthetic paired results;
- parallel tool batches settle as a batch;
- accepted steering and actual model consumption are different events.

SuprAI response:
- ADR-0022 adopts explicit queued-input semantics and safe steering boundaries.

### OpenClaw subagents

Primary:
- https://docs.openclaw.ai/tools/subagents
- https://docs.openclaw.ai/tools/subagents/operations
- https://docs.openclaw.ai/tools/subagents/announce

Current useful semantics:
- spawning is non-blocking;
- completion is push-based;
- parent models should yield rather than poll;
- cancellation can target exact parent-run-owned child trees;
- ownership/run generation matters during recovery;
- child results remain recoverable even when delivery fails;
- ordinary parent completion does not necessarily cancel detached admitted children.

SuprAI response:
- ADR-0023 models subagents as Task-owned child Sessions.
- purposes include delegation, deliberation, verification, research and coding without separate runtimes;
- attached and detached semantics are explicit;
- completion is event-driven, not model polling;
- deliberation children are read-mostly and return compact derived state rather than raw reasoning.

### OpenClaw durable Tasks

Primary:
- https://docs.openclaw.ai/automation/tasks
- https://docs.openclaw.ai/cli/tasks

Current useful semantics:
- background Task registry is separate from Session context;
- task lifecycle distinguishes queued/running/succeeded/failed/timed_out/cancelled/lost;
- execution completion is authoritative for active task records;
- delivery/notification policy is separate from execution status;
- requester and child session identities are separately tracked.

SuprAI response:
- ADR-0024 introduces one durable TaskManager/registry.

### OpenClaw restart recovery

Primary:
- https://docs.openclaw.ai/gateway/restart-recovery

Current useful semantics:
- conversation, pending input, schedules, subagent records and deliveries are durable;
- graceful restart fences new work and drains admitted work first;
- terminal persistence is part of successful drain;
- exact ownership and lifecycle generation are required to adopt old work;
- repeated recovery is bounded;
- stale/ambiguous work may be tombstoned instead of replayed.

SuprAI response:
- ADR-0025 uses owner-generation fencing and replay-safe recovery.

### Hermes delegation

Primary:
- https://github.com/hermes-agent-org/hermes/blob/main/website/docs/user-guide/features/delegation.md
- https://github.com/NousResearch/hermes-agent/blob/main/website/docs/user-guide/features/delegation.md
- https://github.com/NousResearch/hermes-agent/blob/main/tools/AGENTS.md

Useful semantics observed across current Hermes documentation/source:
- child agents use isolated context;
- tool access is restricted;
- concurrency/depth are bounded;
- interrupt propagation follows ownership;
- child steering distinguishes queued from actually delivered;
- child background processes can require explicit ownership handoff;
- durable scheduled work is separate from synchronous delegation.

SuprAI response:
- explicit TaskBrief;
- capability intersection;
- attached/detached child modes;
- explicit process ownership handoff.

### Goose

Primary:
- https://github.com/aaif-goose/goose/blob/main/documentation/docs/guides/context-engineering/subagents.mdx
- https://github.com/aaif-goose/goose/issues/11740

Useful:
- subagents are temporary isolated workers;
- cancellation tokens and state-machine steering exist;
- current Goose architecture still has several siloed async mechanisms;
- the project is actively proposing a unified task registry for processes, timers, subagents and MCP Tasks.

This independently supports SuprAI's TaskManager direction.

### Agent Client Protocol v2

Primary:
- https://github.com/agentclientprotocol/agent-client-protocol/blob/main/docs/protocol/v2/migration.mdx
- https://github.com/agentclientprotocol/agent-client-protocol/blob/main/docs/protocol/v2/session-setup.mdx

Important v2 semantics:
- prompt response acknowledges durable insertion, not turn completion;
- running/requires_action/idle are foreground state notifications;
- background activity can continue while foreground state is idle;
- cancellation is confirmed by later lifecycle state;
- replay uses agent-owned message IDs.

SuprAI response:
- ADR-0021 separates Input admission, Turn work and Run execution.

### OpenAI Codex app-server

Relevant:
- https://github.com/openai/codex/tree/main/codex-rs/app-server
- https://github.com/openai/codex/blob/main/codex-rs/app-server-protocol/src/protocol/thread_history.rs

Useful observations:
- threads, turns and materialized items have separate identities;
- active turn steering/interrupt are distinct protocol operations;
- persisted/live lifecycle correlation is a real integration problem.

SuprAI should keep stable local IDs rather than reusing provider/app-server submission IDs as canonical execution identity.

## MCP Tasks extension

Primary:
- https://tasks.extensions.modelcontextprotocol.io/
- https://tasks.extensions.modelcontextprotocol.io/specification/draft/tasks
- https://tasks.extensions.modelcontextprotocol.io/seps/2663-tasks-extension

Current 2026-07-28 extension:
- server-directed async tools/call execution;
- states: working, input_required, completed, failed, cancelled;
- tasks/get, tasks/update, tasks/cancel;
- server-provided poll interval and TTL;
- optional task status notifications/subscriptions;
- no global tasks/list for security/isolation reasons.

SuprAI response:
- MCP Tasks are mapped into TaskManager as an external task source.
- SuprAI task IDs remain distinct from opaque MCP task IDs.
- TaskManager performs infrastructure polling/push handling; the model does not poll.

## Linux systemd transient tasks

Primary:
- https://www.man7.org/linux/man-pages/man1/systemd-run.1.html
- https://systemd.io/CONTROL_GROUP_INTERFACE/

Useful:
- user service manager can create transient service units;
- D-Bus StartTransientUnit provides a durable external handle;
- cgroup ownership can capture descendants;
- services can outlive the initiating GUI process.

SuprAI response:
- ADR-0027 proposes an optional SystemdTransient ProcessBackend.
- QProcess remains baseline until restart/output/containment behavior is proven.


## Context folding / reasoning isolation

Primary:
- Scaling Long-Horizon Agent via Context Folding, ICML 2026.
- https://proceedings.mlr.press/v306/sun26x.html
- https://github.com/sunnweiwei/FoldAgent
- https://github.com/MiaoLu3/Context_Folding

Important:
- temporary branches inherit a parent history/context;
- branch work is isolated from the main trajectory;
- return/fold preserves only a concise outcome in the main trajectory;
- published results report up to 10x smaller active context on long-horizon tasks;
- strongest published agent learns branch/return behavior with FoldGRPO;
- the released implementation directly copies main messages into a branch Agent and appends only the returned branch result to main.

Training-free related work:
- MM-ContextFold (2026): persistent compact main context + ephemeral branch contexts, then discard branch trace/raw media after textual fold.

SuprAI response:
- ADR-0028 rejects a separate standalone deliberation-branch subsystem;
- isolated deliberation reuses ADR-0023 subagents as `Task(source=subagent, purpose=deliberation)`;
- the child Session owns the temporary reasoning context and returns a compact ReturnCapsule;
- raw child reasoning does not enter parent canonical context by default;
- child context modes are full/scoped/compacted;
- full snapshots protect the parent context but do not eliminate child-local peak context;
- scoped/compacted child snapshots and automatic deliberation policy must be benchmarked.

### NInfer reasoning-history controls

Primary:
- https://github.com/Neroued/ninfer/blob/master/docs/serving.md
- current Qwen templates under tools/chat_templates/.

Important correction:
- preserve_thinking is not a universal fixed default across every artifact/template;
- when no explicit server/request override resolves it, selected template behavior matters;
- current Qwen3.6 artifacts document closed-turn reasoning omitted by default;
- current Qwen3.8 template/model cards document closed-turn reasoning retained by template default;
- request-level preserve_thinking can explicitly control the behavior when supported;
- reasoning output is returned separately from answer content;
- current templates can preserve reasoning within the active multi-step tool chain while omitting older closed-turn reasoning.

SuprAI response:
- do not depend on provider defaults;
- ContextManager owns canonical retention;
- provider-specific preserve_thinking is only a capability/optimization.
