# ADR-0004: Agent turns use an explicit state machine

Status: accepted
Date: 2026-10-06  
Updated: 2026-10-07

## Decision

AgentEngine turn execution is modeled as explicit states, transitions and effects. NativeSuprAIRuntime exposes the production facade; RuntimeOrchestrator owns surrounding durable/session lifecycle.

Baseline states:

```text
Created
PreparingContext
RequestingModel
StreamingModel
EvaluatingToolCalls
AwaitingApproval
SchedulingTools
ExecutingTools
RecordingToolResults
Finalizing
Completed
Cancelling
Cancelled
Failed
```

The exact implementation may refine these names but must preserve explicit transition semantics.

## Rules

- reducers/state transitions do not perform external I/O;
- effects perform provider/tool I/O through explicit ports; durable persistence checkpoints are coordinated by RuntimeOrchestrator around the engine effects;
- every turn has a durable ID;
- every provider attempt has an attempt ID;
- every tool invocation has a durable invocation ID;
- stale events from superseded attempts are ignored;
- terminal transitions are idempotent;
- external side effects have persistence boundaries before invocation and after result;
- retry policy classifies errors rather than blindly resending.

## Parallel tool calls

Parallel model output is only scheduling input.

A tool may execute concurrently only when:
- its metadata says it is parallel-safe;
- policy permits it;
- declared resource scopes do not conflict;
- cancellation/result ordering remains deterministic.

## Why

Mature agents accumulate tool approval, retries, compaction, cancellation, subagents, steering and background execution. A monolithic or recursive tool loop becomes difficult to reason about and test.

Goose's current agent architecture is one useful reference: it separates these concerns into explicit state-machine operations.

## Consequences

State transition tests become a primary correctness suite. UI observes state/events but does not drive internal transitions directly.
