# SuprAI

Linux-first native desktop AI agent and workspace.

SuprAI is a complete Linux-native AI application: its own Qt Quick/QML interface, C++ application core, and native agent runtime. Hermes Agent, OpenClaw, Cline and similar projects are research references only.

**Status: functional modular prototype / M1 in progress.**

Alpha.5 passed its original CI but KDE physical validation exposed that prose rendered literal generated HTML tags: TextEdit.StyledText was not a supported format. Alpha.6 corrects the text format to explicit RichText for **only sanitized C++-generated markup**, retains PlainText for raw streams and adds an actual QTextDocument output regression test. Alpha.6 passed 12/12 CTest including actual ProseText QTextDocument rendering, as well as packaged XCB/Wayland AppImage smoke. Physical KDE visual verification remains open. It can run a deterministic MockRuntime or stream a real OpenAI-compatible Chat Completions endpoint through NativeSuprAIRuntime.

## Current prototype

Implemented:
- native C++20 + Qt Quick/QML application;
- three-pane desktop shell;
- composer, streaming transcript, stop/cancel, new conversation and error state;
- dedicated runtime QThread so provider work does not run on the UI thread;
- dedicated persistence QThread with a thread-affine primary SQLite writer connection;
- XDG config/data/cache/state paths resolved through QStandardPaths and created at startup;
- structured Qt logging categories with stable suprai.* names;
- freedesktop single-instance/activation service over QtDBus with headless degradation;
- explicit Chat / Configuración routes in QML;
- AgentRuntime public contract;
- deterministic MockRuntime;
- provider port separated from concrete adapters;
- tri-state provider capability contract: Unknown / Supported / Unsupported;
- OpenAI-compatible Chat Completions streaming adapter with explicit effective capability reporting;
- application composition root that injects the selected provider/runtime;
- NativeSuprAIRuntime implemented as a thin production facade;
- RuntimeOrchestrator owns stateful conversation/runtime coordination;
- AgentEngine owns the low-level provider execution kernel;
- typed AgentEngineEvent -> RuntimeEvent adapter boundary;
- raw provider `reasoning_content` treated as ephemeral and excluded from later reconstructed prompts;
- explicit public/private module include boundaries enforced by CMake targets;
- CI build, deterministic CTest suite, QML startup smoke, plus an optional authenticated native-runtime/SQLite NInfer remote test;
- CMake-owned portable staging of Qt libraries, QML imports and plugins;
- independent staged smoke tests through XCB/Xvfb and Wayland/Weston with Qt development environment variables removed;
- bundled X11 and Wayland QPA plugins verified in the stage;
- SQLite bootstrap with WAL, foreign keys, busy timeout, schema migrations, quick/integrity checks and FTS5 capability proof;
- schema v1 for sessions, inputs, turns, runs and generalized conversation items;
- typed PersistencePort command/ACK boundary between runtime and SQLite worker;
- native turn-start transaction persists Session/Input/Turn/Run/user item before provider inference begins;
- runtime waits for durable ACK before calling AgentEngine/provider;
- SQLite QSQLITE runtime/plugin included in the portable stage;
- clean worker shutdown enforced by CI;
- newest session restored from SQLite, prepared Runs reconciled as interrupted, and assistant terminal outcome committed before completion ACK;
- initial AppImage preview built from the staged Qt runtime, with bundled Qt/QML/XCB/Wayland/SQLite, SHA-256 file, and packaged XCB/Wayland CI smoke tests;
- alpha.3 provider configuration slice: editable endpoint/model, safe /v1/models discovery, asynchronous QtKeychain-based stored credentials and explicit restart-to-apply (CI passed; physical KDE wallet proof pending).

Not implemented yet:
- older-session selection and advanced ProviderAttempt/Task recovery;
- Responses transport;
- tool calling and approvals;
- MCP;
- memory;
- subagents/deliberation;
- projects/files;
- syntax highlighting, long-transcript benchmarks, keyboard text-selection/accessibility proof, and larger-block virtualization;
- multiple provider profiles with live switching, and secure credential store proof on physical KDE/GNOME;
- release-grade AppImage ABI-floor/cross-distro proof (initial alpha AppImage preview is available through the CI/release pipeline);
- physical KDE Wayland and GNOME Wayland smoke tests (KDE X11/NVIDIA AppImage with NInfer and SQLite recovery verified).

## Module shape

SuprAI is a modular monolith.

```text
QML
 |
 v
ChatController
 |
 v
AgentRuntime
 |----------------------|
 |                      |
MockRuntime      NativeSuprAIRuntime
                       |
                       v
              RuntimeOrchestrator
                       |
                       v
                  AgentEngine
                       |
                       v
                  Provider port
                       ^
                       |
             OpenAI-compatible adapter

ApplicationBootstrap
  wires concrete implementations
```

Current source modules expose only their public `include/suprai/...` surface. Concrete runtime/provider implementations live in private source directories and are not visible to unrelated consumers.

The runtime split required by ADR-0002 is implemented in the current vertical slice. SQLite commits the Session/Input/Turn/Run and the user item before inference, then atomically commits the terminal Run outcome and assistant item before the terminal UI ACK. On startup the native runtime restores the newest local session from SQLite and marks previously prepared Runs as interrupted without replaying them. Selectable older sessions, ProviderAttempt journaling, tool iteration and the remainder of M3 build on these boundaries rather than replacing them.

This is deliberate: adding or replacing a provider should not require changes to QML or stateful orchestration, and session/persistence/task concerns must not accumulate inside the low-level agent execution kernel.

## Native runtime remote integration test

The optional `native_remote_ninfer` CTest target connects the actual C++ OpenAI-compatible provider and NativeSuprAIRuntime to the authenticated external NInfer endpoint. A temporary SQLite database stores a synthetic first turn; a newly instantiated runtime then restores it and must answer a context probe. This complements, but does not replace, the deterministic fake-server regression tests and local physical AppImage testing.

The `remote-ninfer-smoke` workflow discovers the current model alias via `/v1/models`, executes its small Python protocol smoke, and only then runs CTest using the repository secret `NINFER_API_KEY`. PRs cannot access this secret and do not execute the live C++ test. Outputs, keys, and synthetic markers are not printed. The live test should be treated as verified only after its dedicated GitHub Actions job succeeds.

## Remote NInfer CI smoke

`.github/workflows/remote-ninfer-smoke.yml` checks `https://ninfer.minilinux.win/health` from an Ubuntu GitHub-hosted runner. On manual dispatch and main-branch updates to the smoke-test files, the optional authenticated stage discovers the model via `/v1/models`, performs two short synthetic streaming requests, and checks that conversation context can be rebuilt in a second request.

The authenticated test **only runs when repository Actions secret `NINFER_API_KEY` is configured**; otherwise the Actions summary explicitly says `SKIP`. The credential must not be checked into Git, logged, or supplied via untrusted PRs. A public health check does not prove live inference. This tests the remote provider wire protocol, not SuprAI's own SQLite persistence or UI.

The external endpoint is a test service, **not** a permanent default; changing models or ports must not silently change SuprAI application defaults. Manual runs are available from Actions → remote-ninfer-smoke → Run workflow.

## Text rendering (alpha.7 candidate)

Completed assistant messages now have a bounded `messageBlocks` presentation role: safe styled prose plus explicit fenced-code cards, selectable read-only text, monospace code, code-language labels and copy actions. Inline HTTP(S) links are generated from validated Markdown destinations, **never activated by rendering**; a user click immediately delegates to the system browser (no extra modal), with a second strict HTTP(S)/host/userinfo check in C++; rendering itself never opens links. Non-HTTP(S), credential-bearing and malformed links remain inert, with destinations visible. Model-provided HTML/images are not loaded. Streaming and failed/cancelled messages remain plain text, and very large messages safely fall back to non-parsed text.

Alpha.7 candidate also adds fixed-width copy buttons with brief '✓ Copiado' feedback, visible heading size hierarchy, GitHub-style strikethrough and checked/unchecked task markers. These features are pending CI and physical KDE verification. Remaining work: long-transcript performance, tables, callouts/quotes, optional ==highlight==, keyboard accessibility, attachments, richer syntax highlighting, and incremental Markdown during streaming. This is a first native proof rather than a complete document renderer.

## Provider configuration (alpha.4)

The **Configuración** page now edits one active OpenAI-compatible endpoint and model. It can query `/v1/models` using the entered key or the locally stored credential. User-entered API keys are persisted asynchronously through QtKeychain into the desktop's KWallet/Secret Service; no insecure fallback is enabled. Provider URLs and model IDs are the only data stored in QSettings. Secret-store entries are endpoint-hash-scoped. Remote endpoints must use HTTPS; HTTP is restricted to local/private addresses. Discovery never reuses a previously loaded key for a different endpoint.

A change takes effect **after fully closing and reopening the application**. While the model is not yet active, the chat displays a restart/configuration notice and disables Send instead of silently accepting a newline on Enter. When a model is active, Enter submits a message and Shift+Enter inserts a newline. No live runtime mutation is attempted mid-turn. Startup waits asynchronously for a credential-store result before creating NativeSuprAIRuntime; KWallet may prompt to unlock. If a secure store is unavailable, configuration metadata can be saved but credentials cannot. Environment overrides remain available for CI and troubleshooting; they are not a permanent provider default.

This first slice supports **one active endpoint/model**; named provider profiles, deleting profiles, live switching and fuller authenticated probing remain subsequent work. Its build uses the QtKeychain 6 package and includes the project's BSD license notice in AppImage staging.

## AppImage preview

SuprAI can be tested from one executable without installing build-time Qt/CMake dependencies. The experimental `SuprAI-0.1.0-alpha.7-x86_64.AppImage` is generated and verified in GitHub Actions on Ubuntu 24.04 with Qt 6.12.0. Packaged smoke tests run under XCB/Xvfb and headless Wayland/Weston. KDE X11/NVIDIA with real NInfer was tested on 2026-10-11; KDE/GNOME Wayland and other-distro ABI compatibility are not yet proved.

The `appimage-preview` GitHub Actions workflow uploads the AppImage + `.sha256`; after verified `main` builds it publishes the same pair as a GitHub pre-release. To run it on a machine whose local inference server speaks OpenAI-compatible Chat Completions:

```bash
chmod +x SuprAI-0.1.0-alpha.7-x86_64.AppImage
SUPRAI_RUNTIME=native SUPRAI_BASE_URL=http://127.0.0.1:8090/v1 \
  SUPRAI_MODEL=your-model-id SUPRAI_API_KEY=your-local-api-key \
  ./SuprAI-0.1.0-alpha.7-x86_64.AppImage
```

If FUSE is unavailable, add `APPIMAGE_EXTRACT_AND_RUN=1` before the invocation. After saving a model in the Configuración page, close SuprAI completely and reopen it before using the chat; an explicit notice explains why Send is disabled in the current process. For a fake local test, use `SUPRAI_RUNTIME=mock`. Full build instructions follow for contributors.

## Build

Requirements:
- CMake 3.24+;
- Ninja;
- C++20 compiler;
- Qt 6.8+ with the WaylandClient component when building the Linux desktop target;
- Qt6Keychain (e.g. Ubuntu package `qtkeychain-qt6-dev`) and a supported Linux wallet for persistent credentials;
- Wayland client development headers/libraries on the build host.

CI currently verifies with Qt 6.12.0.

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DSUPRAI_BUILD_TESTS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Run the deterministic prototype:

```bash
SUPRAI_RUNTIME=mock ./build/src/suprai
```

Run against an OpenAI-compatible local endpoint:

```bash
SUPRAI_RUNTIME=native \
SUPRAI_BASE_URL=http://127.0.0.1:8090/v1 \
SUPRAI_MODEL=your-model-id \
SUPRAI_API_KEY=your-local-api-key \
./build/src/suprai
```

Current configuration environment variables:
- `SUPRAI_RUNTIME`;
- `SUPRAI_BASE_URL`;
- `SUPRAI_MODEL`;
- `SUPRAI_API_KEY`;
- `SUPRAI_SYSTEM_PROMPT`.

Provider credentials are never persisted in QSettings. The alpha.3 candidate uses QtKeychain for secure asynchronous credential reads/writes, with no insecure fallback. Physical wallet proof remains pending.

## Target

- Linux only.
- Qt 6 + QML UI.
- C++20 core and agent runtime.
- CMake + Ninja.
- AppImage as the first portable artifact.
- Local-first with optional remote model/API endpoints.
- OpenAI-compatible providers first.
- Wayland first; X11 compatibility where practical.
- No Node/Electron dependency in the shipped core application.

## Documentation

AI agents should read in this order:

1. `AGENTS.md`
2. `docs/DOCUMENTATION_POLICY.md`
3. `docs/ARCHITECTURE.md`
4. `docs/ROADMAP.md`
5. `docs/PROJECT_STATE.md`
6. `docs/REFERENCES.md`

Documentation is living and canonical. When research or implementation proves an older decision inferior or wrong, current documentation is replaced or corrected. Git history is the archive.
