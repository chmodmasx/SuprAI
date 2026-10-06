# ADR-0018: Provider-aware token accounting and context budgeting

Status: accepted
Date: 2026-10-06

## Decision

SuprAI owns a `TokenBudgetService` that budgets the final provider request, not raw message text.

It discovers the effective runtime context limit and uses the most exact counting capability available for the active provider/model.

## Effective context limit

Precedence:
1. explicit user override;
2. provider/runtime-advertised effective limit;
3. verified cached probe/result for the exact provider+model configuration;
4. conservative configured fallback;
5. unknown -> require explicit configuration before high-context automation.

Do not use training context as the effective runtime limit when the server advertises a smaller configured window.

Current examples:
- NInfer `/v1/models` exposes effective `max_model_len`;
- vLLM `/v1/models` exposes `max_model_len` on the direct server;
- llama.cpp exposes effective slot `n_ctx` in `/v1/models.meta.n_ctx` and `/props`.

## Token-count capability ladder

`TokenAccountingCapabilities` describes what the provider can do.

Preferred order:
1. exact final-request count endpoint;
2. provider render + tokenize path that includes template/tools/media semantics;
3. provider tokenizer count for a known rendered prompt;
4. SuprAI conservative estimator with explicit uncertainty margin.

Current examples:
- NInfer: `POST /v1/responses/input_tokens` follows the same render/media/tool path as create;
- llama.cpp: current server exposes `/v1/responses/input_tokens` and `/v1/chat/completions/input_tokens`;
- vLLM: exposes effective prompt usage after rendering/generation and generic tokenizer endpoints, but SuprAI must not pretend generic `/tokenize` is equivalent to final agent-request counting when tools/templates/media are involved.

## Budget

Before a model request:

```text
effective_context
  - output_reserve
  - safety_margin
  = maximum_input_budget
```

Then:
1. construct the actual candidate InferenceRequest including instructions, memory, skills, tool schemas, history and attachments;
2. count that candidate through the best provider capability;
3. if it fits, send exactly that semantic request;
4. if it does not fit, invoke ContextCompactor and rebuild/recount;
5. never rely on provider truncation as normal context management.

`output_reserve` is based on requested/allowed output for the active turn, not a single global constant.

The safety margin is larger for estimated counts than for exact counts.

## Tool schemas and media

Tool definitions are part of input context and must be counted.

Multimodal requests use provider-native exact accounting when available. Text-token estimates are not assumed to model image/audio token cost.

## Provider overflow feedback

If the provider rejects a request as over-context:
- record the provider-confirmed effective constraint/evidence;
- do not blindly retry unchanged;
- compact/rebudget once through the normal path;
- protect against retry loops.

## UI

Expose:
- effective context window;
- measured/estimated input tokens;
- reserved output;
- whether count is exact or estimated;
- compaction state.

Do not display a fabricated precise token number when only an estimate is available.

## Why

Agent context includes system instructions, skills, memory, tool schemas, chat-template overhead, tool results and media. Counting only visible messages systematically underestimates real pressure.

Provider-native final-request counting avoids reproducing each model's template logic inside SuprAI.

## Consequences

Token accounting is a provider capability, not a tokenizer utility hidden inside ContextManager.
