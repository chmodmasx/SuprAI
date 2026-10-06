# ADR-0011: SuprAI owns canonical conversation state

Status: accepted
Date: 2026-10-06

## Decision

SuprAI's SQLite state is the canonical durable source of conversation history.

Provider-side response/conversation storage is an optional optimization only.

NativeSuprAIRuntime must be able to reconstruct every model request from SuprAI-owned persisted state without requiring:
- `previous_response_id`;
- provider Conversations/Threads;
- provider-side durable response storage;
- a surviving inference-server process.

## Why

Current local Responses implementations differ materially:
- llama.cpp rejects `previous_response_id`;
- NInfer supports local stored Responses, but they are process-local/bounded and do not survive restart;
- vLLM requires its response store to be enabled and may disable `store` when it is not.

Relying on provider state would make session durability provider-specific and would break local-first recovery.

## Canonical model

Persist generalized conversation items rather than flattening everything to user/assistant text pairs.

Baseline item kinds:
- message;
- reasoning metadata/summary when legitimately exposed;
- tool_call;
- tool_result;
- attachment reference;
- runtime annotation.

Items carry stable SuprAI IDs. Provider IDs are optional metadata.

A session contains ordered turns; turns contain ordered items.

## Provider optimization

A provider may expose:
- previous-response continuation;
- prompt-cache keys;
- prefix/KV reuse;
- server-side token counting.

SuprAI may use these only when they do not become required for correctness.

If an optimization disappears after server restart, the next request must still be reconstructible from local canonical state.

## Responses vs Chat

Responses remains preferred when the endpoint implementation supports the required feature set.

Endpoint existence alone is not sufficient proof of semantic compatibility.

Per-provider configuration supports:
- auto;
- responses;
- chat_completions.

Auto may record learned capability results, but manual override always wins.

## Consequences

- canonical session export/backup is provider-independent;
- changing providers does not destroy session meaning;
- provider storage can accelerate execution but not define history;
- context compaction remains a SuprAI concern.
