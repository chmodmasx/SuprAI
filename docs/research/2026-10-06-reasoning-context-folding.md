# Reasoning Context Isolation / Context Folding Review — 2026-10-06

Status: current research baseline
Purpose: define how SuprAI can isolate reasoning-heavy work without polluting the canonical conversation.

## Executive conclusion

The original idea—copy the current chat into a temporary context, reason there, return only a compact result, then discard the temporary context—has close prior art in **Context Folding / Sub-Trajectory Folding**.

After comparing that idea with SuprAI's existing subagent architecture, the current conclusion is:

> Do not build a separate DeliberationBranch subsystem. Reuse subagents as the execution substrate for isolated deliberation.

Canonical shape:

```text
Parent Session
  -> Parent Turn
      -> Task(source=subagent, purpose=deliberation)
          -> Child Session
              -> Child Turn/Run
                  -> ReasoningWorkspace
                  -> optional read/search/tools
                  -> ReturnCapsule
      -> Parent receives compact derived result
```

The child can be invisible/ephemeral at product level while still using the same lifecycle, cancellation, scheduler, policy, recovery and routing infrastructure as any other subagent.

This avoids duplicate orchestration systems.

## 1. Closest prior art: Context Folding

Paper:
- Scaling Long-Horizon Agent via Context Folding, ICML 2026.
- Code: sunnweiwei/FoldAgent / MiaoLu3/Context_Folding.

The mechanism uses:
- `branch(description, prompt)`: create a temporary sub-trajectory;
- `return(message)`: collapse that trajectory and rejoin the main trajectory with a compact result.

The released FoldAgent implementation creates a branch Agent from the current main-agent history, executes independently, and appends only the returned branch result to the main agent.

That is very close to:

```text
copy current context
-> think/use tools in temporary branch
-> return compact state
-> discard branch trace
```

Important caveat:
The strongest published results include training that teaches the model when/how to branch and return. The isolation mechanism itself can still be implemented at harness level, but arbitrary local models should not be expected to manage the policy optimally without orchestration.

## 2. Training-free relevance

Training-free systems such as MM-ContextFold reinforce the same design pattern:
- compact persistent main context;
- temporary branch contexts for expensive work;
- return a concise textual result;
- discard branch-local trace/data.

Therefore context folding is useful as a runtime/harness architecture even when SuprAI does not train the model specifically for it.

## 3. Why subagents are the right SuprAI substrate

SuprAI already defines a subagent as:

```text
Task(source=subagent)
  + child Session
  + child Turn/Run
  + parent/requester binding
```

A separate deliberation-branch subsystem would need the same:
- context isolation;
- lifecycle;
- model invocation;
- tool access;
- policy;
- cancellation;
- concurrency scheduling;
- persistence/recovery;
- result routing;
- parent yield/resume.

That duplication would be architectural debt.

Instead, add purpose/profile semantics:

```text
SubagentPurpose
  delegation
  deliberation
  verification
  research
  coding
```

All still run through NativeSuprAIRuntime.

## 4. Deliberation subagent

A deliberation child exists primarily to think, compare, verify, plan, synthesize, or explore without carrying its raw scratch trace into the parent conversation.

Example:

```text
Parent:
  user request
  reasoning effort = low

Child deliberator:
  same local model
  reasoning effort = high/xhigh
  read/search authority only
  raw reasoning remains child-local
  returns ReturnCapsule
```

The child may use the same OpenAI-compatible provider/model as the parent. It does not require a second loaded model.

ExecutionScheduler decides actual physical concurrency.

With one large model on one GPU, logically concurrent children may execute serially.

## 5. Critical distinction: parent-context footprint vs child peak

Let:
- `P` = child starting context;
- `R` = raw reasoning/tool trace;
- `S` = compact ReturnCapsule.

Without isolation:

```text
future parent context ~= P + R + answer
```

With a deliberation child:

```text
child peak ~= P + R
future parent context ~= parent + S + answer
```

The main benefit is that `R` is not inherited by future parent turns.

However:

> The child still has a finite context window.

If:
- parent/full snapshot = 180k;
- child reasoning = 100k;
- model context = 262k;

then a full-copy child still overflows.

Subagent isolation solves canonical/future-context pollution. It does not make the active transformer's context infinite.

## 6. Child context modes

To control child peak, use:

```text
full
scoped
compacted
```

### full

Copy/reference the full relevant parent snapshot.

Use when:
- parent context is small;
- exact continuity matters;
- sufficient reasoning headroom remains.

### scoped

Provide only:
- current request;
- selected prior items;
- relevant project instructions;
- selected memory;
- selected files/artifacts;
- required tool state.

Likely best default when parent history is already large.

### compacted

Provide a derived compacted snapshot governed by ADR-0019.

Use when:
- parent context is near capacity;
- the child needs broad history but cannot afford the full transcript.

Canonical history remains untouched.

## 7. ReasoningWorkspace

`ReasoningWorkspace` remains useful, but it is not the context-isolation mechanism.

Definition:

> Ephemeral reasoning/scratch state inside one Run.

May contain:
- raw provider-separated reasoning;
- temporary plans;
- hypotheses;
- provisional conclusions;
- tool-planning state;
- provider-native reasoning metadata/state.

Hierarchy:

```text
Subagent Child Session
  -> Turn
      -> Run
          -> ReasoningWorkspace
```

A normal parent Run can also own a ReasoningWorkspace.

The difference is that a deliberation child gives that workspace an isolated child context.

By default raw workspace content is discarded after it is no longer needed.

## 8. ReturnCapsule

Do not return raw chain-of-thought to the parent.

Return compact structured derived state.

Candidate:

```yaml
goal:
facts:
hypotheses:
decisions:
constraints:
unresolved:
next_steps:
evidence_refs:
artifacts:
final_answer:
```

Rules:
- facts retain provenance;
- hypotheses remain hypotheses;
- model speculation does not silently become trusted memory;
- evidence should reference canonical tool/file/artifact records;
- the capsule must stay much smaller than the branch trace or the isolation loses value.

## 9. Merge modes

### A. return_state_then_continue

```text
child
  -> ReturnCapsule

parent
  -> consumes capsule
  -> final synthesis/verification
  -> answer
```

Best when:
- multiple children need arbitration;
- parent should verify;
- parent owns final response policy.

### B. return_answer

```text
child
  -> ReturnCapsule + final_answer

parent
  -> accepts/persists/displays result
```

Best when:
- the child is effectively the primary reasoner;
- another parent inference would merely repeat work.

Exact default remains proof-gated.

## 10. Multiple reasoners

The architecture naturally supports:

```text
               Parent
      ┌──────────┼──────────┐
      v          v          v
 Deliberator  Deliberator  Verifier
 architecture risks        critique
      │          │          │
      └──────────┼──────────┘
                 v
          compact capsules
                 │
                 v
               Parent
```

The parent receives only compact results, not every raw reasoning trace.

However, this must remain bounded:
- max child depth;
- max logical concurrency;
- provider/model concurrency budget;
- global task budget;
- latency/compute budget.

Do not create recursive agent explosions.

## 11. Worker vs deliberation authority

### Worker/delegation child

May have explicit mutation/process capabilities depending on the task.

### Deliberation child

Read-mostly by default.

Recommended:
- selected filesystem read: policy-dependent;
- search/retrieval: policy-dependent;
- write/edit: deny by default;
- mutating shell/process execution: deny by default;
- external messages: deny;
- persistent memory write: deny;
- schedule creation: deny;
- privilege escalation: deny;
- recursive spawning: deny by default.

Reasoning intensity must not imply more authority.

## 12. Automatic deliberation policy

Do not spawn a child for every turn.

Conceptual runtime policy:

```text
direct
isolated
auto
```

### direct
Parent reasons/responds normally.

### isolated
Create a deliberation child.

### auto
Choose based on:
- model profile;
- requested reasoning effort;
- context pressure;
- expected task horizon;
- tool-heavy workflow;
- verification/research need;
- user preference.

Example future model profile:

```yaml
reasoning_behavior: heavy

deliberation:
  preferred: isolated

parent:
  reasoning_effort: low

child:
  reasoning_effort: high
```

Exact thresholds require benchmarks.

## 13. NInfer preserve_thinking: exact semantics

Current upstream NInfer exposes:
- `enable_thinking`;
- `reasoning_effort`;
- `preserve_thinking`;
- separate `reasoning_content` and `content`;
- optional `--default-thinking-budget`.

`preserve_thinking` controls whether reasoning from **closed assistant turns** is rendered back into later prompts according to the selected chat template.

It does NOT:
- disable current-turn reasoning;
- remove current reasoning from the model's live sequence while generating;
- guarantee one universal default across all artifacts/templates.

Important:
- if request/server does not explicitly resolve preservation, template behavior matters;
- current Qwen3.6 examples/templates can omit closed-turn reasoning by default;
- current Qwen3.8 templates/model cards may retain it by default.

Therefore SuprAI should never depend on a universal NInfer default.

When supported and desired, send explicit:

```json
"preserve_thinking": false
```

## 14. NInfer active multi-tool nuance

Current NInfer Qwen templates can omit older closed-turn reasoning while preserving reasoning belonging to the active multi-step tool chain after the latest real user query.

Conceptually:

```text
User A
  Reasoning RA
  Answer A

User B
  Reasoning RB
  Tool Call
  Tool Result
  continuation
```

With closed-turn omission:
- RA can disappear from User B's prompt;
- RB can remain useful during User B's active tool chain;
- once User B becomes historical, RB can be omitted from later User C prompts.

This is a useful provider-native optimization, but SuprAI still owns canonical context construction.

## 15. Generic OpenAI-compatible provider requirements

SuprAI remains local/OpenAI-compatible first.

The architecture cannot require NInfer-only fields.

Provider capability model should distinguish:

```text
reasoning_output_separated
reasoning_history_control
reasoning_effort_control
thinking_budget_control
provider_native_reasoning_state
```

If supported:
- use provider-native reasoning controls.

If not:
- SuprAI reconstructs future prompts from its own canonical items;
- raw child reasoning is not included in parent requests.

Correctness stays provider-independent.

## 16. What this architecture solves

Strongly:
- raw reasoning does not pollute visible parent chat;
- raw reasoning does not need to persist in parent canonical context;
- reasoning-heavy local models can work in isolated child contexts;
- future parent turns inherit only compact derived state;
- multiple specialists can reason independently without merging raw traces;
- existing Task/subagent lifecycle is reused.

## 17. What it does not solve

Not by itself:
- child reasoning still consumes child context/KV;
- full child snapshots can still start too large;
- summarization/capsules can lose information;
- extra child/parent inference can add latency;
- arbitrary models may not produce ideal capsules without harness guidance;
- local hardware may force serial execution.

These are measurement/implementation problems, not reasons to duplicate the runtime architecture.

## 18. Recommended prototype matrix

Compare:

1. direct reasoning with raw historical reasoning retained;
2. direct reasoning with closed-turn reasoning omitted;
3. deliberation child + full context;
4. deliberation child + scoped context;
5. deliberation child + compacted context;
6. multiple deliberation/verification children + parent synthesis.

Test:
- NInfer;
- llama.cpp;
- vLLM;
- reasoning-heavy local models;
- ordinary chat;
- coding/tool loops;
- research;
- near-capacity context.

Measure:
- peak child context;
- parent prompt growth;
- task success/quality;
- repeated reasoning;
- latency;
- GPU/KV pressure;
- tool correctness;
- capsule information loss;
- extra inference cost.

## 19. Current recommendation

Use the existing subagent architecture.

Do not build a standalone DeliberationBranch system.

Canonical terminology:

```text
Task(source=subagent, purpose=deliberation)
Child Session
ReasoningWorkspace
ReturnCapsule
```

The accurate product promise is:

> SuprAI can perform reasoning-heavy work in an isolated child agent context and return only compact, provenance-aware derived state to the parent conversation.

The child's raw reasoning still uses the child's finite context window; isolation prevents that scratch trace from becoming permanent parent-context debt.
