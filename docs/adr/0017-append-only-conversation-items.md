# ADR-0017: Conversation history is append-oriented generalized items

Status: accepted
Date: 2026-10-06  
Updated: 2026-10-07

## Decision

The canonical conversation store is not a mutable list of `{role,text}` messages.

It is an ordered stream of generalized SuprAI items grouped by turn.

Baseline item kinds:
- message;
- reasoning_summary / reasoning_metadata when legitimately available;
- tool_call;
- tool_result;
- attachment;
- runtime_annotation.

Message content contains typed parts:
- text;
- image reference;
- file/reference metadata;
- future media parts.

Provider-specific item types are mapped at the provider boundary.

## IDs

Stable SuprAI IDs exist independently of provider IDs:
- session_id;
- turn_id;
- item_id;
- tool_invocation_id;
- attachment_id.

Provider IDs/call IDs are metadata used for wire mapping.

## Append-oriented rule

Completed canonical items are not rewritten merely to implement:
- retry;
- regenerate;
- edit-and-resend;
- branch;
- provider switch.

Those actions create new lineage.

Sessions/turns therefore retain enough lineage metadata to express a branch even if the first UI only shows a linear path.

## Tool structural integrity

Every accepted tool call receives a terminal tool outcome item.

Possible outcomes include:
- success;
- error;
- denied;
- cancelled;
- skipped_by_steering;
- outcome_unknown.

This keeps replay/model mapping structurally valid and audit-friendly.

## Partial streaming

Partial provider deltas are runtime state, not canonical completed items.

SuprAI may checkpoint partial assistant text for crash recovery, but it must be distinguishable from a completed canonical item.

A failed/incomplete attempt remains inspectable rather than being silently merged with a retry.

## Steering/user input while working

The runtime may later support steering/interrupt-and-redirect.

Incoming user intent while a turn runs is first a pending runtime event/queue entry. It becomes canonical conversation input at a defined state-machine boundary.

Never mutate an in-flight provider request's already-sent context.

## Why

Modern Responses-style APIs already generalize history beyond role/text messages, and mature agents need tool results, reasoning metadata, attachments, retries and branches.

OpenClaw additionally demonstrates the value of append-only structurally paired tool-call/result history and explicit steering boundaries.

## Consequences

- Chat Completions becomes merely one serialization target;
- Responses maps naturally;
- retries/branches preserve audit history;
- future subagents and background work can attach lineage without changing the base schema.

## Implemented foundation

The current domain layer now implements the generalized item shape directly rather than using a `{role,text}` placeholder.

Implemented typed content alternatives:
- MessageContent;
- ReasoningSummaryContent;
- ToolCallContent;
- ToolResultContent;
- AttachmentContent;
- RuntimeAnnotationContent.

ConversationItem stores:
- item ID;
- owning Turn ID;
- item lifecycle state;
- one typed content variant.

The current TranscriptModel intentionally projects only MessageContent. Non-message canonical/domain items can therefore exist without forcing QML to understand tool/attachment/runtime semantics prematurely.

RuntimeOrchestrator now stores generalized ConversationItem values as its in-memory prototype history and converts only MessageContent into the current Chat Completions provider request at the provider boundary.

Raw provider reasoning is still not inserted as a ReasoningSummary item automatically. A reasoning summary is canonical only when SuprAI explicitly creates one under the reasoning/deliberation policy.
