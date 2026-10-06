# ADR-0028: Ephemeral deliberation branches / context folding

Status: proposed
Date: 2026-10-06

## Proposal

Add a SuprAI-owned ephemeral context-isolation mechanism for reasoning-heavy turns.

A Turn may execute a `DeliberationBranch` derived from a canonical parent ContextSnapshot.

The branch:
- is not a canonical user-visible Session;
- may reason/call tools in its own temporary context;
- owns ephemeral raw reasoning state;
- returns a compact structured `ReturnCapsule`;
- is discarded after merge unless explicit diagnostics retention is enabled.

Canonical history receives only approved derived state/final answer, not the full branch trace.

## Motivation

Reasoning-heavy local models can generate large scratch traces. Even when reasoning is hidden from UI, replaying it into future prompts consumes context and can degrade long-horizon operation.

Context Folding research shows that temporary sub-trajectories can be folded back into concise outcomes while preserving a much smaller main active context.

## Important limitation

This does not make reasoning tokens disappear from the branch's own active context.

If the branch starts from a full `P`-token parent snapshot and generates `R` tokens, its peak remains approximately `P + R`.

Therefore branch context construction supports:
- full_snapshot;
- scoped_snapshot;
- compacted_snapshot.

The final default requires measurement.

## ReturnCapsule

Proposed fields:

```text
goal
facts[]
hypotheses[]
decisions[]
constraints[]
unresolved[]
next_steps[]
evidence_refs[]
artifacts[]
final_answer?
```

Facts/hypotheses retain provenance and must not silently become trusted memory.

## Provider independence

The mechanism is SuprAI-owned.

Provider capabilities such as NInfer `preserve_thinking` may optimize history rendering but are not required for correctness.

Generic OpenAI-compatible providers remain supported.

## NInfer integration

When NInfer exposes `preserve_thinking`, SuprAI should explicitly request the desired behavior rather than depend on artifact/template defaults.

Current NInfer templates demonstrate a useful pattern:
- closed historical reasoning can be omitted;
- reasoning within the active multi-step tool turn can remain available.

This is complementary to, not a replacement for, DeliberationBranch.

## Proof required before acceptance

Implement a controlled prototype comparing:
1. baseline raw reasoning retained;
2. raw reasoning omitted after each closed turn;
3. full-snapshot deliberation branch;
4. scoped/compacted deliberation branch.

Test at least:
- NInfer;
- llama.cpp;
- vLLM;
- reasoning-heavy local models;
- tool-free chat;
- multi-tool agent work;
- long context near capacity.

Measure:
- peak input+live output context;
- future canonical prompt size;
- task quality;
- reasoning repetition;
- tool error rate;
- latency;
- VRAM/KV pressure;
- summary loss/recovery.

## Acceptance condition

Accept only if context isolation materially reduces canonical/long-horizon context growth without unacceptable quality loss, and the runtime can explain clearly which state was ephemeral vs canonical.
