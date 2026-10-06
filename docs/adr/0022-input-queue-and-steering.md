# ADR-0022: Queued input and steering have explicit semantics

Status: accepted
Date: 2026-10-06

## Decision

Inputs arriving while a Turn is active use an explicit disposition.

Supported semantic modes:

- steer;
- followup;
- collect;
- interrupt.

The initial UI may expose fewer modes, but persistence/runtime types reserve them.

## Steer

Steering modifies future decisions in the active Turn at a safe boundary.

It does NOT:
- terminate an already-running tool;
- widen the active Turn's policy/capabilities;
- mutate already-sent model context;
- silently create a new Session.

### Safe boundaries

For sequential tools:
1. allow the already-running call to settle;
2. check steering before each later unstarted call;
3. when steering is consumed, mark old unstarted calls `skipped_by_steering`;
4. create synthetic terminal ToolResults for skipped calls;
5. append steering input after tool results;
6. issue the next model request.

A parallel tool batch already admitted settles as a batch before steering.

## Steer disposition

Track separately:

- accepted;
- delivered;
- missed;
- rejected;
- converted_to_followup.

Queue acceptance is not proof the model/runtime consumed the steer.

## Followup

Preserve input for a later Turn without modifying active work.

## Collect

Coalesce compatible queued follow-ups after a debounce/quiet window.

## Interrupt

Request active Turn cancellation, then start a later Turn for the new Input after cancellation/reconciliation.

## Priority

Recommended queue priority:
1. explicit cancel/operator control;
2. approval/clarification response;
3. user steering/correction;
4. required child/task completion;
5. ordinary follow-up;
6. low-priority internal progress.

FIFO is preserved within a class. Add aging/fairness to prevent starvation.

## Structural invariant

Conversation history remains append-oriented and every requested tool call gets a terminal result, including calls skipped because of steering.

## Why

OpenClaw's current steering model demonstrates that arbitrary tool preemption breaks transcript structure and creates ambiguous side effects. Hermes similarly distinguishes queued vs actually delivered child steering.
