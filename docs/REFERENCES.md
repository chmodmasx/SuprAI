# Reference Notes

Purpose: preserve architecture lessons and current external contracts without coupling SuprAI to another agent implementation.

Last major research pass: 2026-10-06.

## Research policy

Reference projects are used to:
- discover solved engineering problems;
- compare failure modes;
- validate architecture choices;
- avoid reinventing established interoperable formats.

They are not production runtime dependencies unless a future ADR explicitly approves one.

Never copy source merely because a pattern is useful. Check the source license and reimplement/adapt deliberately.

## Hermes Agent / Hermes Desktop

Upstream:
- https://github.com/NousResearch/hermes-agent

Useful lessons:
- strong authority boundaries between UI/machine/backend state;
- explicit user-action/approval flows;
- bounded curated memory;
- SQLite + FTS5 conversation search;
- project/session identity;
- local/remote execution must be explicit;
- compatibility probing.

Do not copy:
- Hermes runtime dependency;
- Electron/React desktop stack;
- Hermes gateway protocol as SuprAI's internal domain.

Relevant source paths:
- apps/desktop/AGENTS.md
- apps/desktop/DESIGN.md
- tui_gateway/
- website/docs/user-guide/features/memory.md

## OpenClaw

Upstream:
- https://github.com/openclaw/openclaw

Useful lessons:
- Linux lifecycle and packaging are architectural concerns;
- trust boundaries for privileged native bridges;
- Wayland/X11 need separate proof;
- memory provenance/trust metadata;
- external content cannot inherit native authority.

Do not copy:
- Tauri/WebKit frontend;
- OpenClaw runtime/gateway as SuprAI's engine.

## Goose

Upstream:
- https://github.com/aaif-goose/goose

Relevant source:
- crates/goose/src/agents/state_machine/
- crates/goose/src/context_mgmt/
- crates/goose/src/execution/
- crates/goose/src/agents/mcp_client.rs

Useful lesson:
A mature agent loop becomes a state machine. Current source separates LLM execution, tool calling, approvals, retries, compaction, max-turn control, steering, subagents and skills.

SuprAI response:
- ADR-0004 requires explicit turn state transitions/effects.

## GPT4All

Upstream:
- https://github.com/nomic-ai/gpt4all

Relevant source:
- gpt4all-chat/src/
- gpt4all-chat/CMakeLists.txt

Useful lesson:
Qt/QML/C++ is proven for a substantial native local-AI desktop.

Caution:
Avoid accumulating inference, persistence, context, tools and UI behavior into one ChatLLM-style god object.

## OpenCode

Documentation:
- https://opencode.ai/v2/docs/permissions

Useful lesson:
- allow / ask / deny;
- resource-pattern permission scopes;
- explicit external-directory boundary.

Critical lesson:
Approval does not reduce host authority. Shell execution still has the user's filesystem/process/network access unless OS containment is applied.

SuprAI response:
- ADR-0005 separates PolicyEngine from ContainmentBackend.

## Model Context Protocol

Current final revision:
- 2026-07-28.

Primary:
- https://blog.modelcontextprotocol.io/posts/2026-07-28/
- https://plan.modelcontextprotocol.io/matrix
- https://ts.sdk.modelcontextprotocol.io/v2/protocol-versions

Important:
- stateless core;
- MRTR;
- full JSON Schema 2020-12 tool schemas;
- formal extensions;
- roots/sampling/protocol logging deprecated;
- modern ping removed;
- protocol-era compatibility matters.

Current official SDK matrix does not list C++.

SuprAI response:
- ADR-0006 targets 2026-07-28 and initially uses a small native client over Qt/QProcess/QtNetwork.

## Agent Skills

Specification:
- https://agentskills.io/specification

Important:
- SKILL.md required;
- YAML name + description;
- optional scripts/references/assets;
- progressive disclosure;
- optional compatibility/license/metadata;
- allowed-tools is experimental.

SuprAI response:
- ADR-0007 adopts the standard.
- allowed-tools never bypasses PolicyEngine.

## Local inference servers

### llama.cpp
- https://github.com/ggml-org/llama.cpp/blob/master/tools/server/README.md

Current relevant capabilities:
- Chat Completions;
- Responses;
- SSE;
- tool/function calling;
- multimodal;
- schema-constrained output support.

### vLLM
- https://docs.vllm.ai/en/stable/api/vllm/entrypoints/openai/responses/

Relevant:
- Responses implementation;
- typed streaming events;
- reasoning/tool parsing ecosystem.

### NInfer
- https://github.com/Neroued/ninfer/blob/master/docs/serving.md

Relevant:
- /v1/chat/completions;
- /v1/responses;
- typed Items/SSE;
- reasoning separated from answer text;
- function calls;
- input-token endpoint;
- local response state/continuations;
- explicit rejection of unsupported fields.

SuprAI response:
- ADR-0003 uses normalized inference types, Responses preferred, Chat compatibility.

## JSON Schema

Current canonical dialect for SuprAI tools:
- JSON Schema 2020-12.

Primary spec:
- https://json-schema.org/draft/2020-12/json-schema-core.html

MCP 2026-07-28 also uses full JSON Schema 2020-12 for tool input/output schemas.

### C++ validator research

jsoncons:
- https://github.com/danielaparker/jsoncons
- documents Draft 2020-12 support;
- header-only;
- Boost Software License;
- documents required-test-suite compliance for implemented keywords.

Valijson:
- current stated target is draft-7.

pboettch/json-schema-validator:
- current stated target is draft-7.

SuprAI response:
- ADR-0009 proposes jsoncons behind a narrow SchemaValidator interface, pending proof.

## Linux containment

### Landlock
- https://landlock.io/

Useful:
- unprivileged self-restriction;
- stackable LSM;
- reduces ambient rights.

### bubblewrap
- https://github.com/containers/bubblewrap

Useful:
- mount/user/PID/network namespace construction;
- seccomp support;
- no-new-privileges.

Critical:
- bubblewrap explicitly states it is a low-level sandbox construction tool, not a complete security policy;
- Ubuntu 24.04+ restricts unprivileged user namespaces through AppArmor;
- AppImage cannot assume bwrap is always usable.

Ubuntu references:
- https://documentation.ubuntu.com/release-notes/24.04/
- https://documentation.ubuntu.com/security/security-features/privilege-restriction/apparmor/

SuprAI response:
- PolicyEngine always;
- containment feature-probed;
- Landlock/bwrap/none are distinct effective states.

## Qt

Current target family:
- Qt 6.12.

Primary:
- https://doc.qt.io/qt-6.12/supported-platforms.html
- https://doc.qt.io/qt-6/qt-releases.html
- https://doc.qt.io/qt-6/qt-generate-deploy-qml-app-script.html
- https://doc.qt.io/qt-6/linux-deployment.html

Current facts:
- Qt 6.12.0 released 2026-09-30;
- Qt 6.12 is an LTS line;
- immediate extended LTS patch access has commercial-license distinctions;
- Ubuntu 22.04 x86_64/GCC 11 is a supported configuration;
- official Linux Online Installer binaries are built on Ubuntu 24.04/glibc 2.39;
- CMake deployment APIs can stage Qt runtime dependencies/plugins/QML.

Licensing:
- https://www.qt.io/development/open-source-lgpl-obligations

Before distribution, audit each module and satisfy LGPL/GPL obligations.

## AppImage

Primary guidance:
- https://docs.appimage.org/reference/best-practices.html
- https://docs.appimage.org/introduction/concepts.html

Important:
- build on a base no newer than the oldest supported target;
- AppImage does not remove glibc/libstdc++ ABI floors;
- host graphics/system libraries require deliberate exclusion/inclusion decisions.

linuxdeploy Qt plugin:
- https://github.com/linuxdeploy/linuxdeploy-plugin-qt

It supports Qt 6 and QML, but Wayland/QML edge cases make it unsuitable as the sole dependency-discovery authority for a Wayland-first application without proof.

SuprAI response:
- ADR-0010 proposes Qt/CMake-owned AppDir staging and AppImage finalization afterward.

## Memory references

Hermes:
- bounded curated MEMORY/USER data;
- SQLite+FTS5 session history.

OpenClaw:
- curated files plus indexed history;
- provenance/trust stored separately from recalled prose;
- optional vector layer.

SuprAI response:
- ADR-0008 starts with SQLite canonical history + FTS5 + bounded curated memory with provenance;
- vector retrieval is deferred.

## Remaining research backlog

Before M1 completion:
- prove exact Qt 6.12 build toolchain on candidate Ubuntu 22.04 baseline;
- audit minimum Qt modules and licenses;
- benchmark QML transcript rendering strategy.

Before M3:
- define normalized InferenceRequest/InferenceEvent contract exactly;
- build fake SSE compatibility corpus from llama.cpp/vLLM/NInfer;
- decide provider capability persistence/probing rules.

Before M4:
- prototype Landlock capability probe;
- prototype bubblewrap availability/failure behavior;
- define initial built-in tool set and risk taxonomy.

Before M5:
- implement MCP protocol-era conformance fixtures;
- verify actual third-party MCP servers against 2026-07-28 + legacy fallback.

Before M6:
- define memory mutation approval UX;
- measure FTS5 quality before considering vectors.

Before release:
- choose SuprAI project license;
- perform Qt/module/dependency license audit;
- produce notices/source-offer/relinking documentation as required.
