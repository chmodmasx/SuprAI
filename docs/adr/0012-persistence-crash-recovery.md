# ADR-0012: Persistence worker and crash-safe side-effect journal

Status: accepted
Date: 2026-10-06

## Decision

All durable SuprAI state is owned through a persistence service.

SQLite write operations are serialized through a dedicated persistence worker/connection.

Baseline database settings:
- WAL journal mode;
- foreign keys enabled;
- explicit busy timeout;
- migrations with schema versioning;
- integrity checks in diagnostics/tests.

Do not expose arbitrary SQL to QML or agent components.

## Qt threading rule

A QSqlDatabase connection is used only from the thread/context that owns it.

The persistence worker owns the primary writer connection.

Read connections may be introduced later when measurements justify them, but each remains thread-affine.

## Side-effect journal

Tool invocations that may mutate external state are durable records before execution.

Lifecycle:

```text
prepared
  -> authorized
  -> executing
  -> succeeded | failed | cancelled

process crash while executing
  -> outcome_unknown
```

Before starting an external side effect:
1. persist invocation ID, tool identity, normalized arguments, policy decision and resource scope;
2. commit;
3. transition to executing;
4. commit;
5. execute.

After result:
1. persist result/error and effective execution metadata;
2. transition terminal state;
3. commit;
4. only then allow the agent turn to consume the result.

## Recovery rule

On startup, an invocation left in `executing` is not assumed to have failed.

For mutating/non-idempotent tools it becomes `outcome_unknown`.

SuprAI MUST NOT automatically replay such an invocation.

Read-only/idempotent tools may support an explicit safe-retry policy later.

## Provider request recovery

Model inference is not treated like an external mutating tool, but transparent retries are still bounded:
- before observable output: classified transport retry may be allowed;
- after output/tool-call visibility: do not silently restart and splice a second generation into the same attempt.

## FTS5

Canonical history has a separate FTS5 text index.

At startup/tests, verify FTS5 is actually enabled in the shipped SQLite build.

Do not assume compile-time features merely because QSQLITE is present.

## Why

SQLite WAL permits concurrent readers with one writer, but the writer remains serialized. Qt SQL connections are thread-affine.

More importantly, an application crash can occur after a side effect happened but before its result was persisted. Treating that as ordinary failure can duplicate destructive actions on retry.

## Consequences

- turns and tool effects become recoverable/inspectable;
- DB access is more disciplined;
- ambiguous outcomes are surfaced instead of guessed;
- future background execution can reuse the same journal.
