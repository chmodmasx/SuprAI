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

## Context

Default child context is isolated and explicit.

The parent creates a TaskBrief containing:
- goal;
- selected context/items;
- project/workspace identity;
- attachments;
- expected output contract;
- model/profile choice;
- capability envelope.

Do not implicitly clone the entire parent conversation by default.

A future `fork` mode may explicitly include selected parent history.

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

## Depth and concurrency

- default max spawn depth: 1;
- always bounded;
- per-parent/session cap;
- global cap;
- provider/model-aware concurrency cap.

## Attached vs detached

### attached
Parent Turn logically depends on the child.
- parent cancellation cascades;
- parent may yield;
- completion resumes the same logical Turn.

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

## Result trust

Store full child transcript durably.

Inject into parent context by default only:
- final summary;
- structured result;
- artifact references;
- failure metadata.

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

## Why

Hermes, OpenClaw and Goose all converge on isolated child execution, restricted authority and bounded concurrency, while OpenClaw's push completion/yield design avoids wasteful model polling.
