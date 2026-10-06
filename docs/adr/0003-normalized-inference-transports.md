# ADR-0003: Normalize inference; Responses preferred, Chat compatible

Status: accepted
Date: 2026-10-06

## Decision

SuprAI owns provider-neutral `InferenceRequest`, `InferenceEvent`, tool-call, usage and error types.

Initial OpenAI-compatible transports:
- `OpenAIResponsesTransport` — preferred;
- `OpenAIChatCompletionsTransport` — compatibility.

Provider wire objects never escape `providers/`.

## Why

Current llama.cpp, vLLM and NInfer all expose Responses-compatible endpoints as well as Chat-style APIs, but compatibility is not identical. Designing the runtime around either wire format would make provider quirks part of SuprAI's agent semantics.

Responses offers a better structural fit for typed streamed items, reasoning and function calls. Chat remains important for existing/local servers.

## Auto-selection

`auto` may attempt Responses first.

Fallback to Chat is allowed only when endpoint non-support is established before observable generation. An arbitrary HTTP 400 is not sufficient because it may indicate invalid configuration or unsupported fields rather than a missing Responses route.

Resolved capability may be cached per provider configuration and can always be manually overridden.

## Consequences

- AgentLoop consumes only normalized events.
- Transport tests must include malformed SSE, partial tool arguments, terminal events and cancellation.
- Provider feature support is capability metadata, not inferred from product name.
- New provider transports can be added without changing AgentLoop semantics.
