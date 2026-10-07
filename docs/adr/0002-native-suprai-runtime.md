# ADR-0002: Native SuprAI runtime with separated execution kernel

Status: accepted  
Date: 2026-10-06  
Updated: 2026-10-07

## Decision

SuprAI's production agent runtime is implemented by SuprAI itself.

The canonical production implementation is `NativeSuprAIRuntime`.

Inside that implementation, stateful orchestration and low-level agent execution are separate responsibilities:

```text
AgentRuntime interface
        |
        +-------------------+
        |                   |
        v                   v
NativeSuprAIRuntime      MockRuntime
        |
        v
RuntimeOrchestrator
        |
        v
AgentEngine
```

`MockRuntime` exists only for deterministic tests and UI development.

Hermes Agent, OpenClaw, Cline and similar systems are research references only. They are not planned runtime dependencies, gateways or production backends.

## Context

The product goal is not to build a native frontend for another agent.

Using an existing agent runtime as the first real backend would optimize for short-term functionality at the cost of defining SuprAI around another project's semantics. That would postpone the product-defining work—agent execution, tools, approvals, persistence, context management, MCP, tasks and memory—and make later replacement harder.

At the same time, implementing every responsibility directly inside one `NativeSuprAIRuntime` object would create the same long-term coupling problem internally.

Current Cline architecture reinforces a useful split: the agent execution loop can remain relatively small while stateful session/application orchestration owns persistence, lifecycle and host concerns.

## RuntimeOrchestrator

`RuntimeOrchestrator` owns stateful SuprAI execution semantics around the engine.

Responsibilities include:
- Session/Input/Turn/Run ownership;
- durable identity and lineage;
- persistence boundaries;
- queued input and steering;
- user-action/approval lifecycle;
- TaskManager and subagent coordination;
- recovery/owner-generation fencing;
- ContextManager policy;
- memory/project integration;
- translating engine events into application/domain events.

It depends on ports/contracts rather than concrete provider/database/platform implementations.

## AgentEngine

`AgentEngine` is the comparatively stateless execution kernel for one active Run.

Responsibilities include:
- execute the provider/tool iteration loop;
- request provider inference through a provider port;
- consume normalized inference events;
- assemble tool-call execution batches from already-authorized/scheduled work;
- receive tool results;
- enforce iteration/completion limits;
- implement low-level cancellation boundaries;
- emit typed `AgentEngineEvent` values.

It does not own:
- SQLite/session persistence;
- application UI;
- project/session history;
- provider wire protocols;
- Task registry;
- long-lived memory;
- Linux desktop integration.

The engine may receive prepared context/request data from orchestration/context services. Provider-specific request projection still terminates at the provider boundary.

## Event boundary

Engine events are not the UI contract.

```text
AgentEngineEvent
      |
      v
RuntimeEventAdapter
      |
      v
SuprAI domain/application event
      |
      v
UI projection / QAbstractListModel
```

QML never consumes provider stream objects or `AgentEngineEvent` implementation details directly.

## Interceptors and observers

Execution-critical interception is distinct from observation.

Blocking interceptors may:
- validate or transform a provider request;
- enforce policy;
- request approval;
- validate/transform a tool result.

They require explicit ordering, timeout and cancellation semantics.

Observers:
- UI;
- telemetry;
- diagnostics;
- logging;
- activity views;

must not block provider streaming or tool execution correctness. High-frequency deltas may be queued/coalesced at observer boundaries.

## Consequences

Positive:
- SuprAI owns its agent semantics from the beginning;
- no runtime dependency on Hermes/OpenClaw/Cline;
- NativeSuprAIRuntime does not become a god object;
- execution-kernel tests can run without persistence/UI;
- session/recovery/task behavior can evolve without rewriting the engine;
- provider and host adapters stay replaceable;
- event translation creates a clean boundary for Qt/QML and future headless/remote clients.

Costs:
- more explicit interfaces and adapters;
- orchestration/engine mapping code;
- careful ownership rules are required to avoid duplicating state between engine and orchestrator.

## Hard rules

- Production operation must not require a third-party agent runtime.
- Do not add a Hermes/OpenClaw/Cline runtime adapter without a new ADR.
- Provider transports are not agent runtimes.
- Agent policy must not live in QML or provider classes.
- `RuntimeOrchestrator` is the stateful authority around Runs; `AgentEngine` is not a persistence/session god object.
- Engine events are translated before presentation.
- Observational work must never be placed synchronously in the token-stream critical path.
- External source code may be studied, but copied/adapted code must respect its license and be independently reviewed.
