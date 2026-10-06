# ADR-0020: Prompt/KV caching is an optional provider optimization

Status: accepted
Date: 2026-10-06

## Decision

Prompt/KV caching may improve latency but never participates in correctness, canonical history or session identity.

SuprAI sends semantically complete requests even when it expects the provider to reuse a prefix.

## Stable-prefix strategy

ContextBuilder orders stable material before volatile material when semantics permit:

1. stable system/runtime instructions;
2. stable project instructions;
3. active skill instructions;
4. curated memory snapshot;
5. tool schemas/catalog;
6. compaction/history;
7. newest conversation tail and current user input.

Exact ordering remains model/template-aware. Do not reorder messages merely for caching if that changes model semantics.

Changes to instructions, memory, skills, tool schemas or model configuration naturally invalidate some prefix reuse.

## Provider-specific hints

Examples:
- llama.cpp reuses common prompt prefixes and has server prompt-cache controls;
- vLLM Automatic Prefix Caching hashes token blocks and supports `cache_salt`;
- NInfer supports OpenAI prompt-cache options/breakpoints.

These are adapter capabilities.

Do not invent a universal `prompt_cache_key` meaning across providers.

## Privacy / multi-tenant servers

Prefix caching can create timing side channels in shared inference servers.

When a provider supports cache isolation (for example vLLM `cache_salt`):
- SuprAI may generate a cryptographically random secret salt scoped to the desired trust boundary;
- never derive it from a public username/project name;
- store it as secret material when persistence is required;
- bound its length.

For a truly single-user local server, cache salting may be omitted to maximize reuse.

## Metrics

When providers expose cached-token counts, store them as performance telemetry separate from semantic usage.

Cache hit/miss must never change the persisted conversation.

## Failure

Cache eviction, server restart, slot migration or disabled caching must only affect performance.

A request must remain correct after every cache is lost.

## Why

llama.cpp, vLLM and NInfer all implement prefix/prompt caching differently. Conflating those mechanisms with session continuation would bind SuprAI to ephemeral provider state.

## Consequences

Context layout can be cache-friendly while canonical durability remains entirely SuprAI-owned.
