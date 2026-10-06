# ADR-0007: Skills use the Agent Skills standard

Status: accepted
Date: 2026-10-06

## Decision

SuprAI implements Agent Skills-compatible skill directories rather than defining a proprietary primary format.

Canonical form:

```text
skill-name/
  SKILL.md
  scripts/
  references/
  assets/
```

`SKILL.md` uses the Agent Skills YAML frontmatter and Markdown body.

## Loading

Use progressive disclosure:
1. index name + description for discovery;
2. load the full SKILL.md when selected;
3. load scripts/references/assets only when needed.

Suggested paths:
- project: `.agents/skills/`;
- user: `$XDG_DATA_HOME/suprai/skills/`.

## Security

Skill metadata is not authority.

In particular, the experimental `allowed-tools` field may inform UX/policy suggestions but must never bypass SuprAI's PolicyEngine.

Scripts supplied by a skill use the same tool/process policy and containment pipeline as any other executable action.

## Extensions

SuprAI-specific fields should use namespaced `metadata` keys where possible rather than forking the format.

## Why

An interoperable skill format already exists and supports instructions, executable helpers, references, assets and progressive context loading.
