# Reasoning Context Isolation / Context Folding Review — 2026-10-06

Status: active research
Purpose: evaluate whether model reasoning can live in an ephemeral branch/context and only a compact result returns to the canonical conversation.

## Executive conclusion

The proposed idea already exists in closely matching form under the name **Context Folding / Sub-Trajectory Folding**.

The core pattern is:

```text
main context
  -> branch temporary sub-context
       -> reason / call tools / explore
       -> return compact result
  -> fold branch away
  -> continue main context with compact result only
```

This is materially different from merely hiding a `reasoning_content` field.

It can keep the canonical/main conversation small, but it does NOT make the reasoning tokens disappear from the temporary branch's own active context during generation.

Therefore:
- it solves parent-context pollution;
- it solves future-turn reasoning accumulation;
- it can strongly reduce long-horizon active context;
- an exact full-context clone does not solve single-branch peak context pressure;
- to reduce branch peak too, the branch must use a scoped/compacted snapshot and/or fold internally in stages.

## 1. Closest prior art: Context Folding

Paper:
- Scaling Long-Horizon Agent via Context Folding, ICML 2026.
- Code: sunnweiwei/FoldAgent / MiaoLu3/Context_Folding.

The mechanism has two explicit actions:
- `branch(description, prompt)`: create a temporary sub-trajectory;
- `return(message)`: collapse the branch and rejoin the main trajectory with only the return message.

The released FoldAgent implementation literally creates a branch Agent from a copy of the main agent's current messages:

```python
history = agent['main'].messages()
agent[agent_name] = Agent(llm_client, history, ...)
```

The branch then runs independently. On return, only a concise branch result is appended to the main agent as an observation. Intermediate branch messages remain outside the main trajectory.

This is extremely close to the proposed "temporary copy of the current chat, reason there, summarize, merge back, delete temporary chat" idea.

Important caveat:
The published strongest results use a model trained to learn when/how to branch and return (FoldGRPO). The mechanism itself is implementable at harness level, but arbitrary local models should not be assumed to manage it optimally without orchestration.

## 2. Training-free evidence

MM-ContextFold (2026) uses a training-free variant for multimodal agentic retrieval:
- persistent compact text-only main context;
- ephemeral branch contexts for expensive image-dependent subtasks;
- branch trace and raw media discarded after a concise result returns.

This strengthens the case that context folding can be a harness/runtime pattern, not only a trained model behavior.

Other systems/patterns also use isolated subagent contexts returning summaries, but Context Folding is the closest exact formulation.

## 3. Critical distinction: main-context footprint vs branch peak

Let:
- `P` = parent/main context before current task;
- `R` = raw reasoning/branch trace;
- `S` = compact return summary/checkpoint.

Without isolation:

```text
future main context ~= P + R + answer
```

With a temporary full clone:

```text
branch peak ~= P + R
future main context ~= P + S + answer
```

Therefore a full clone can save approximately `R-S` tokens from all future main-context requests.

But if `P + R` exceeds the model context limit, the branch still fails.

Example:

```text
context limit = 262k
P = 180k
R = 70k
S = 1k

branch peak = 250k   -> fits
main after fold ~= 181k + answer

but if R = 100k:
branch peak = 280k   -> still overflows
```

So "exact copy" is useful for isolation, not a complete solution to peak reasoning pressure.

## 4. Better SuprAI variant

Use an ephemeral **DeliberationBranch**, not a second canonical Session.

Concept:

```text
Canonical Session
  -> Turn
      -> DeliberationBranch
          context_snapshot
          raw reasoning
          tool trace
          hypotheses
          temporary provider state
          -> ReturnCapsule
      -> canonical result
```

The branch can have context modes:

```text
full_snapshot
scoped_snapshot
compacted_snapshot
```

Default policy should probably be adaptive:
- short parent context: full snapshot;
- large parent context: scoped/compacted snapshot;
- tool-heavy long run: nested/periodic fold checkpoints.

The branch should reference the parent's canonical item IDs rather than physically duplicate persisted history.

## 5. What returns from the branch

Do not automatically promote a prose "summary of chain-of-thought" as truth.

Prefer a structured ReturnCapsule / WorkingState:

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

Important:
- facts retain provenance;
- hypotheses remain hypotheses;
- branch output is agent-generated data, not user/system authority;
- evidence references point to canonical tool/file artifacts where possible.

Raw reasoning is ephemeral by default.

## 6. Two possible merge modes

### A. return-state then parent continues

```text
branch -> ReturnCapsule
main   -> next model call using capsule
main   -> final answer
```

Pros:
- parent regains high-level control;
- closest to Context Folding paper.

Cons:
- extra model call;
- parent context still must fit capsule + current history.

### B. branch returns state + final answer

```text
branch -> ReturnCapsule + final_answer
main   -> persist/display answer directly
```

Pros:
- no extra parent inference;
- simple for ordinary chat turns.

Cons:
- parent does not get a final verification pass;
- must separate answer from branch scratch state cleanly.

SuprAI may support both depending on task type.

## 7. Relation to ReasoningWorkspace

`ReasoningWorkspace` and `DeliberationBranch` are related but not identical.

### ReasoningWorkspace
Ephemeral storage/representation of reasoning generated inside one Run/provider sequence.

It does not by itself create a separate model context.

### DeliberationBranch
A separate temporary inference context derived from a parent snapshot.

It can own a ReasoningWorkspace.

Thus:

```text
DeliberationBranch
  └─ ReasoningWorkspace
```

The branch is the context-isolation mechanism. The workspace is the ephemeral reasoning state inside it.

## 8. NInfer preserve_thinking: exact semantics

Current upstream NInfer exposes:
- `enable_thinking`;
- `reasoning_effort`;
- `preserve_thinking`;
- separate `reasoning_content` and `content` streams/fields;
- optional `--default-thinking-budget`.

`preserve_thinking` controls whether reasoning from **closed assistant turns** is rendered back into later prompts according to the selected chat template.

It does NOT:
- disable current-turn reasoning;
- remove current reasoning from the model's active sequence while it is generating;
- guarantee the same default across every artifact/template.

Important correction:
The server option is optional/unset unless explicitly configured; when the request/server does not resolve a value, the selected template's default applies.

Current official examples differ:
- Qwen3.6 template/model cards: closed-turn reasoning omitted by default;
- Qwen3.8 template/model cards: closed-turn reasoning retained by template default.

Therefore SuprAI must not assume a universal NInfer default.

When the extension is supported, SuprAI should send an explicit `preserve_thinking:false` when it wants deterministic closed-turn omission.

## 9. NInfer multi-step tool nuance

The current Qwen templates preserve reasoning generated after the latest real user query across assistant/tool-result steps in the same active multi-step tool chain even when old closed-turn reasoning is omitted.

Conceptually:

```text
User A
  Assistant reasoning RA + answer A

User B
  Assistant reasoning RB + tool call
  Tool result
  Assistant continuation
```

With closed-turn preservation disabled:
- RA can be omitted from the prompt for User B;
- RB can remain available within User B's ongoing tool chain;
- once User B's turn becomes historical, RB can be omitted from later User C prompts.

This behavior is very close to the desired "reasoning is temporary working state for the current turn, not permanent conversation history".

SuprAI should still own this policy in its ContextManager rather than relying exclusively on provider-specific template behavior.

## 10. Generic OpenAI-compatible harness implications

SuprAI targets local OpenAI-compatible endpoints first.

The canonical architecture must not depend on the NInfer-only `preserve_thinking` extension.

Provider capability model should distinguish:

```text
reasoning_output_separated
reasoning_history_control
reasoning_effort_control
thinking_budget_control
provider_native_reasoning_state
```

If a provider supports `preserve_thinking`, use it as an optimization/semantic aid.

If not, SuprAI still reconstructs future requests from its own canonical history and can omit raw reasoning items itself.

## 11. What this does and does not solve

### Solves well
- raw reasoning does not pollute visible chat;
- raw reasoning does not need to persist in canonical conversation;
- future turns can use a compact derived checkpoint instead;
- tool-heavy/subtask exploration can be folded away;
- main-context growth can be dramatically reduced on long-horizon work.

### Does not solve by itself
- reasoning tokens inside the currently active branch still consume that branch's context window/KV;
- a full exact clone starts with the same large parent context;
- summarization can lose subtle premises;
- arbitrary local models may not autonomously know when/how to branch;
- an extra summarization/merge model call can add compute/latency.

## 12. Recommended SuprAI research direction

Prototype at harness level, without model training:

1. accept canonical user Input;
2. build a parent ContextSnapshot;
3. create ephemeral DeliberationBranch;
4. run the same configured local model/provider;
5. keep raw reasoning/tool trace branch-local;
6. force/derive a structured ReturnCapsule;
7. validate/provenance-tag the capsule;
8. merge only capsule/final answer into canonical Turn;
9. destroy branch-local reasoning state;
10. compare against baseline preserve/omit reasoning.

Test three branch context modes:
- full_snapshot;
- scoped_snapshot;
- compacted_snapshot.

Measure:
- peak context tokens per inference;
- canonical context growth;
- answer/task quality;
- repeated-reasoning rate;
- latency;
- tool correctness;
- summary information loss.

## 13. Current recommendation

Do not call the feature "reasoning outside the context window".

More accurate names:
- Ephemeral Deliberation Branch;
- Context Folding;
- Reasoning Context Isolation.

The correct promise is:

> Raw deliberation may execute in an ephemeral branch and be folded into a compact, provenance-aware state before the canonical conversation continues.

This preserves conceptual accuracy: the temporary branch still has an active context window, but the parent/canonical context does not inherit its full reasoning trace.
