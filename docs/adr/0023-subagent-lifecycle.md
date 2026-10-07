# ADR-0023: Subagents are Task-owned child Sessions

Status: accepted
Date: 2026-10-06

## Decision

A SuprAI subagent is not another runtime implementation.

It is:

```text
Task(source=subagent)
  + child Session
  + child Turn/Run lifecycle
  + requester/parent binding
```

All subagents execute through NativeSuprAIRuntime.

Subagents are a general execution primitive. They may serve different purposes without creating separate runtimes or managers.

Baseline purposes:

```text
delegation
deliberation
verification
research
coding
```

The exact list may evolve, but `deliberation` is explicitly a subagent specialization, not a separate DeliberationBranch subsystem.

## Context

Default child context is isolated and explicit.

The parent creates a TaskBrief containing:
- goal;
- selected context/items;
- project/workspace identity;
- attachments;
- expected output contract;
- model/profile choice;
- capability envelope;
- subagent purpose;
- context mode.

Do not implicitly clone the entire parent conversation by default.

### Context modes

Subagent context construction supports:

```text
full
scoped
compacted
```

#### full
Use a complete parent snapshot when the parent context is small enough and exact continuity is useful.

#### scoped
Use only the context relevant to the delegated task:
- current user request;
- selected canonical items;
- project constraints;
- selected files/artifacts;
- selected memory;
- required tool state.

This should be preferred when copying the full parent would waste a large part of the child's context window.

#### compacted
Use a derived compacted parent snapshot when context pressure is already high.

Compaction remains governed by ADR-0019:
- canonical history is not rewritten;
- provenance is retained;
- the compacted snapshot is a derived artifact.

The runtime may select context mode adaptively, but the chosen mode is observable.

## Subagent purposes

### delegation
The child performs a bounded external task and returns a result.

Examples:
- inspect a subsystem;
- modify a specific module;
- run a bounded workflow.

### deliberation
The child exists primarily to perform isolated reasoning.

Typical use:
- compare architectures;
- reason deeply;
- explore hypotheses;
- plan;
- synthesize evidence;
- reason with high effort without carrying raw scratch reasoning into the parent context.

A deliberation child may still use read/search tools when allowed.

It should be read-mostly by default.

### verification
The child critiques or verifies a candidate answer, plan, patch, assumption, or result.

It should receive the candidate plus the minimum evidence needed to test it.

### research
The child gathers and synthesizes information.

### coding
The child performs a bounded code task under explicit filesystem/process policy.

Purposes are policy/profile hints, not new runtime types.

## Deliberation subagents and reasoning isolation

A deliberation subagent is the canonical SuprAI mechanism for context-folded reasoning.

Architecture:

```text
Parent Session
  -> Parent Turn
      -> Task(source=subagent, purpose=deliberation)
          -> Child Session
              -> Child Turn/Run
                  -> ReasoningWorkspace
                  -> optional tools
                  -> ReturnCapsule
      -> Parent receives compact result
```

Raw child reasoning does not become parent canonical history.

The child Session may be ephemeral at product level while still having enough temporary/runtime persistence for cancellation, diagnostics, recovery policy, and exact result routing.

Do not create a parallel `DeliberationBranchManager`.

## ReasoningWorkspace

`ReasoningWorkspace` is ephemeral scratch state inside a Run.

It may contain:
- raw reasoning exposed by the provider;
- temporary plans;
- hypotheses;
- provisional conclusions;
- temporary provider reasoning state;
- reasoning/tool planning metadata.

It is not canonical conversation history.

It does not itself create a new context. Context isolation comes from the child Session/subagent boundary.

Therefore:

```text
Subagent Child Session
  -> Turn
      -> Run
          -> ReasoningWorkspace
```

By default raw ReasoningWorkspace data is discarded when it is no longer needed.

Explicit debug/diagnostic retention, if later supported, must be clearly separate from canonical context reconstruction.

## ReturnCapsule

Deliberation/research/verification children should return a compact structured result rather than a prose dump of raw chain-of-thought.

Baseline shape:

```text
goal
facts[]
hypotheses[]
decisions[]
constraints[]
unresolved[]
next_steps[]
evidence_refs[]
artifacts[]
final_answer?
```

Properties:
- facts retain provenance;
- hypotheses remain explicitly hypotheses;
- evidence points to canonical tool/file/artifact references when possible;
- child-generated claims do not gain user/system authority;
- ReturnCapsule is derived state, not a replacement for canonical evidence.

The exact schema remains proof-gated and may be narrowed before implementation.

## Authority

Effective child authority is the intersection of:
- parent/requester authority;
- configured child profile;
- task-specific restrictions;
- current system/project policy.

Children can never widen authority.

Default child restrictions should block control-plane side effects such as:
- persistent memory writes;
- schedule creation;
- external messaging;
- privilege escalation;
- recursive spawning unless explicitly enabled.

### Deliberation authority

Deliberation children are more restricted by default.

Recommended baseline:
- filesystem read: capability/policy dependent;
- search/retrieval: capability/policy dependent;
- filesystem mutation: deny by default;
- shell/process mutation: deny by default;
- external messaging: deny;
- persistent memory writes: deny;
- schedule creation: deny;
- privilege escalation: deny;
- child spawning: deny unless explicitly enabled.

Reasoning-heavy execution should not gain side-effect authority merely because it is doing more reasoning.

## Model/profile selection

A child may use:
- the same provider/model as the parent;
- the same model with a different reasoning effort;
- a different configured model/profile later.

Example:

```text
Parent:
  model = local-model
  reasoning = low

Deliberation child:
  model = same local-model
  reasoning = high/xhigh
```

This is an orchestration decision, not an assumption that multiple models must be loaded simultaneously.

ExecutionScheduler decides actual physical concurrency.

Logical concurrency does not imply simultaneous GPU generation.

## Depth and concurrency

- default max spawn depth: 1;
- always bounded;
- per-parent/session cap;
- global cap;
- provider/model-aware concurrency cap.

With one large local model, multiple logical children may execute serially or with provider-supported scheduling.

Do not design the API around multi-GPU assumptions.

## Attached vs detached

### attached
Parent Turn logically depends on the child.
- parent cancellation cascades;
- parent may yield;
- completion resumes the same logical Turn.

A deliberation child used to produce the current answer is normally attached.

### detached
Child may outlive the spawning Run/Turn.
- Task remains registered;
- completion routes later to requester Session;
- explicit cancellation remains available.

Spawn semantics must declare which mode applies.

## Completion

Completion is push/event driven.

The model must not poll child state in a loop.

The parent can use a runtime control equivalent to `yieldUntil(task_ids)`:
- current Run stops;
- Turn becomes waiting/suspended;
- Task completion wakes the Turn;
- a new Run generation continues it.

## Merge modes

A child result may be consumed in two main ways.

### return_state_then_continue

```text
child -> ReturnCapsule
parent -> another inference using capsule
parent -> final answer/action
```

Use when the parent should synthesize, arbitrate, or verify the result.

### return_answer

```text
child -> ReturnCapsule + final_answer
parent -> persist/display accepted answer
```

Use when another parent inference would add little value.

Policy must make the merge mode explicit.

## Result trust

Store child transcript according to Task durability/debug policy, but do not inject it wholesale into parent context.

Inject into parent by default only:
- ReturnCapsule;
- final result/summary;
- artifact references;
- relevant failure metadata.

Subagent output is agent-generated evidence, not user/system authority.

## Completion binding

Bind result routing to exact:
- requester_session_id;
- requester_turn_id;
- spawning_run_id;
- task_id;
- child_session_id;
- child run generation.

Stale results cannot enter a replaced/new Session merely because a display key matches.

## Provider-specific reasoning controls

Subagent/context isolation is provider-independent.

Provider-specific features may improve it.

For example, when NInfer supports `preserve_thinking`, SuprAI may explicitly request omission of closed-turn reasoning. This is complementary to child-session isolation and must not become required for correctness.

Generic OpenAI-compatible providers remain first-class.

## Why

Hermes, OpenClaw and Goose converge on isolated child execution, restricted authority and bounded concurrency. Context Folding research shows that temporary sub-trajectories can return compact results instead of polluting the main context.

Reusing the existing subagent/Task/child-Session architecture for deliberation is simpler and more coherent than building a second branch lifecycle with duplicate:
- context ownership;
- cancellation;
- scheduling;
- permissions;
- persistence;
- recovery;
- result routing.

## Consequences

- no separate DeliberationBranch entity is required;
- ReasoningWorkspace remains an internal ephemeral Run concept;
- deliberation becomes a constrained subagent purpose;
- full/scoped/compacted child context modes must be implemented/benchmarked;
- ExecutionScheduler controls real GPU concurrency;
- raw reasoning can be discarded without discarding useful derived state;
- the parent context only receives compact derived output by default.
