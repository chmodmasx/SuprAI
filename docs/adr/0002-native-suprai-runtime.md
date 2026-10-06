# ADR-0002: Native SuprAI agent runtime from the start

Status: accepted
Date: 2026-10-06

## Decision

SuprAI's production agent runtime is implemented by SuprAI itself.

The canonical production implementation is `NativeSuprAIRuntime`.

Hermes Agent, OpenClaw and similar systems are research references only. They are not planned runtime dependencies, adapters, gateways or production backends.

A deterministic `MockRuntime` exists only for tests and UI development.

## Context

The product goal is not to build a native frontend for Hermes. The goal is to build a native Linux AI application with its own agent behavior.

Using Hermes as the first real backend would optimize for short-term functionality at the cost of defining SuprAI around another project's runtime semantics. That would postpone the hardest and most product-defining work—agent loop, tools, approvals, persistence, context management, MCP and memory—and make later replacement harder.

Upstream projects are still valuable because they expose solved engineering problems and failure modes.

## Consequences

Positive:
- SuprAI owns its agent semantics from the beginning;
- no runtime dependency on Hermes/OpenClaw;
- model/provider integration can target local OpenAI-compatible servers directly;
- tools, MCP, memory, context and sessions evolve as one coherent design;
- desktop and agent behavior can be optimized together.

Costs:
- more engineering work earlier;
- agent-loop correctness, persistence and tool security must be implemented and tested by us;
- maturity will arrive more gradually than by wrapping an existing agent.

## Architecture

```text
QML UI
   |
C++ Application
   |
AgentRuntime
   |--------------------|
NativeSuprAIRuntime   MockRuntime
   |
AgentLoop
   + ProviderRegistry
   + ContextManager
   + ToolRegistry
   + ApprovalManager
   + MCPClientManager
   + SessionStore
   + MemoryService
```

## Hard rules

- Production operation must not require Hermes or OpenClaw.
- Do not add a Hermes/OpenClaw runtime adapter without a new ADR.
- Provider transports are not agent runtimes.
- Agent policy must not live in QML or provider classes.
- NativeSuprAIRuntime remains the canonical source of agent behavior.
- External source code may be studied, but copied/adapted code must respect its license and be independently reviewed.
