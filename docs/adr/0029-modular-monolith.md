# ADR-0029: Modular monolith with explicit ports and composition root

Status: accepted
Date: 2026-10-06

## Decision

SuprAI is implemented initially as a **modular monolith**:

- one primary native application process;
- independently testable C++ modules;
- explicit public interfaces;
- private implementations;
- directed dependency rules;
- one composition root that wires implementations together;
- optional capabilities can be added, replaced, disabled, or compiled out without invasive changes to unrelated modules.

Modularity is a correctness requirement, not only a source-tree preference.

## Goals

A change to one subsystem should normally require changes only to:
1. that subsystem;
2. its explicit interface/contract when the contract itself changes;
3. the composition root/configuration;
4. contract/integration tests at the affected boundary.

Examples:
- add a provider without editing AgentLoop internals;
- replace SQLite repository implementation without changing QML;
- remove MCP without changing provider code;
- add a tool without changing ToolExecutor;
- replace notification backend without touching runtime;
- change memory retrieval without changing canonical conversation persistence;
- add another subagent purpose without creating another runtime.

## Non-goal: early public plugin ABI

This ADR does NOT establish a stable binary plugin ABI.

Do not introduce a public dlopen/Qt-plugin ABI before real external consumers prove the need and shape.

Reasons:
- C++ ABI/versioning burden;
- Qt version coupling;
- security boundary complexity;
- packaging complexity;
- harder evolution during early architecture work.

Initial modularity is source/interface/configuration modularity inside the application.

A future public plugin ABI requires a separate ADR.

## Module rule

Every significant subsystem has:

```text
module/
  public/
    interfaces + value contracts
  internal/
    implementation
  tests/
```

Repository layout may use equivalent CMake/source conventions; the architectural rule matters more than exact folder names.

Consumers include only the module's public surface.

No module may include another module's `internal/` headers.

## CMake target rule

Each significant subsystem is its own CMake target where practical.

Example:

```text
suprai_domain
suprai_provider_api
suprai_provider_openai
suprai_runtime
suprai_ui
suprai_tools
suprai_context
suprai_memory
suprai_mcp
suprai_persistence
suprai_platform_linux
suprai_app
```

Optional concrete adapters may have narrower targets:

```text
suprai_provider_openai
suprai_secret_qtkeychain
suprai_containment_landlock
suprai_containment_bwrap
```

The final executable links selected implementations from the composition root.

Current prototype proof:
- `suprai_provider_api` contains the QObject provider contract;
- `suprai_provider_openai` contains the concrete Chat Completions adapter;
- `suprai_runtime` links only the provider contract;
- concrete `NativeSuprAIRuntime` and `MockRuntime` classes are private runtime implementation headers;
- the public runtime surface exposes `AgentRuntime` plus narrow creation functions;
- `suprai_ui` exposes ChatController while TranscriptModel remains private;
- `ApplicationBootstrap` creates the concrete provider and injects it into the native runtime;
- CI compiles/tests this layout without a broad global source include directory.

Avoid one giant target containing the entire product.

## Dependency direction

Canonical high-level direction:

```text
QML/UI
   |
   v
Application
   |
   v
Domain contracts
   ^
   |
Runtime/orchestration
   |
   +---- ports ----> Provider
   +---- ports ----> Tool system
   +---- ports ----> Context
   +---- ports ----> Memory
   +---- ports ----> Persistence
   +---- ports ----> MCP
   +---- ports ----> Platform services
```

Concrete infrastructure depends on contracts, not the reverse.

Examples:
- runtime knows `IProvider`, not NInfer/vLLM/llama.cpp classes;
- runtime knows persistence/repository ports, not SQLite queries;
- UI knows application/domain projections, not Provider or SQL objects;
- tool execution knows containment/policy interfaces, not one hardcoded backend;
- platform-neutral modules do not include Linux implementation headers.

## Domain layer

Domain contains stable value semantics and identifiers:
- Session;
- Input;
- Turn;
- Run;
- ConversationItem;
- ProviderAttempt;
- ToolInvocation;
- Task;
- AgentEvent;
- capability/value types.

Domain must not depend on:
- QML;
- Qt Network;
- Qt SQL;
- provider wire formats;
- MCP transport details;
- Linux desktop APIs.

Using small Qt Core value types may be permitted when justified, but domain contracts must not acquire infrastructure behavior.

## Ports and adapters

Use dependency inversion at architecture-significant boundaries.

Examples:

```text
ProviderPort
  <- OpenAICompatibleProvider

SessionRepository
  <- SQLiteSessionRepository

SecretStore
  <- QtKeychainSecretStore

ContainmentBackend
  <- LandlockBackend
  <- BubblewrapBackend
  <- NoContainmentBackend

NotificationService
  <- PortalNotificationBackend
  <- FreedesktopNotificationBackend
```

Do not create an interface for every trivial class. Add a port where replacement, policy isolation, testing, or platform/provider variation is real.

## Registries

Open-ended extension families use registries rather than central switch statements.

Examples:
- ProviderRegistry;
- ToolRegistry;
- SkillRegistry;
- model capability registry;
- containment backend registry where useful.

Adding one provider/tool/skill should not require editing a global enum + switch across multiple modules.

Registration occurs through the composition root or a narrowly owned registry API.

## Composition root

All concrete wiring lives in one application/bootstrap area.

Conceptually:

```text
ApplicationBootstrap
  create Persistence
  create PlatformServices
  create ProviderRegistry
  create ToolRegistry
  create ContextManager
  create MemoryService
  create TaskManager
  create NativeSuprAIRuntime
  connect UI facade
```

Do not let modules instantiate arbitrary concrete implementations from other modules.

Avoid global service locators/singletons as hidden dependency injection.

Dependencies are constructor/factory inputs or explicit narrow runtime references.

## Optional capabilities

Features that are genuinely optional must expose capability state.

Examples:
- MCP;
- tray;
- global shortcuts;
- WebEngine preview;
- voice;
- PDF support;
- containment backend;
- secure persistent secret store.

Optionality rules:
- absence must be representable;
- UI/runtime behavior degrades explicitly;
- no scattered compile-time assumptions;
- do not crash because an optional module is absent.

CMake options may select modules, but `#ifdef` branches should be confined to build/adaptor/composition boundaries rather than spread through domain/runtime code.

## Replaceability examples

### Provider

```text
Agent runtime
    |
    v
ProviderPort
    |
    +-- OpenAICompatibleProvider
          |
          +-- ResponsesTransport
          +-- ChatCompletionsTransport
```

Adding a future native provider should not change AgentLoop/TurnStateMachine semantics.

### Persistence

```text
Runtime
  -> repository ports
       -> SQLite adapters
```

SQLite schema details do not escape persistence.

### Tool

```text
ToolRegistry
  + built-in tool
  + MCP-mapped tool
  + skill-provided tool
```

AgentLoop consumes normalized ToolDefinition/Invocation/Result only.

### Deliberation

```text
Subagent Task
  purpose = deliberation
```

No separate reasoning runtime is introduced.

## Events and commands

Cross-module asynchronous communication uses explicit commands/events/value objects.

Do not use arbitrary QObject pointer reach-through between modules.

Rules:
- commands express requested action;
- events express observed fact/state transition;
- domain events use provider/platform-neutral types;
- high-frequency UI projections may be coalesced after domain semantics are preserved.

An event bus must not become an untyped global dumping ground. Events remain owned and documented.

## Data ownership

Each persistent data family has one owning module/service.

Examples:
- canonical conversation state: persistence/runtime contract;
- provider configuration: settings/provider configuration owner;
- secrets: SecretStore only;
- memory: MemoryService;
- task lifecycle: TaskManager;
- schedules: scheduling service.

No subsystem reads another subsystem's SQLite tables directly.

Cross-domain access goes through an explicit query/port/service contract.

## Configuration ownership

Configuration is structured and namespaced by module.

Example conceptually:

```text
providers.*
runtime.*
tools.*
mcp.*
memory.*
platform.*
ui.*
```

A module validates its own configuration.

Do not build one untyped map of arbitrary settings consumed everywhere.

## Failure isolation

Module boundaries also define failure behavior.

Examples:
- MCP unavailable -> MCP capability degraded, core chat continues;
- tray unavailable -> no tray, app continues;
- one provider unhealthy -> other configured providers remain valid;
- memory indexing failure -> canonical history remains intact;
- notification failure -> Task execution status remains successful;
- containment backend unavailable -> policy can deny/degrade explicitly; never fake sandboxing.

One optional subsystem failure must not corrupt unrelated authoritative state.

## Testing contract

Every module requires:
- unit tests for local logic;
- contract tests for public ports;
- fake/test adapter where valuable;
- integration tests only at real boundaries.

Key replacements must be exercised:

```text
Native provider <-> FakeProvider
SQLite repository <-> InMemory/Fake repository where suitable
real tool <-> deterministic fake tool
Linux service <-> fake platform service
NativeSuprAIRuntime <-> MockRuntime
```

Tests must detect forbidden dependency creep.

## Dependency enforcement

M1 should establish enforcement mechanisms, not rely only on discipline.

At minimum:
- separate CMake targets;
- public/private include paths;
- target_link_libraries visibility;
- no broad global include directories;
- no umbrella internal header exposing the whole codebase.

Later add an automated architecture/dependency check if ordinary CMake target boundaries prove insufficient.

## Change rule

When adding a feature, first identify:
- owning module;
- public contract required;
- dependencies it is allowed to consume;
- whether it is optional;
- capability/failure behavior;
- tests at the boundary.

If a feature requires unrelated modules to know its implementation details, the boundary is probably wrong.

## Consequences

Benefits:
- easier replacement and experimentation;
- fewer regressions across unrelated features;
- deterministic testing;
- optional features become genuinely optional;
- local-model/provider evolution is contained;
- future plugin ABI remains possible.

Costs:
- more explicit contracts/types;
- composition/bootstrap code;
- occasional adapter mapping;
- discipline against "just include this internal header".

These costs are intentional and cheaper than architectural coupling.
