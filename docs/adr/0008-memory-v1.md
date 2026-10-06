# ADR-0008: Memory v1 separates history, search and curated memory

Status: accepted
Date: 2026-10-06

## Decision

SuprAI memory v1 has three mandatory layers and one deferred layer.

### 1. Canonical history
SQLite stores durable session/message/item history.

Summaries never replace canonical history.

### 2. Searchable history
SQLite FTS5 indexes textual history from the start.

### 3. Curated active memory
A bounded set of records is eligible for prompt injection.

Scopes:
- user;
- agent/profile;
- project.

Each record carries provenance/trust metadata outside recalled prose.

Suggested metadata:
- id;
- scope;
- source session/message;
- provenance/trust class;
- created/updated timestamps;
- supersession relation;
- status;
- optional importance.

Suggested provenance classes:
- explicit_user;
- agent_inferred;
- imported;
- untrusted_external;
- system.

### 4. Semantic/vector retrieval
Deferred until usage demonstrates that FTS5 is insufficient.

No embedding model or vector database is a mandatory runtime dependency for v1.

## Rules

- conversation persistence is not memory;
- memory writes are explicit observable mutations;
- agent-generated text cannot self-assign higher trust through prose;
- superseded facts should be replaced/superseded rather than accumulated as contradictory active truths;
- in-flight turn context must not silently mutate because memory changed concurrently;
- retrieval results preserve provenance.

## Why

Hermes demonstrates the value of bounded curated prompt memory plus SQLite FTS5 session search. OpenClaw demonstrates useful provenance/trust separation. Both support keeping durable history distinct from active prompt memory.

## Consequences

MemoryService can later add hybrid/vector retrieval without changing the canonical session store or trust model.
