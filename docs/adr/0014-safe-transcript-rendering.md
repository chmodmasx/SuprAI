# ADR-0014: Safe native transcript rendering

Status: proposed
Date: 2026-10-06

## Problem

A long streaming AI transcript stresses:
- layout;
- Markdown parsing;
- variable-height virtualization;
- selection/copy;
- code blocks;
- untrusted links/images;
- token-by-token updates.

Qt Quick can render Markdown directly, but rich/Markdown text may load remote images. Directly feeding untrusted model Markdown into a generic rich-text surface therefore violates SuprAI's no-ambient-network rule.

## Proposed architecture

### Transcript model

C++ `QAbstractListModel` owns transcript presentation state derived from domain items.

QML `ListView`:
- `reuseItems: true`;
- no durable state in delegates;
- tuned `cacheBuffer`;
- lightweight delegates;
- variable-height behavior benchmarked with realistic long conversations.

### Streaming

Provider deltas update the runtime model immediately but UI notifications are coalesced to a short frame-oriented interval when semantic ordering permits.

Do not trigger a full transcript rebuild per token.

Benchmark at minimum:
- 1k messages;
- very long Markdown answer;
- long code block;
- active streaming while scrolling/resizing.

### Markdown

Use a safe Markdown presentation layer.

Required properties:
- raw HTML disabled/sanitized;
- remote image loading disabled by default;
- inline file/image references resolve only through explicit SuprAI attachment resources;
- link activation is intercepted and routed to explicit OpenURI behavior;
- no script/web execution;
- code blocks are explicit components where practical.

Qt's Markdown/CommonMark/GitHub-dialect parser can be reused, but the final renderer must enforce these resource policies.

Do not use Qt WebEngine to render ordinary chat.

### Code blocks

Own component:
- language label;
- copy action;
- horizontal scrolling;
- monospace font;
- no execution on click;
- optional syntax highlighter added only after dependency/performance review.

### Streaming Markdown

Do not assume reparsing an ever-growing full Markdown string per token will scale.

M1/M2 should benchmark:
A. coalesced full-message Markdown reparse;
B. completed-block + streaming-tail rendering.

Choose the simpler implementation if it stays within performance targets.

## Acceptance criteria

- no network request caused merely by rendering model Markdown;
- no WebEngine dependency;
- stable selection/copy;
- smooth long transcript scroll;
- streaming does not produce O(n)-sized UI object churn per token;
- external links require a user action.

## Implemented M2 slice (2026-10-09)

A private SafeMarkdownRenderer owned by suprai_ui now:

- Parses completed assistant messages using QTextDocument MarkdownNoHTML.
- Iterates only text fragments, escaping all model-supplied text.
- Projects to a minimal explicit allowlist of Qt StyledText tags: b, i,
  font and br. Images become a placeholder; anchors and resource URLs are
  not emitted. No renderer-initiated network access is expected.
- Keeps streaming output, user text, failed/cancelled items and output over
  65,536 UTF-16 code units as plain text.
- Caches rich text once per completed message; streaming deltas do not parse
  Markdown. The canonical conversation stays unmodified.
- Adds security and large-message boundary tests without any new third-party
  runtime or WebEngine dependency.

ADR remains proposed. This is only the initial safe subset. Selection/copy,
large code blocks, external links with policy-backed OpenURI, attachment
resources and full long-transcript performance proof remain unverified.
