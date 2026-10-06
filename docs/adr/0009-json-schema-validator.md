# ADR-0009: jsoncons as JSON Schema 2020-12 validator candidate

Status: proposed
Date: 2026-10-06

## Problem

SuprAI's canonical tool schema is JSON Schema 2020-12. Many established C++ validators still implement only draft-7.

Examples found during research:
- Valijson: draft-7;
- pboettch/json-schema-validator: draft-7.

Using either as the canonical validator would force MCP/provider schemas through a lossy older dialect.

## Candidate

`jsoncons` currently documents support for:
- Draft 4;
- Draft 6;
- Draft 7;
- Draft 2019-09;
- Draft 2020-12.

Its JSON Schema extension documents passing required JSON Schema Test Suite tests for the implemented 2020-12 keywords. It is header-only and uses the Boost Software License.

## Proposed decision

Use jsoncons only behind a narrow `SchemaValidator` interface.

Do not make jsoncons types the SuprAI domain JSON representation.

Conceptual API:

```text
SchemaValidator
  compile(schema)
  validate(compiledSchema, instance)
  validateSchema(schema)
```

Qt JSON/domain values are serialized/converted at the boundary.

## Required proof before acceptance

- pin and build the dependency reproducibly;
- run official JSON Schema 2020-12 test vectors relevant to SuprAI;
- test oneOf/anyOf/allOf/$defs/$ref/unevaluatedProperties;
- disable or strictly control external URI resolution;
- enforce schema depth/size/validation-time limits;
- benchmark typical tool schemas;
- verify AppImage size/compile-time impact;
- review dependency update/security process.

## Sources

- https://github.com/danielaparker/jsoncons
- https://github.com/danielaparker/jsoncons/blob/master/doc/ref/jsonschema/jsonschema.md
