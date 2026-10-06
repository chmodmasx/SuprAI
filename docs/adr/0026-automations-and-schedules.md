# ADR-0026: Schedule definitions are separate from Task executions

Status: accepted
Date: 2026-10-06

## Decision

A schedule/automation is a durable trigger definition.

Each firing creates a unique occurrence and Task.

```text
Schedule
  -> Occurrence
  -> Task
  -> Session/Turn/Run
```

Do not store a recurring schedule as a permanently-running Task.

## Schedule fields

At minimum:
- schedule_id;
- name;
- trigger;
- timezone;
- task brief/prompt;
- target profile/model;
- requested capability envelope;
- result delivery target;
- enabled;
- misfire policy;
- overlap policy.

Each occurrence has a stable identity.

Recommended idempotency key:
`schedule_id + scheduled_occurrence_time`.

## Misfire policy

Explicit choices:
- skip;
- run_once;
- catch_up_limited(N).

Do not silently execute an unlimited backlog after downtime.

## Overlap policy

Explicit choices:
- forbid_overlap;
- allow_overlap;
- replace_previous.

Default: `forbid_overlap`.

## Execution context

Default scheduled agent execution uses a fresh isolated task Session.

Result delivery may target an interactive Session/channel/UI, but the scheduled run should not silently mutate that interactive transcript as if it were a user turn.

## Authority

Creation captures requested capability intent, not permanent authorization.

At every execution:

```text
captured requested capability envelope
INTERSECT
current system/project policy
INTERSECT
currently available tools
```

Existing policy changes can reduce or block a later scheduled run.

## Recursion

Scheduled Tasks cannot create further schedules by default.

This can be relaxed later through explicit policy.

## Why

Hermes runs scheduled work in fresh sessions. OpenClaw persists schedule definitions separately from run/task history. Both support separating "when to start work" from "the work execution itself."
