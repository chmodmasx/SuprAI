# ADR-0015: Explicit thread ownership

Status: accepted
Date: 2026-10-06  
Updated: 2026-10-07

## Decision

SuprAI uses explicit QObject/thread ownership.

Baseline topology:

```text
Main/UI thread
  QApplication
  QQmlApplicationEngine
  UI-facing models/controllers
        |
        | queued signals/commands
        v
Runtime thread
  NativeSuprAIRuntime facade
  RuntimeOrchestrator
  AgentEngine / turn state machine
  provider network objects
  MCP/process coordination
        |
        | queued persistence requests
        v
Persistence thread
  PersistenceWorker
  primary QSQLITE writer connection
```

Heavy pure computations may use QThreadPool/QtConcurrent only when measured.

## Rules

- QML-visible models live on the UI thread.
- NativeSuprAIRuntime/runtime worker ownership is moved to a dedicated QThread with an event loop; RuntimeOrchestrator and AgentEngine runtime QObjects remain on that runtime thread unless a later ADR proves a separate execution thread is needed.
- QNetworkAccessManager/replies, QProcess and timers used by the runtime are created/used in their owning thread.
- PersistenceWorker owns its QSqlDatabase connection.
- Cross-thread interaction uses queued signals/slots or immutable value commands/events.
- No UI code dereferences runtime-owned QObject pointers.
- No BlockingQueuedConnection in ordinary runtime paths.
- shutdown is ordered: stop accepting work -> cancel/drain runtime -> flush persistence -> stop worker threads -> destroy UI/application.

## Why

Qt QObject subclasses have thread affinity. Network/process/timer/database objects are not safe to call arbitrarily across threads.

Keeping runtime work off the UI thread also prevents context building, parsing, token accounting or accidental blocking operations from causing visible stalls.

## Event delivery

High-frequency stream deltas may be coalesced at the UI boundary. UI/telemetry/log observers must not be synchronously awaited by the provider token-stream path.

Coalescing must preserve:
- item order;
- UTF-8/text correctness;
- terminal event ordering;
- tool/reasoning channel separation.

State transitions themselves are never dropped.

## Testing

Add:
- thread-affinity assertions in debug builds;
- shutdown-under-active-stream test;
- cancellation race tests;
- runtime event after UI/session switch tests;
- persistence shutdown/flush test.
