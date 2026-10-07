# ADR-0028: Isolated deliberation reuses subagent infrastructure

Status: accepted
Date: 2026-10-06

## Decision

SuprAI will not create a separate `standalone deliberation-branch` runtime subsystem.

Reasoning isolation/context folding is implemented by the existing subagent architecture defined in ADR-0023:

```text
Task(source=subagent, purpose=deliberation)
  -> child Session
      -> child Turn/Run
          -> ReasoningWorkspace
          -> optional tools
          -> ReturnCapsule
```

The child may be product-ephemeral and invisible in the normal chat UI.

Only the compact result required by the parent is merged into the parent context by default.

## Why

The proposed standalone deliberation branch and the existing subagent model need almost the same machinery:
- isolated context;
- child lifecycle;
- cancellation;
- scheduling;
- permissions;
- tool access;
- persistence/recovery;
- result routing;
- compact return;
- parent yield/resume.

Implementing both would duplicate architecture and create divergent semantics.

A subagent specialization gives us Context Folding behavior without a second orchestration system.

## Purpose is not runtime type

`deliberation` is a subagent purpose/profile.

Other purposes may include:
- delegation;
- verification;
- research;
- coding.

All use NativeSuprAIRuntime.

Do not create:
- standalone deliberation-branch manager;
- separate reasoning-agent runtime;
- separate branch persistence lifecycle.

## Context isolation

A deliberation child receives one of:

```text
full
scoped
compacted
```

### full

Use a full parent snapshot when:
- context is small;
- exact continuity matters;
- enough child reasoning headroom remains.

### scoped

Use selected parent context:
- current request;
- relevant canonical items;
- project constraints;
- selected evidence/artifacts;
- selected memory/tool state.

This is likely the preferred mode for large conversations.

### compacted

Use a derived compacted snapshot when the parent is already under context pressure.

Compaction follows ADR-0019 and never replaces canonical history.

## Critical limitation

Subagent isolation does not make reasoning tokens disappear from the child's own model context.

If:
- `P` = child starting context;
- `R` = child reasoning/tool trace;

then child peak is still approximately:

```text
P + R
```

The architecture reduces parent/future-context pollution, not the transformer context requirement of the active child itself.

Therefore context mode and reasoning/output headroom remain necessary.

## ReasoningWorkspace

`ReasoningWorkspace` is ephemeral scratch state inside the child Run.

It may contain:
- provider-separated reasoning output;
- temporary hypotheses;
- planning;
- provisional conclusions;
- provider-native reasoning state;
- temporary reasoning/tool metadata.

It does not become canonical parent history.

Default lifecycle:

```text
create with Run
use during child work
derive ReturnCapsule
discard raw workspace
```

Debug retention, if implemented, is separate from canonical context and must be opt-in.

## ReturnCapsule

A deliberation child should return compact structured state, not a raw chain-of-thought dump.

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

Requirements:
- distinguish facts from hypotheses;
- retain provenance;
- reference canonical evidence where possible;
- do not convert model speculation into trusted memory;
- keep output small enough to justify isolation.

The exact schema remains implementation/benchmark driven.

## Merge modes

### return_state_then_continue

The parent receives the ReturnCapsule and performs another inference.

Useful for:
- synthesis;
- comparing multiple children;
- verification;
- arbitration.

### return_answer

The child returns a compact state plus final answer and the parent can accept/display it without an extra inference.

Useful when:
- the child was effectively the only reasoner;
- another parent call would only duplicate work.

The runtime policy chooses explicitly.

## Deliberation policy

Do not spawn a deliberation child for every turn.

Conceptual policy:

```text
direct
isolated
auto
```

### direct
Parent model reasons/responds normally.

### isolated
Use a deliberation child.

### auto
Runtime chooses based on signals such as:
- requested reasoning effort;
- model reasoning behavior/profile;
- context pressure;
- expected task horizon;
- number/type of tools;
- verification/research need;
- user preference.

The automatic policy requires measurements before exact thresholds are accepted.

## Provider/model selection

The child may use:
- same model/provider as parent;
- same model with higher reasoning effort;
- different configured model/profile later.

Example:

```text
Parent:
  reasoning = low

Deliberation child:
  reasoning = high/xhigh
```

This does not imply actual simultaneous inference.

ExecutionScheduler decides physical execution according to provider/GPU constraints.

## Security

Deliberation children are read-mostly by default.

Recommended default authority:
- read selected files/evidence: allowed by policy;
- retrieval/search: allowed by policy;
- file mutation: deny;
- arbitrary shell mutation: deny;
- external messaging: deny;
- persistent memory writes: deny;
- schedule creation: deny;
- privilege escalation: deny;
- recursive child spawning: deny.

A need for deep reasoning must never imply broader side-effect authority.

## NInfer

NInfer-specific `preserve_thinking` is complementary.

When supported, SuprAI can explicitly request closed-turn reasoning omission.

However:
- defaults vary by loaded template/artifact;
- generic OpenAI-compatible providers may not support this extension;
- SuprAI must remain correct without it.

The canonical isolation mechanism is the child Session/context boundary, owned by SuprAI.

## Local-model constraints

SuprAI is local/OpenAI-compatible first.

Logical child concurrency does not mean multiple large local model generations run simultaneously.

ExecutionScheduler may serialize deliberators/subagents when the configured provider/model/GPU cannot support useful concurrent generation.

Do not assume multi-GPU hardware.

## Proof requirements

Benchmark at least:
1. direct reasoning;
2. direct reasoning with closed-turn reasoning omitted;
3. deliberation child with full context;
4. deliberation child with scoped context;
5. deliberation child with compacted context.

Across:
- NInfer;
- llama.cpp;
- vLLM;
- reasoning-heavy local models;
- tool-free chat;
- multi-tool agent work;
- long conversations near context capacity.

Measure:
- child peak context;
- parent canonical prompt growth;
- quality/task success;
- repeated reasoning;
- latency;
- tool correctness;
- VRAM/KV pressure;
- ReturnCapsule information loss;
- extra inference overhead.

## Consequences

Accepted:
- no standalone deliberation-branch architecture;
- deliberation is a subagent purpose;
- ReasoningWorkspace is Run-local ephemeral state;
- ReturnCapsule is the parent merge boundary;
- child context supports full/scoped/compacted modes;
- provider-specific reasoning controls remain optional optimizations.

Proof-gated:
- automatic spawn thresholds;
- default context mode;
- exact ReturnCapsule schema;
- whether return_answer or return_state_then_continue should be the common default.
