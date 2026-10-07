
# Cline Code Architecture Review — 2026-10-07

Status: current research baseline
Upstream reviewed: https://github.com/cline/cline
Pinned upstream commit: e6a3b1cf0a730f396b34727e2da3ab73871b9553
License observed: Apache-2.0

Purpose: identify concrete engineering and product patterns from current Cline that improve SuprAI without copying Cline's VS Code, TypeScript, WebView stack or importing Cline as a runtime dependency.

## Executive assessment

The strongest Cline reference is no longer the historical VS Code Task/controller implementation.

Current Cline has evolved toward this layered shape:

    @cline/shared
         |
         v
    @cline/llms
         |
         v
    @cline/agents
         |
         v
    @cline/core
         |
         v
    Host applications

The important split is:

- agents: comparatively stateless agent execution loop, tool orchestration, runtime events, hooks and request preparation;
- core: stateful sessions, persistence, compaction, plugins, hub, schedules, subagents and host services;
- hosts: VS Code/CLI/Desktop-specific UI, terminal, diff and transport adapters.

This strongly validates SuprAI's modular-monolith direction and exposes several patterns worth adopting or adapting.

The old .clinerules/cline-overview.md describes the historical WebviewProvider -> Controller -> Task architecture. For current design research, sdk/ARCHITECTURE.md and sdk/packages/* are the stronger sources.

## 1. Runtime kernel vs stateful orchestration

Cline separates model/tool iteration from stateful session orchestration.

SuprAI should strongly consider refining NativeSuprAIRuntime into:

    RuntimeOrchestrator
          |
          v
      AgentEngine

AgentEngine responsibilities:
- provider request lifecycle;
- turn iteration;
- tool-call sequencing;
- low-level runtime events;
- cancellation boundaries.

RuntimeOrchestrator responsibilities:
- Session/Input/Turn/Run ownership;
- persistence;
- approvals/user actions;
- TaskManager and subagents;
- context policy;
- recovery;
- memory/project integration.

Assessment: ADOPT THE PRINCIPLE.

Do not copy Cline package names or Node architecture. Define the exact C++ split before accepting a new ADR.

## 2. Provider request projection is not canonical history

Cline's prepareTurn may rewrite messages/system prompt for a provider request without replacing its saved transcript.

This validates SuprAI's existing direction:

    Canonical ConversationItems
           |
           v
      ContextManager
           |
           v
      RequestProjection
           |
           v
      ProviderTransport

Assessment: ALREADY ALIGNED; STRENGTHEN.

Provider-prepared messages must never silently become canonical history.

## 3. Low-level runtime events -> domain events -> UI projection

Cline has a low-level AgentRuntimeEvent stream, then a RuntimeEventAdapter, then host/UI translation.

SuprAI should use the same conceptual layering:

    AgentEngineEvent
          |
          v
    RuntimeEventAdapter
          |
          v
    Domain/ApplicationEvent
          |
          v
    QAbstractListModel / QML

Assessment: ADOPT.

Benefits:
- QML never depends on provider/engine quirks;
- runtime and UI can evolve independently;
- a later headless/remote client can consume domain events;
- mappings are separately testable.

## 4. Blocking interceptors vs non-blocking observers

Cline exposes lifecycle hooks before/after run, model and tool.

A recent Cline performance bug came from forwarding every streaming delta through remote hooks synchronously. Every token caused serialization, IPC/persistence and an awaited round trip.

SuprAI should separate:

Interceptors:
- may block or transform;
- explicit timeout/cancellation;
- examples: policy, approvals, before inference, before/after tool.

Observers:
- cannot alter correctness;
- never block token generation;
- examples: UI, telemetry, diagnostics and logs.

Streaming observer delivery can be queued/coalesced.

Assessment: ADOPT WITH A STRONGER CONTRACT.

## 5. Retry only before observable generation

Current Cline retries classified transient provider failures with bounded exponential backoff.

Critical rule: once a turn emitted text, reasoning, media or a tool call, it is never transparently retried.

SuprAI rule:

    request attempt
      |
      +-- failure before observable output
      |      -> bounded classified retry may be safe
      |
      +-- observable output/tool call emitted
             -> no transparent retry
             -> explicit incomplete/failure outcome

An unchanged prepared/compacted request should be reused on a safe retry rather than compacted again.

Assessment: ADOPT / ALREADY ALIGNED.

## 6. Actual provider token counts feed compaction

Cline previously relied on character/token estimates. Dense content such as disassembly, image dumps and minified sources could overflow while estimates remained low.

Current Cline feeds provider-reported previous input-token usage back into request preparation and uses it to correct conservative budgeting.

It also has deterministic/basic context-overflow recovery because emergency recovery must not depend only on another successful summarizer call.

Reasoning models forced Cline to increase summarizer output allowance because a tight budget could be consumed by thinking with no useful summary text.

Assessment: ADOPT THE LESSON while retaining SuprAI's stronger canonical-history/CompactionArtifact design.

Requirements to preserve:
- exact provider-native token count when available;
- previous actual count can calibrate estimator conservatively;
- deterministic emergency recovery path;
- emergency recovery does not require an LLM;
- reserve actual visible summary output headroom for reasoning models;
- one compaction per unchanged prepared request;
- canonical history remains untouched.

## 7. Oversized tool-result virtualization

Current Cline supports a cache-oversized result policy.

For opted-in tools:
- original/full result remains available;
- model request receives a bounded preview;
- the model receives a recoverable reference;
- later reads can fetch more of the result;
- native images are not flattened into the text preview.

This is extremely relevant to local finite-context models.

Proposed SuprAI shape:

    ToolResult
       |
       +--> full canonical result / Artifact
       |
       +--> ContextProjection
              bounded preview
              artifact reference
              recovery hint

Potential LargeResultArtifact fields:
- artifact_id;
- session_id;
- tool_invocation_id;
- media type;
- size;
- provenance;
- retention policy.

A read-artifact capability can retrieve selected ranges later.

Important:
- reference is scoped to session/project authority;
- filesystem/shell does not implicitly resolve it;
- original output remains auditable;
- model-facing preview is independently bounded.

Assessment: HIGH-VALUE CANDIDATE.

## 8. Tool execution modes and ordering barriers

Cline tools may be sequential or parallel. Adjacent parallel calls may overlap while sequential tools form ordering barriers. Subagent calls now use this mechanism.

SuprAI should adapt, not copy literally.

Actual concurrency should be:

    parallel-safe declaration
      + policy
      + resource/path conflict analysis
      + provider/hardware budget
      = scheduler decision

Assessment: ADAPT.

Our resource-aware scheduler remains stronger than a single boolean.

## 9. Child-agent approvals must be identity-bound

Cline found a real concurrency bug: multiple child agents could reach a shared terminal approval prompt and a single yes could approve more than one pending operation.

SuprAI invariant:
every user action is bound to exact:
- session_id;
- turn_id;
- run_id;
- task_id when relevant;
- child_session_id when relevant;
- tool_invocation_id;
- user_action/request ID.

Approving delegation is not blanket approval for all child mutations.

Assessment: ADOPT AS INVARIANT.

## 10. Proceed While Running

Cline VS Code supports a visible foreground command, background execution and a Proceed While Running action.

When detached:
- agent receives bounded partial output;
- process continues;
- remaining output goes to a bounded log;
- completion remains observable;
- long-running command does not block the agent forever.

This maps directly to SuprAI's existing ToolInvocation -> Task ownership handoff.

Proposed flow:

    ToolInvocation owns process
             |
             | Continue while running
             v
       TaskManager owns process
             |
             +--> partial result returned to agent
             +--> log/artifact continues
             +--> Task remains observable
             +--> completion event later

Assessment: ADOPT THE PRODUCT/LIFECYCLE PATTERN.

Potential UI:
- Stop;
- Continue while running;
- Open terminal/log;
- background task indicator.

## 11. Bounded output everywhere

Cline explicitly caps:
- live terminal buffers;
- detached logs;
- model-facing output;
and preserves useful tail/final status.

SuprAI must separately bound:
- UI live tail;
- in-memory event backlog;
- canonical artifact storage/retention;
- model-facing preview;
- detached logs.

Truncation must always be explicit.

Assessment: ADOPT.

## 12. Workspace checkpoints separate conversation from filesystem state

Cline's current checkpoint implementation is considerably stronger than the simple old "shadow repo" description.

It uses Git objects/private refs, stash-shaped snapshots, untracked-file capture, private persistent scratch index, diff comparison, restore transactions and rollback.

Conversation and workspace can be restored independently:
- workspace only;
- conversation/task only;
- both.

SuprAI should preserve two independent axes:

    conversation lineage
    workspace filesystem state

Potential optional module:

    WorkspaceCheckpointService
      capture
      compare
      restore
      prune

Safeguards:
- never silently rewrite/drop user's real Git commits;
- intentionally snapshot untracked files;
- restore is transactional/recoverable;
- multi-root/multiple repos require coordination;
- cost/storage are observable;
- cleanup policy is explicit.

Assessment: HIGH-VALUE OPTIONAL MODULE.

## 13. Editing an older prompt should create lineage

Cline integrates previous-message editing with session forking/checkpoint restore.

SuprAI should not mutate canonical history in place.

    edit old Input
       |
       v
    branch/new lineage
       |
       +--> optional workspace restore
       |
       v
    new Turn

Assessment: ADOPT SEMANTICS.

## 14. Prepare ChangeSet -> preview -> approve -> apply

Cline's VS Code host wraps generic editor/apply-patch capabilities with native diff preview and approval behavior.

The preview remains host/UI behavior rather than being hardcoded into the agent kernel.

SuprAI should go one step further:

    Prepare normalized ChangeSet
          |
    Validate expected base/version
          |
    Native diff preview
          |
    Policy / approval
          |
    Apply EXACT prepared ChangeSet
          |
    Persist actual result / checkpoint

Approval should bind to the prepared change hash/version.

If target content changed between preview and apply, invalidate/reprepare/reapprove.

Preview failure must not mutate the file or silently approve.

Assessment: ADOPT THE SEPARATION; PROTOTYPE BEFORE ADR.

## 15. Checkpoints help auto-approval but do not make arbitrary side effects reversible

Cline presents checkpoints as a safety net for autonomous edits/commands.

SuprAI must be stricter.

Checkpoint rollback cannot undo:
- external messages;
- remote API writes;
- database side effects;
- credential leakage;
- privilege changes.

Checkpoint availability can lower friction for reversible workspace mutations only.

Assessment: ADAPT CRITICALLY.

## 16. Tool policy: useful shape, unsafe default

Cline's SDK tool policy is essentially enabled + autoApprove with wildcard/per-tool overrides. SDK docs currently say an unspecified tool is enabled and auto-approved.

Do NOT copy that default.

SuprAI keeps:
- allow;
- ask;
- deny;
- scoped path/network/command/resource policy.

Useful Cline pieces:
- wildcard baseline plus per-tool override;
- timeout/retry metadata.

Assessment: DO NOT COPY DEFAULT; ADAPT OVERRIDE SHAPE.

## 17. Typed lifecycle interceptors

Cline hooks can:
- inject context;
- mutate provider request;
- block model/tool execution;
- transform tool result;
- observe events.

SuprAI can use internal typed interception points:

    BeforeRun
    BeforeInference
    AfterInference
    BeforeTool
    AfterTool
    AfterRun

Rules:
- observational hooks cannot mutate;
- blocking interceptors have deadlines;
- order is deterministic;
- transformations retain provenance;
- no public unbounded plugin ABI in v1.

Assessment: ADAPT.

## 18. Completion policy

Cline can require explicit completion semantics in task-oriented modes and can nudge/continue if a model returns ordinary text when an explicit completion is expected.

Potential SuprAI concept:

    TurnCompletionPolicy
      conversational
      explicit_task_completion
      structured_result

Useful for:
- autonomous coding;
- scheduled runs;
- subagents;
- workflows.

Ordinary chat should still terminate naturally with final text.

Assessment: MODE-SPECIFIC CANDIDATE.

## 19. Provider/model capabilities need tri-state semantics

Cline recently fixed bugs where absent/empty capability metadata was treated as an authoritative denial, silently removing tools/images for several custom/gateway providers.

This is directly relevant to local OpenAI-compatible endpoints.

SuprAI should model:

    supported
    unsupported
    unknown

Never interpret missing metadata as unsupported.

Capability provenance should distinguish:
- explicit provider declaration;
- verified probe;
- user override;
- unknown.

Assessment: ADOPT IMMEDIATELY.

## 20. MCP failure isolation and trust

Cline current lessons:
- bounded connect timeout;
- unreachable MCP server must not destroy the whole interactive session;
- capability list operations can run concurrently after initialize;
- cleanup continues even if one server disconnect fails;
- opening a repo does not implicitly start workspace-controlled Agent Plugin MCP servers.

SuprAI rules:
- one server failure degrades only that server/capability;
- connect/discovery have deadlines;
- dispose independent servers with all-settled semantics;
- opening a project never executes repo-controlled MCP/plugin code automatically;
- passive discovery and executable activation are different trust steps.

Assessment: ADOPT.

## 21. Lazy session persistence

Current Cline may allocate a session ID without immediately creating an empty durable history entry. First accepted user turn makes it durable.

Good for avoiding empty-session clutter.

SuprAI can consider:
- transient new-chat identity;
- persist when first Input is admitted;
- drafts/attachments before send remain explicitly draft-owned.

Assessment: GOOD UX CANDIDATE.

## 22. Root and child sessions

Cline persists child/subagent sessions while normal history can query root sessions only.

SuprAI should similarly:
- persist child sessions with lineage;
- default history UI to root sessions;
- expose child tree in inspector;
- filter before pagination at persistence/query boundary.

Assessment: ADOPT.

## 23. User-action state is explicit

Cline VS Code host has explicit phases such as idle, streaming, awaiting approval and awaiting follow-up.

When switching/replacing tasks it settles pending approvals/questions so an abandoned run does not remain blocked forever.

SuprAI rule:
- every pending UserActionRequired has an owner and terminal resolution;
- replacing/deleting/switching execution resolves or cancels old pending actions;
- UI phase projects authoritative runtime state, not the last visible card.

Assessment: ADOPT.

## 24. Status is reported, not fabricated

Cline's current hub code is careful not to default an incomplete snapshot to running, because late state can reopen a completed run or leave UI gates stuck.

SuprAI already has generation fencing. Preserve:
- authoritative state;
- monotonic/validated transitions;
- stale-event rejection.

Assessment: VALIDATES CURRENT DESIGN.

## 25. Streaming coalescing belongs at boundaries

Cline coalesces/bounds shell output and slower presentation/transport consumers.

SuprAI should separate:
- semantic event order;
- event-delivery batching;
- UI render cadence.

Never drop state transitions; text/progress deltas may be batched.

Assessment: VALIDATES ADR-0014/0015.

## 26. Plan/Act is profile/policy, not a core boolean

Cline's Plan/Act UX is useful, but SuprAI should not hard-code a special global mode.

Represent it as profiles:

Planning profile:
- read-mostly;
- mutation disabled;
- higher deliberation if desired.

Action profile:
- mutation capabilities exposed;
- approvals follow policy.

Assessment: ADAPT PRODUCT IDEA, NOT IMPLEMENTATION.

## 27. Browser automation should remain optional

Cline's browser integration is not a good baseline dependency for SuprAI.

Reasons:
- browser runtime weight;
- security surface;
- unnecessary for native core.

If added later:
- optional module;
- explicit engine/remote-browser decision;
- separate ADR.

Assessment: AVOID AS BASELINE.

## 28. Host-specific adapters around generic core

Current Cline's VS Code host overrides:
- shell execution with visible VS Code terminal;
- generic file edit with native diff UI;
- runtime events with VS Code message projection;
- generic session lifecycle with host coordinators.

SuprAI should keep equivalent seams even with only one host today.

Examples:
- generic mutation contract -> QML diff presenter;
- process Task -> native terminal/log inspector;
- OpenURI -> XDG portal;
- notification -> portal/freedesktop.

Assessment: ADOPT.

## 29. Loop/mistake guard

Cline has repeated tool-call loop detection and mistake limits.

This is useful for local models, but should be more semantic than "same tool N times".

Potential SuprAI LoopGuard signals:
- same normalized tool call repeatedly;
- unchanged failure repeatedly;
- no new evidence/state;
- repeated max-output cutoff;
- repeated invalid schema output;
- repeated read of unchanged artifact.

Possible actions:
- one corrective notice;
- alter strategy;
- ask user;
- bounded terminal stop.

TaskManager should eliminate legitimate polling loops first.

Assessment: ADAPT.

## 30. Worktree-isolated coding sessions

Current Cline Desktop can run a task in a dedicated Git worktree/branch.

Potential SuprAI project mode:

    workspace_mode
      in_place
      isolated_worktree

Benefits:
- safer autonomous coding;
- easy diff/merge/discard;
- stronger isolation than rollback alone.

Costs:
- Git only;
- disk usage;
- submodule/LFS/worktree complexity;
- cleanup and path semantics.

Assessment: STRONG FUTURE CANDIDATE, NOT BASELINE.

## 31. What not to copy

Do not copy:
- VS Code/WebView/React technology stack;
- current Cline default auto-approval semantics;
- simple parallel/sequential flag as sole scheduler safety;
- provider wire types as canonical domain state;
- browser/Puppeteer as core dependency;
- historical giant Task/controller architecture;
- mutable-history compaction semantics;
- source code wholesale without deliberate Apache-2.0 compliance.

Architectural patterns should normally be reimplemented in SuprAI's C++/Qt design.

## 32. Highest-value candidates

Tier A — architecture/implementation planning:
1. runtime execution kernel vs stateful orchestrator;
2. engine events -> domain events -> UI projection;
3. blocking interceptors vs non-blocking observers;
4. no retry after observable generation;
5. actual provider token feedback;
6. tri-state provider capabilities;
7. identity-bound user/child approvals;
8. bounded/coalesced output.

Tier B — likely product/runtime features:
9. large ToolResult artifacts with bounded model projection;
10. Proceed While Running / ToolInvocation -> Task handoff;
11. workspace checkpoints with independent conversation/workspace restore;
12. prepared ChangeSet -> preview -> approval -> exact apply;
13. root/child session persistence with root-only default list;
14. LoopGuard;
15. lazy persistence of untouched sessions.

Tier C — later optional workflow:
16. isolated Git worktree coding sessions;
17. Plan/Act-like profiles;
18. browser automation.

## 33. Recommended next design work

Before M1/M2 architecture hardens:
- decide exact AgentEngine vs RuntimeOrchestrator C++ split;
- preserve engine-event/domain-event separation.

Before M4:
- prototype ChangeSet/preview/approval/exact-apply;
- define process output limits;
- define ToolInvocation -> Task background handoff.

Before M6:
- define LargeResultArtifact/context projection;
- incorporate provider actual-token feedback;
- deterministic emergency compaction.

Before advanced coding UX:
- prototype WorkspaceCheckpointService;
- compare checkpoints with isolated Git worktree execution.

## Sources inspected

Current architecture:
- https://github.com/cline/cline/blob/e6a3b1cf0a730f396b34727e2da3ab73871b9553/sdk/ARCHITECTURE.md
- https://github.com/cline/cline/blob/e6a3b1cf0a730f396b34727e2da3ab73871b9553/sdk/packages/agents/README.md
- https://github.com/cline/cline/blob/e6a3b1cf0a730f396b34727e2da3ab73871b9553/sdk/packages/agents/src/agent-runtime.ts
- https://github.com/cline/cline/blob/e6a3b1cf0a730f396b34727e2da3ab73871b9553/sdk/packages/shared/src/agent.ts
- https://github.com/cline/cline/blob/e6a3b1cf0a730f396b34727e2da3ab73871b9553/sdk/packages/core/src/runtime/orchestration/session-runtime-orchestrator.ts
- https://github.com/cline/cline/blob/e6a3b1cf0a730f396b34727e2da3ab73871b9553/sdk/packages/core/src/runtime/orchestration/runtime-event-adapter.ts

Context:
- https://github.com/cline/cline/blob/e6a3b1cf0a730f396b34727e2da3ab73871b9553/sdk/packages/core/src/extensions/context/compaction.ts
- https://github.com/cline/cline/blob/e6a3b1cf0a730f396b34727e2da3ab73871b9553/docs/features/auto-compact.mdx

Checkpoints:
- https://github.com/cline/cline/blob/e6a3b1cf0a730f396b34727e2da3ab73871b9553/docs/core-workflows/checkpoints.mdx
- https://github.com/cline/cline/blob/e6a3b1cf0a730f396b34727e2da3ab73871b9553/sdk/packages/core/src/hooks/checkpoint-hooks.ts
- https://github.com/cline/cline/blob/e6a3b1cf0a730f396b34727e2da3ab73871b9553/sdk/packages/core/src/session/session-versioning-service.ts
- https://github.com/cline/cline/blob/e6a3b1cf0a730f396b34727e2da3ab73871b9553/sdk/packages/core/src/session/checkpoint-diff.ts

VS Code host:
- https://github.com/cline/cline/blob/e6a3b1cf0a730f396b34727e2da3ab73871b9553/apps/vscode/src/sdk/sdk-diff-edit-coordinator.ts
- https://github.com/cline/cline/blob/e6a3b1cf0a730f396b34727e2da3ab73871b9553/apps/vscode/src/sdk/vscode-run-commands-tool.ts
- https://github.com/cline/cline/blob/e6a3b1cf0a730f396b34727e2da3ab73871b9553/apps/vscode/src/sdk/message-translator.ts
- https://github.com/cline/cline/blob/e6a3b1cf0a730f396b34727e2da3ab73871b9553/apps/vscode/src/sdk/sdk-interaction-coordinator.ts

Tool API:
- https://github.com/cline/cline/blob/e6a3b1cf0a730f396b34727e2da3ab73871b9553/docs/sdk/reference/tools-api.mdx

License:
- https://github.com/cline/cline/blob/e6a3b1cf0a730f396b34727e2da3ab73871b9553/LICENSE
