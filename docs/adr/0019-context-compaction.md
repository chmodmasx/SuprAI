# ADR-0019: Context compaction is a derived, auditable artifact

Status: accepted
Date: 2026-10-06  
Updated: 2026-10-07

## Decision

Compaction never rewrites or deletes canonical conversation history.

It creates a derived `CompactionArtifact` used by ContextManager when assembling future model requests.

## Artifact

Baseline metadata:

```text
id
session_id
source_first_item_id
source_last_item_id
source_item_ids/hash
first_retained_item_id
summary
created_at
summarizer_provider/model
summarizer_config_version
prompt_template_version
tokens_before
tokens_after
count_method
quality_status
supersedes_compaction_id?
```

The artifact is reproducible/auditable against canonical source items.

## Context assembly

A compacted request contains:
- stable system/runtime instructions;
- currently active skill/memory/tool context;
- selected compaction artifact(s);
- recent canonical tail items verbatim.

Canonical history remains searchable/exportable independently.

## Boundary rules

Never split structural pairs:
- tool_call + terminal tool_result;
- attachment metadata needed by a retained message;
- user clarification + the action it resolves when the pair is semantically required.

Pending/active tool work is never compacted away.

Recent user requests and unresolved asks remain verbatim where possible.

## Compaction sequence

When pressure is detected:
1. reduce prompt-only copies of old oversized tool output where a deterministic structured summary can preserve outcome metadata;
2. choose a safe historical boundary;
3. generate a structured summary of older canonical items;
4. validate the artifact;
5. rebuild the exact provider request;
6. recount;
7. commit the artifact only if it produces a valid useful reduction.

The prompt-only reduction of tool output never alters canonical ToolResult data.

## Quality checks

At minimum:
- non-empty summary;
- source range still exists and matches recorded hash/IDs;
- required unresolved asks/identifiers are represented by structured guard fields or retained verbatim;
- tool structural pairing preserved;
- result is smaller by a meaningful amount;
- no generated artifact is accepted if the summarizer failed/truncated unexpectedly.

If compaction would increase context size, reject it.

A failed compaction leaves canonical history and previous valid context plan untouched.

## Trigger policy

Do not hard-code a universal "compact at 50%" rule.

Primary trigger is provider-aware TokenBudgetService:
- candidate input does not fit after output reserve/safety margin.

Optional proactive trigger may run before the hard boundary to avoid repeated emergency compactions, but it is computed from the active budget and remains configurable.

## Summarizer

The summarizer may be:
- active model;
- configured auxiliary model;
- later a deterministic/local summarizer for selected data classes.

Its identity/version is stored in the artifact.

## Manual UX

Later UI may offer:
- compact now;
- preview compaction;
- inspect source range;
- inspect summary;
- restore/rebuild context from canonical history.

"Restore" means change the context plan; canonical history was never destroyed.

## Why

Lossy summaries inevitably omit detail. Treating them as canonical history causes irreversible drift and makes debugging impossible.

Mature agents preserve recent history, keep tool pairs together, and record compaction separately. SuprAI makes that separation explicit from the schema level.

## Consequences

Long sessions can remain efficient without sacrificing auditability or provider independence.


## Emergency overflow recovery

If a provider rejects the candidate request as over-context after normal budgeting:
- mark the previous estimate as disproven evidence;
- force one deterministic recovery pass;
- prefer reductions that do not require another successful model call first;
- rebuild and recount the candidate request;
- bound recovery attempts.

Emergency recovery may:
- replace old model-facing tool-result bodies with deterministic bounded projections;
- retain only already-valid CompactionArtifacts plus recent canonical tail;
- drop optional derived context before canonical/recent user intent;
- require user action if the non-history baseline itself does not fit.

Emergency recovery never deletes or rewrites canonical history.

## Oversized ToolResult artifacts

Large tool results must not automatically become large model-context debt.

A full result can be retained as canonical data or a session/project-scoped artifact while ContextManager projects a bounded representation into inference:

```text
ToolResult
   |
   +--> full result / LargeResultArtifact
   |
   +--> model projection
           preview
           structural metadata
           artifact reference
           recovery/read hint
```

A baseline LargeResultArtifact records:
- artifact_id;
- session_id;
- tool_invocation_id;
- media/content type;
- size metadata;
- provenance;
- retention policy;
- integrity/hash metadata where useful.

The model-facing projection may preserve:
- head/tail excerpts;
- structured summary;
- error/final-status fields;
- native media references;
- an opaque artifact ID for later bounded reads.

Rules:
- full result remains auditable independently of prompt projection;
- artifact authority is scoped to the owning session/project/tool policy;
- shell/filesystem tools do not implicitly resolve internal artifact IDs;
- retrieval uses a dedicated bounded artifact-read capability;
- provider projection is allowed to be much smaller than canonical storage;
- UI and model-facing truncation limits are separate;
- truncation/reduction is explicit, never silent.

This mechanism is preferred before expensive summarization for old oversized tool output.
