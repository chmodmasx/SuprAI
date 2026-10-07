# ADR-0003: Normalize inference; Responses preferred, Chat compatible

Status: accepted
Date: 2026-10-06  
Updated: 2026-10-07

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

## Capability semantics

Provider/model capabilities are tri-state:

```text
supported
unsupported
unknown
```

Missing or empty capability metadata is never treated as authoritative `unsupported`.

Capability provenance is retained where useful:
- explicit provider declaration;
- verified probe;
- cached verified observation;
- user/config override;
- unknown.

This is important for local/OpenAI-compatible endpoints whose catalogs may omit tool, vision, reasoning or transport metadata even when the runtime supports the feature.

Unknown capability may trigger a conservative probe, explicit configuration, or a compatibility path. It must not silently strip tools/images/features from a request merely because metadata is absent.

## Consequences

- AgentLoop consumes only normalized events.
- Transport tests must include malformed SSE, partial tool arguments, terminal events and cancellation.
- A provider attempt may be transparently retried only before any observable text, reasoning, media or tool call has been emitted; after observable generation, failure/incomplete state is explicit rather than silently replayed.
- Provider feature support is capability metadata, not inferred from product name.
- New provider transports can be added without changing AgentLoop semantics.
