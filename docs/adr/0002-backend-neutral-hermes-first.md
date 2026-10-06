# ADR-0002: Backend-neutral desktop, Hermes-first adapter

Status: accepted
Date: 2026-10-06

## Decision

SuprAI owns an `AgentBackend` abstraction.

The first production-grade backend integration should be Hermes Agent through its gateway JSON-RPC/WebSocket protocol.

Hermes is an adapter, not SuprAI's domain model.

A deterministic MockBackend must be implemented before Hermes integration.

## Context

Rebuilding agent sessions, tools, approvals, memory, skills, profiles and robust streaming at the same time as creating the desktop would multiply risk and make UI bugs inseparable from agent-runtime bugs.

Hermes already exposes the required classes of behavior through a headless gateway and has explicit local/remote operation.

The long-term product should still be capable of a native SuprAI agent runtime.

## Consequences

Positive:
- useful desktop can arrive before a new agent engine;
- UI architecture can be tested against a mature runtime;
- later NativeSuprAIBackend can replace or coexist with Hermes;
- backend contract becomes testable through MockBackend.

Costs:
- an adapter/translation layer is required;
- Hermes protocol compatibility must be versioned/probed;
- some Hermes features may not map 1:1 to SuprAI concepts.

## Hard rules

- No Hermes RPC method names in QML.
- No Hermes JSON types in QML.
- No UI feature may branch on backend name when a capability bit can express the behavior.
- Remote/local execution location must be explicit.
- Backend failure must not crash the shell.
