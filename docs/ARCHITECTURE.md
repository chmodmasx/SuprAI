# SuprAI Architecture v0

Status: proposed baseline.

## 1. Goals

Build a Linux-native AI desktop workspace with the interaction quality of modern agent desktops while keeping the runtime modular and replaceable.

Primary goals:
- native Qt/QML UI;
- low idle overhead;
- first-class Wayland behavior;
- AppImage distribution;
- local and remote agent operation;
- streaming tool-aware chat;
- explicit approvals and user requests;
- project/workspace awareness;
- desktop integration;
- no permanent dependency on Hermes/OpenClaw internals.

Non-goal for v0:
- implementing a full autonomous agent runtime before the desktop shell is proven.

## 2. Architectural shape

```text
+------------------------------------------------------+
|                    QML Presentation                  |
| chat | sessions | projects | files | settings | UI |
+---------------------------+--------------------------+
                            |
                            v
+------------------------------------------------------+
|                C++ Application Layer                 |
| navigation state | commands | use-cases | adapters  |
+-------------+----------------------+-----------------+
              |                      |
              v                      v
+-------------------------+   +------------------------+
| SuprAI Domain Models    |   | Linux Platform Layer   |
| session/message/tool/...|   | tray/dbus/portal/etc. |
+-------------+-----------+   +------------------------+
              |
              v
+------------------------------------------------------+
|                AgentBackend interface                |
+--------------------+----------------+----------------+
                     |                |
          +----------+----+     +-----+----------------+
          | HermesBackend |     | MockBackend          |
          | JSON-RPC/WS    |     | deterministic tests |
          +----------+----+     +----------------------+
                     |
                     v
             Hermes Agent gateway

Later:
AgentBackend -> NativeSuprAIBackend
```

## 3. Why Qt Quick/QML

Use QML for the interactive shell and C++ for application/runtime logic.

Rationale:
- native Linux process and window integration;
- strong Wayland support through Qt platform plugins;
- GPU-accelerated scene graph;
- declarative UI suitable for dynamic agent surfaces;
- no Chromium runtime required for the core chat UI;
- C++ access to D-Bus, filesystem, QProcess, sockets and desktop services;
- straightforward CMake packaging.

Baseline modules:
- Qt::Core
- Qt::Gui
- Qt::Qml
- Qt::Quick
- Qt::QuickControls2
- Qt::Network
- Qt::WebSockets
- Qt::Sql
- Qt::DBus
- Qt::Concurrent
- Qt::Svg

Optional later:
- Qt::Multimedia
- Qt::Pdf
- Qt::WebEngineQuick

Qt WebEngine is deliberately optional because it materially increases binary size, memory use, security surface and packaging complexity.

## 4. C++ standard

Use C++20 initially.

Reasons:
- mature compiler availability on intended Linux build baselines;
- sufficient coroutines/types/concepts support where useful;
- lower toolchain friction than requiring newer language modes without a demonstrated need.

## 5. Repository structure

Proposed:

```text
/
  AGENTS.md
  CMakeLists.txt
  cmake/
  src/
    app/
      main.cpp
      Application.*
    domain/
      Session.*
      Message.*
      ToolCall.*
      AgentEvent.*
      BackendCapabilities.*
    backends/
      AgentBackend.*
      hermes/
        HermesBackend.*
        HermesRpcClient.*
        HermesMapper.*
      mock/
        MockBackend.*
    services/
      SessionService.*
      ProjectService.*
      SettingsService.*
      SecretService.*
    platform/
      linux/
        LinuxDesktopIntegration.*
        LinuxNotifications.*
        LinuxTray.*
        LinuxPortal.*
        LinuxSecretStore.*
    persistence/
      AppDatabase.*
      migrations/
    qml/
      Main.qml
      shell/
      chat/
      sessions/
      projects/
      files/
      settings/
      components/
      theme/
  tests/
    unit/
    backend-contract/
    integration/
  packaging/
    appimage/
    desktop/
  docs/
    ARCHITECTURE.md
    ROADMAP.md
    PROJECT_STATE.md
    REFERENCES.md
    adr/
```

## 6. AgentBackend boundary

The desktop must be backend-neutral.

### Required capability groups

```text
connection
sessions
messages
streaming
turn-control
user-requests
models
profiles
tools
skills
files?       capability-gated
terminal?    capability-gated
memory?      capability-gated
voice?       capability-gated
```

A backend announces capabilities at connection time.

The UI must not infer capability from backend name.

Example:
```cpp
struct BackendCapabilities {
    bool sessions;
    bool cancelTurn;
    bool approvals;
    bool profiles;
    bool skills;
    bool remoteFiles;
    bool terminal;
    bool clientTools;
};
```

## 7. Hermes adapter

Hermes is the recommended first real backend because it already exposes a bidirectional JSON-RPC protocol through its gateway.

Known upstream properties:
- WebSocket transport;
- JSON-RPC in both directions;
- server -> client requests for approvals/clarifications/secrets/etc.;
- server -> client event notifications;
- session creation/resume;
- prompt submission;
- streaming events;
- local or remote gateway operation.

Implementation rule:
- `HermesRpcClient` speaks Hermes protocol.
- `HermesMapper` converts Hermes payloads to SuprAI domain types.
- `HermesBackend` implements `AgentBackend`.
- no QML code knows a Hermes RPC method name.

Local Hermes lifecycle is optional:
- connection mode A: attach to existing local Hermes;
- mode B: spawn/manage a Hermes gateway child process;
- mode C: connect to remote Hermes.

Do not assume local process ownership merely because the endpoint is localhost.

## 8. Native SuprAI agent

Not v0.

When implemented, it must plug into the same `AgentBackend` contract.

Likely components:
- provider abstraction;
- OpenAI-compatible Responses/Chat transport;
- tool registry;
- MCP client;
- approval policy;
- session persistence;
- memory;
- skill registry;
- agent loop;
- context management;
- subagents/background jobs.

Do not build these into UI classes.

## 9. UI information architecture v0

### Main shell

```text
+--------------------------------------------------------------+
| top bar: project / backend / model / status                  |
+--------------+--------------------------------+--------------+
| left rail    | main chat                      | inspector     |
|              |                                |              |
| new chat     | transcript                     | tool details |
| sessions     |                                | files        |
| projects     |                                | preview      |
| agents       |                                | artifacts    |
| skills       | composer                       | terminal     |
+--------------+--------------------------------+--------------+
```

Inspector is collapsible.

### Chat must support
- Markdown;
- code blocks;
- streaming;
- reasoning summary/status when backend exposes it;
- tool call cards;
- approvals;
- clarifications;
- attachments;
- image input;
- stop/cancel;
- retry/branch only when backend supports it.

Do not design around showing hidden chain-of-thought.

## 10. Project/workspace model

A Project is a desktop-owned workspace abstraction.

It may contain:
- display name;
- one or more directories;
- repositories;
- preferred backend/profile;
- sessions;
- UI layout metadata.

A session can exist without a Project.

Filesystem authority depends on backend mode:
- local backend: local workspace;
- remote backend: remote workspace unless a client capability explicitly exposes local files.

This distinction must always be visible to the user.

## 11. Persistence

Use SQLite for SuprAI-owned durable metadata.

Store:
- projects;
- connection definitions without raw secrets;
- UI/session associations;
- window state;
- recent items;
- migration version.

Do NOT duplicate backend conversation history unless there is a concrete feature requiring a cache.

Secrets:
- store references/IDs in SQLite;
- actual secret material in the Linux secret backend.

## 12. Linux integration

Desired capabilities:
- system tray / status notifier;
- notifications;
- single instance;
- desktop file;
- URL/deep-link handler;
- file chooser;
- clipboard;
- drag/drop;
- portal-aware file/screenshot interaction;
- optional global shortcut;
- optional autostart;
- user-service integration where justified.

Wayland first.

Global shortcut support must be capability-gated because compositor policies differ.

## 13. Local process model

Potential process tree:

```text
suprai
  └─ optional managed backend
       └─ hermes serve
```

Rules:
- stdout/stderr captured to structured logs;
- crash detected;
- bounded restart policy;
- user can inspect failure reason;
- do not kill an external backend the app did not start;
- on quit, ownership determines whether child backend is stopped.

## 14. Transport

Hermes v0:
- QWebSocket;
- JSON-RPC 2.0-like request IDs;
- bidirectional requests;
- event notifications;
- explicit timeout policy;
- reconnect state machine.

States:
```text
Disconnected
Resolving
Connecting
Authenticating
Ready
Reconnecting
Degraded
Failed
```

Do not compress all failure conditions into "offline".

## 15. Logging

Structured log categories:
- app.lifecycle
- backend.connection
- backend.rpc
- backend.events
- session
- platform
- persistence
- packaging

Sensitive fields must be redacted before logging.

Recommended runtime location:
`$XDG_STATE_HOME/suprai/logs/`
with XDG fallback behavior.

## 16. Configuration

Follow XDG directories.

Suggested:
- config: `$XDG_CONFIG_HOME/suprai/`
- data: `$XDG_DATA_HOME/suprai/`
- cache: `$XDG_CACHE_HOME/suprai/`
- state/logs: `$XDG_STATE_HOME/suprai/`

Never use a hidden home directory when an XDG location fits.

## 17. AppImage

Build strategy:
1. CMake install into AppDir.
2. deploy executable and Qt runtime/plugins/QML imports.
3. add desktop file/icon/AppRun metadata.
4. build AppImage.
5. smoke-test on the declared ABI floor.

Use shared Qt libraries inside the AppImage rather than static Qt unless licensing/technical review explicitly changes this.

The build image defines the ABI floor. Keep it intentional.

## 18. Compatibility

Target classes:
- KDE Plasma Wayland;
- GNOME Wayland;
- X11 fallback;
- common x86_64 distributions meeting AppImage ABI floor.

Initial release architecture:
- x86_64 first.
- arm64 later if CI and dependency closure are clean.

## 19. Security model

Trust boundaries:
1. user;
2. QML renderer;
3. C++ application;
4. local filesystem/platform;
5. backend;
6. remote tool execution;
7. untrusted preview content.

Rules:
- explicit capability bridges;
- no "execute arbitrary native command" QML API;
- backend-provided HTML is untrusted;
- user-visible host identity for remote execution;
- secret requests identify requester/backend/session;
- dangerous tools require backend policy and visible approval state.

## 20. Definition of v0 architecture complete

Architecture v0 is proven when:
- Qt shell boots;
- MockBackend streams a deterministic conversation;
- state model handles tools + approval request;
- HermesBackend connects to a real gateway;
- new session + prompt + streaming + cancel works;
- app survives backend disconnect/reconnect;
- AppImage launches on clean test VM;
- no Hermes-specific type leaks into QML.
