# ADR-0014: Safe native transcript rendering

Status: partially implemented — long-transcript and physical desktop acceptance pending
Date: 2026-10-06
Updated: 2026-10-12

## Problem and boundaries

A streaming AI transcript must support rich text, fenced code, selection, links
and changing heights without triggering network requests or tying token delivery
to the speed of QML painting. Model output is untrusted.

The canonical conversation belongs to RuntimeOrchestrator and persistence.
`suprai_ui` owns only its projected transcript. Presentation never changes
canonical text, execution state or provider behavior.

## Current implementation

- `TranscriptModel` is a C++ `QAbstractListModel` whose roles expose canonical
  plain text, terminal state and disposable Markdown presentation blocks.
- `MarkdownBlocks` splits prose from fenced code; code cards are non-executing,
  selectable, horizontally scrollable and expose explicit copy actions.
- `SafeMarkdownRenderer` uses Qt's GitHub Markdown parser with
  `MarkdownNoHTML`; all text is escaped before emitting a narrow set of
  generated RichText markup. Model HTML and images never reach the QML parser.
- Absolute HTTP(S) anchors with a host and without embedded credentials are
  allowed only from parsed Markdown. A deliberate user click is revalidated
  by `ChatController` before opening the system browser; no confirmation
  modal and no ambient navigation occur.
- `ProseText.qml` uses `TextEdit.RichText` **only** for generated allowlisted
  presentation, otherwise `TextEdit.PlainText`. The cursor belongs to the
  final streaming segment; it is not repeated after every prose block.
- Rendering is synchronous and bounded: at most 65,536 UTF-16 units and
  80 blocks for final completed assistant messages. Oversized messages,
  user text and stopped/failed messages stay plain.
- During an active assistant stream, raw deltas are appended immediately,
  but `dataChanged` and Markdown preview rebuilds are **coalesced every
  90 ms** for the affected rows, not emitted or reparsed per token.
  Live rich previews are bounded to 32,768 UTF-16 units; larger in-progress
  messages fall back to raw text, while final completion still uses the
  full 65,536-unit limit. No message is truncated.
- The streaming parser can show a still-open fenced code block as a code
  card. Final completion uses the strict matched-fence parser. Terminal
  completion refreshes the projection immediately; failure or cancellation
  discards any preview and returns to raw text. Pending timer work cannot
  resurrect terminated items.
- The implementation introduces no WebEngine, new runtime dependency,
  network loader or agent-layer change.

## Validation requirements

Automated tests cover:
- Markdown safety, untrusted HTML/images/URLs, fenced code and size bounds;
- QML instantiation and selectable `QTextDocument` rich-text interpretation;
- deferred/coalesced UI updates, unfinished code fences and terminal cleanup;
- packaged XCB/Xvfb and Wayland/Weston smoke tests.

These tests do **not** prove real KDE/GNOME link/copy accessibility or long-run
smoothness. Physical acceptance of the alpha.7 link/copy improvements and the
new live preview remain outstanding until tested on a real desktop.

## Remaining acceptance work

Benchmark both coalesced full-message parsing (current preview) and a
completed-block + streaming-tail incremental parser, including 1,000+ items,
long Markdown, long fences, scrolling and pane resize under real inference.
Choose measured complexity rather than assuming the 90 ms budget ensures
smoothness. Evaluate tables, quotes, attachments, keyboard accessibility,
and syntax highlighting as independent UI modules, without adding WebEngine
to the ordinary chat surface.
