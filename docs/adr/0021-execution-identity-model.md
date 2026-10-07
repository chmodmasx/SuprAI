# ADR-0021: Distinct Session, Input, Turn, Run, Attempt and Task identities

Status: accepted
Date: 2026-10-06  
Updated: 2026-10-07

## Decision

SuprAI uses separate durable identities for:

- Session — conversation/workspace context.
- Input — admitted user/internal input.
- Turn — one logical foreground work episode.
- Run — one executable generation/segment of a Turn.
- ProviderAttempt — one model/provider request attempt within a Run.
- ToolInvocation — one durable tool execution.
- Task — detached/asynchronous/background work record.

These concepts are never collapsed into one generic "run" identifier.

## Relationships

```text
Session
  ├─ Inputs
  └─ Turns
      └─ Runs
          ├─ ProviderAttempts
          ├─ ToolInvocations
          └─ spawned Tasks
```

A Turn may have multiple Runs sequentially due to:
- yield/wait continuation;
- restart recovery;
- infrastructure interruption;
- other explicit resume semantics.

A Task may outlive the spawning Run and may own a child Session.

## Input admission

Accepting input and completing a Turn are separate.

`submitInput()` durably inserts an Input and returns its ID. Foreground work lifecycle is reported separately.

This mirrors the useful semantic split in ACP v2 and supports queued input, steering, replay, reconnect, and background work.

## Session foreground state

Foreground state is separate from Task activity:

- idle;
- running;
- requires_action;
- cancelling;
- degraded.

A Session may be foreground-idle while Tasks remain active.

## Recovery generation

Every Run has an owner generation. A recovered/resumed Run is a new execution generation under the same logical Turn.

Late events from superseded generations are ignored.

## Why

A single Session/Message model cannot express:
- steering;
- wait/resume;
- crash recovery;
- background children;
- detached work;
- exact cancellation scope;
- duplicate-effect prevention.

## Consequences

Database schema and runtime APIs must preserve these identities from the beginning, even if early milestones only use a subset.

## Implemented foundation

The in-memory prototype now creates distinct semantic IDs for:
- Session;
- Input;
- Turn;
- Run;
- ConversationItem;
- ToolInvocation;
- Attachment.

IDs use readable semantic prefixes plus UUID identity. Prefixes are for diagnostics/type recognition; correctness still uses the actual ID field and owning relationships.

RuntimeOrchestrator currently:
- owns one active Session identity;
- creates one Input per accepted prompt;
- creates one Turn for that Input;
- creates generation-1 Run for the current provider execution;
- groups user/assistant ConversationItems by Turn ID;
- links the next linear Turn to the prior Turn as parent lineage;
- creates a new Session identity and clears prototype lineage on resetSession().

This is an in-memory proof only. Durability, ProviderAttempt identity, recovery generations and database constraints remain M3 work.
