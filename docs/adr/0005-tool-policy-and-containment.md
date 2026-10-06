# ADR-0005: Tool authorization and OS containment are separate layers

Status: accepted
Date: 2026-10-06

## Decision

Every tool invocation passes through `PolicyEngine`.

Policy outcomes:
- allow;
- ask;
- deny.

OS containment is a separate `ContainmentBackend` capability.

Candidate backends:
- Landlock;
- bubblewrap;
- none.

The UI must never label execution as sandboxed when containment is `none`.

## Canonical tool metadata

Each tool declares:
- origin;
- input/output JSON Schema;
- risk classes;
- mutating/read-only status;
- filesystem scope requirements;
- network requirements;
- process requirements;
- privilege requirements;
- idempotence;
- parallel-safety.

Baseline risk classes:
- read_only;
- local_mutation;
- process_execution;
- network_access;
- credential_access;
- privilege_escalation;
- destructive.

## Policy scopes

Rules may match:
- tool;
- origin;
- project;
- filesystem path;
- command family;
- network host/domain;
- session;
- one-shot vs persisted grant.

## Process execution baseline

When possible:
- minimal/explicit environment;
- explicit cwd;
- separate process group/session;
- timeout;
- output limits;
- descendant cleanup on cancellation;
- workspace-scoped filesystem access;
- network treated as an independent authority.

## Why

User approval changes authorization, not kernel authority. A host shell still has the user's ambient filesystem/process/network privileges unless containment is applied.

bubblewrap is a low-level sandbox constructor rather than a complete security policy and may be constrained by distribution user-namespace policy. Landlock can provide unprivileged self-restriction but must be feature-probed.

## Consequences

Security UI must expose effective policy and containment separately.
