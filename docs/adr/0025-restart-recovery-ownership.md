# ADR-0025: Recovery is owner-generation aware and replay-safe

Status: accepted
Date: 2026-10-06

## Decision

Persisted `running` state alone is never evidence that work is still executing.

Active Runs/Tasks carry execution ownership metadata:
- runtime_instance_id;
- owner_generation;
- executor-specific external handle;
- PID/process-start identity where useful;
- lease/last-seen metadata where appropriate.

A recovery generation fences older executors/events.

## Startup reconciliation

For every non-terminal Run/Task:
1. inspect persisted owner;
2. determine whether that exact owner/executor is still authoritative;
3. reconcile executor-specific state;
4. choose recover, wait, lost, outcome_unknown, or requires_action;
5. persist the decision before resuming model work.

## Foreground recovery

### interrupted pure inference
A new Run generation may rebuild from canonical conversation state.

Partial prior model output remains explicitly incomplete.

### durable completed ToolResult
Reuse; never re-execute.

### interrupted read-only/idempotent tool
May be eligible for policy-controlled retry.

### interrupted mutating/non-idempotent tool
If execution started but no durable terminal outcome exists:
- mark `outcome_unknown`;
- do not replay automatically;
- require reconciliation/user decision.

### external task with durable handle
Query the external authority.

### process-local task with dead owner
Mark `lost` when no authoritative result can be recovered.

## Recovery attempts

Persist:
- recovery_attempt_count;
- recovery_generation;
- last_recovery_at;
- last_recovery_error;
- recovery_blocked flag.

Recovery loops are bounded.

Repeated failure transitions to operator/user review rather than infinite automatic retries.

## Freshness

Each Run/Task class declares a freshness/resume policy.

Do not automatically wake arbitrary stale work after a long outage.

## Graceful shutdown

Order:
1. fence new admission;
2. persist recoverable/interrupted markers;
3. bounded drain;
4. settle terminal persistence;
5. cancel remaining in-process work;
6. flush DB;
7. stop worker threads/process.

Drain success requires terminal result persistence, not merely executor completion.

## Why

OpenClaw's current restart-recovery design demonstrates the need for exact ownership, generations, bounded retries and durable completion custody. SuprAI's ADR-0012 already requires ambiguous mutating tool effects to avoid automatic replay; this ADR generalizes the rule to all executable work.
