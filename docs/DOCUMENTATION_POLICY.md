# Documentation Policy

Status: canonical
Purpose: define how SuprAI documentation is maintained.

## 1. Core rule

The repository documentation describes the **best current understanding of SuprAI**.

It is not an archive of every idea previously considered.

When new research, implementation evidence, upstream changes, tests, or architectural work show that an existing statement is wrong, incomplete, obsolete, or inferior:

1. replace it with the better current version;
2. update every canonical document that depends on it;
3. remove obsolete duplicated material;
4. remove or rewrite superseded ADRs/research notes when they would mislead;
5. update `docs/PROJECT_STATE.md` in the same change.

Git history is the archive. Current files are the truth.

## 2. No stale-doc preservation

Do NOT keep obsolete text merely to preserve history.

Forbidden patterns:
- keeping contradictory "old approach" and "new approach" sections in canonical docs;
- leaving an ADR accepted when the project no longer follows it;
- adding "v2" documents while a stale "v1" remains authoritative-looking;
- keeping dead research notes that no longer contain unique useful evidence;
- duplicating the same decision in multiple documents with divergent wording;
- retaining a wrong statement with a note saying "this is outdated" when it can simply be corrected or removed.

If historical context is useful for understanding a migration, keep only the minimum required context and make the current rule unambiguous.

## 3. Source-of-truth order

For project intent and architecture:

1. current code and tests;
2. accepted ADRs;
3. `docs/ARCHITECTURE.md`;
4. `docs/ROADMAP.md`;
5. `docs/PROJECT_STATE.md`;
6. research/reference notes.

If code and canonical documentation disagree:
- determine whether the code is intentionally ahead, accidentally divergent, or wrong;
- resolve the disagreement;
- update documentation in the same change.

Never silently normalize documentation to buggy code.

## 4. ADR maintenance

ADRs are current architectural decisions, not immutable museum records.

### Same topic, improved decision
Edit the existing ADR in place:
- update decision;
- update rationale;
- update consequences;
- update date/status if useful;
- update dependent docs.

### Decision fundamentally replaced
If the old ADR would mislead:
- delete it;
- create the replacement ADR if one is still needed;
- update all references/indexes/state.

Do not keep a superseded ADR solely for history. Git already preserves it.

### Proposed ADR rejected
Delete it unless the rejection contains unique information that materially protects future work from repeating a known mistake. If retained, convert the useful evidence into current research/reference documentation and remove the dead proposal.

## 5. Research maintenance

Research documents exist to support current engineering decisions.

When new research changes a conclusion:
- update the research note if it remains useful;
- merge useful parts into a better research note when appropriate;
- delete redundant or obsolete notes;
- update ADRs/architecture affected by the conclusion.

A dated filename does not grant permission to leave known-wrong technical claims in the active repository.

External facts that are time-sensitive should include enough context/date to know when they were verified.

## 6. Canonical-document responsibilities

### `AGENTS.md`
Engineering invariants and instructions for future AI/developers.

Must contain only rules that are currently intended to be followed.

### `docs/ARCHITECTURE.md`
Current system architecture.

No obsolete alternative architectures unless they are still active options requiring a decision.

### `docs/ROADMAP.md`
Current implementation sequence and proof gates.

Remove completed/dead paths that would misdirect future work; retain completed milestones only when their verified outputs remain useful navigation.

### `docs/PROJECT_STATE.md`
Current resumable handoff.

Must be updated after every meaningful research, architecture, implementation, or verification change.

### `docs/REFERENCES.md`
Current external facts, upstream lessons, and evidence relevant to decisions.

Remove stale references that no longer support current architecture.

### `docs/research/`
Focused supporting research.

Merge/delete when a newer document fully replaces an older one.

### `docs/adr/`
Current significant architecture decisions and proof-gated proposals.

No knowingly obsolete accepted decisions.

## 7. Change checklist

Before finishing meaningful work:

```text
[ ] Did this change invalidate an existing statement?
[ ] Did it change an architectural decision?
[ ] Did it make an ADR obsolete?
[ ] Did it make a research note redundant or wrong?
[ ] Did it change roadmap ordering or proof gates?
[ ] Did it change PROJECT_STATE?
[ ] Are there now two documents claiming different truths?
```

If any answer is yes, clean the documentation before handoff.

## 8. Contradiction scan

Future AI/developers should periodically search the repository for:
- renamed concepts;
- old backend/runtime names;
- deprecated APIs/protocol versions;
- superseded dependency choices;
- obsolete milestone numbers;
- old state names;
- old schema names.

When a decision changes, perform this scan immediately rather than waiting for a later cleanup milestone.

## 9. Documentation quality target

A new AI should be able to clone the current repository and derive the intended system without needing:
- prior chats;
- deleted assumptions;
- archaeological interpretation of stale ADRs;
- guessing which of two conflicting documents is newer.

Current repository state should be enough.
