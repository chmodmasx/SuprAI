# ADR-0006: MCP client targets the 2026-07-28 protocol era

Status: accepted
Date: 2026-10-06

## Decision

SuprAI's MCP implementation targets the current final MCP revision `2026-07-28` as its canonical modern protocol.

MCP is an adapter into SuprAI's Tool/Resource/Prompt domain. It is not a second agent loop.

## Modern assumptions

Design for:
- stateless core;
- modern routing/version headers;
- Multi Round-Trip Requests when required;
- JSON Schema 2020-12 tool input/output;
- extension-aware capability handling.

Do not build new SuprAI architecture around deprecated MCP roots, server-side sampling or protocol logging.

## C++ implementation

The current official MCP SDK matrix does not list C++.

Initial strategy:
- implement a deliberately small SuprAI-owned client;
- Qt JSON serialization;
- `QProcess` stdio transport;
- QtNetwork Streamable HTTP transport;
- strict protocol-era adapter;
- conformance fixtures based on official spec examples/tests.

Do not implement unused protocol surface.

Initial useful methods:
- tools list/call;
- resources list/read where needed;
- prompts list/get where needed;
- structured results;
- cancellation/timeouts;
- required discovery/version negotiation;
- MRTR when a real integration requires it.

## Legacy

Compatibility with the 2025 protocol era may be implemented behind an explicit legacy adapter.

Legacy semantics must not leak into NativeSuprAIRuntime.

## Revisit

Revisit the custom-client choice if an official high-quality C++ SDK appears or a community SDK demonstrates clear maintenance, conformance, security and packaging advantages.
