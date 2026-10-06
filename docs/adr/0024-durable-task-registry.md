# ADR-0024: Unified durable Task registry

Status: accepted
Date: 2026-10-06

## Decision

SuprAI has one durable TaskManager/Task registry for asynchronous work.

Task is an execution/activity record, not a Session replacement.

Sources may include:
- subagent;
- background process;
- MCP task;
- scheduled run;
- remote worker;
- future long-running media/build operations.

## States

Canonical Task states:

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

Terminal:
- succeeded;
- failed;
- timed_out;
- cancelled;
- lost;
- outcome_unknown.

### lost
The authoritative executor disappeared and the outcome cannot be discovered after reconciliation/grace.

### outcome_unknown
An external mutating operation may already have happened and replay is unsafe.

## Core fields

At minimum:

```text
task_id
source
status
requester_session_id?
requester_turn_id?
requester_run_id?
child_session_id?
active_run_id?
external_handle?
created_at
queued_at?
started_at?
updated_at
ended_at?
timeout_at?
owner_instance_id?
owner_generation
progress?
summary?
result_ref?
error?
notification_policy
delivery_status
durability_class
idempotency_key?
```

## Execution vs delivery

Task execution state and result-delivery state are independent.

Delivery states:
- not_requested;
- pending;
- delivered;
- failed;
- incomplete;
- suppressed.

A successfully completed Task remains `succeeded` even if notification/delivery fails.

## Notifications

Per-task notification policy:
- done_only;
- state_changes;
- silent.

Canonical state is always inspectable regardless of notification policy.

## Cancellation

`cancel_requested` is not terminal.

Transition to `cancelled` only after authoritative executor settlement where that is knowable.

External/MCP cancellation may be refused or unconfirmable.

## No model polling

TaskManager, not the model, watches external progress/completion.

For MCP Tasks it:
- stores opaque server task ID separately;
- honors server poll interval;
- consumes push notifications when available;
- routes `input_required` into SuprAI user-action machinery.

## MCP mapping

MCP `io.modelcontextprotocol/tasks` is one Task source.

MCP task IDs are never SuprAI task IDs.

## Why

OpenClaw's current background-task registry explicitly separates task state, session context and delivery. Goose is independently moving toward the same shared substrate because siloed async mechanisms lead to polling and lost completions.
