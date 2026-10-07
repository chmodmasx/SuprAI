# ADR-0005: Tool authorization and OS containment are separate layers

Status: accepted
Date: 2026-10-06  
Updated: 2026-10-07

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


## Approval identity

Every interactive approval is bound to an exact action identity.

At minimum, where applicable:
- session_id;
- turn_id;
- run_id;
- task_id;
- child_session_id;
- tool_invocation_id;
- user_action/request ID.

A generic "approve the current thing" mechanism is forbidden.

Approving delegation to a child/subagent does not imply blanket approval for unrelated child-side mutations. Child authority remains the intersection defined by the subagent/task policy.

When a session/run/tool invocation is replaced, cancelled or invalidated, any pending approval owned by it must receive an explicit terminal resolution.

## Prepared mutation contract

For file/workspace mutations, the preferred contract is:

```text
prepare normalized ChangeSet
        |
validate expected base/version
        |
preview
        |
policy / approval
        |
revalidate base/version
        |
apply the exact prepared ChangeSet
        |
persist actual result/checkpoint
```

Approval binds to the prepared change identity/hash and its expected base state.

If the target changed between preview and apply:
- do not apply the stale approved mutation;
- invalidate the approval;
- reprepare/repreview/reapprove as required.

The preview is presentation, not authority. A failed preview must not silently authorize or execute a mutation.

Auto-approved edits still record the actual applied change.

## Output and execution bounds

Every process/tool must have independent limits for:
- in-memory/live output;
- UI event backlog;
- model-facing result projection;
- persisted artifact/log retention;
- execution timeout where appropriate.

Truncation is explicit.

A tool's declaration that it is parallel-safe is necessary but not sufficient for concurrent execution. Policy, resource/path conflicts, cancellation semantics and scheduler capacity determine actual concurrency.
