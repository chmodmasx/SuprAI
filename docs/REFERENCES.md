# Reference Notes

Purpose: preserve architecture lessons from upstream projects without coupling SuprAI to their implementation.

## Hermes Agent / Hermes Desktop

Upstream:
- https://github.com/NousResearch/hermes-agent

Observed architecture:
- Desktop separates machine shell, renderer, and agent backend.
- Current Hermes Desktop uses Electron + React for the desktop surface.
- Hermes Agent runs headless and exposes a gateway.
- Desktop connects through JSON-RPC/WebSocket.
- The protocol is bidirectional: client requests, backend events, and backend -> client requests.
- Backend remains authoritative for sessions/tools/model work.
- Desktop supports local and remote gateways.
- Hermes documentation explicitly treats UI state, machine state, and backend state as separate authorities.

Important source locations at time of research:
- `apps/desktop/README.md`
- `apps/desktop/AGENTS.md`
- `apps/desktop/DESIGN.md`
- `apps/shared/src/json-rpc-channel.ts`
- `tui_gateway/ws.py`
- `tui_gateway/transport.py`
- `tui_gateway/AGENTS.md`
- `tui_gateway/contracts/`

Lesson to adopt:
- clean authority boundaries;
- gateway-neutral UI;
- compatibility probing;
- explicit local/remote execution identity;
- narrow native capability bridge;
- state scoped by connection/profile/session/project;
- reconnect is a state transition, not a full application reboot.

Lesson NOT to copy:
- Electron/React stack, because SuprAI's explicit goal is native Qt/Linux.

## OpenClaw

Upstream:
- https://github.com/openclaw/openclaw

Relevant source:
- `apps/linux/`

Observed Linux companion:
- Tauri v2 shell;
- local/remote gateway model;
- system tray;
- notifications;
- setup/discovery;
- AppImage packaging;
- Linux-specific regression tests;
- attention to Wayland/X11 and native titlebar behavior;
- browser/reading content isolated from privileged dashboard bridge;
- explicit build ABI floor.

Lesson to adopt:
- Linux packaging must be treated as architecture, not a final build-script detail;
- native capabilities must be scoped;
- ownership matters when starting/stopping local services;
- test X11 and Wayland separately;
- AppImage build environment determines ABI compatibility;
- external/untrusted web content requires a separate trust boundary.

Lesson NOT to copy by default:
- Tauri/WebKit frontend. SuprAI will use Qt Quick/QML unless an ADR changes this.

## Qt

Target family:
- Qt 6.

At project initialization in October 2026:
- Qt 6.11 is a mature current line.
- Qt deployment on Linux requires explicit handling of runtime libraries, plugins, and QML modules.
- CMake-based Qt deployment APIs should be preferred where useful.
- AppImage-specific assembly still requires an AppDir/AppImage toolchain.

Policy:
- do not tie source architecture to one patch release;
- pin the CI/release toolchain reproducibly;
- record the exact release Qt version in build metadata.

## AppImage

Potential tool family:
- linuxdeploy
- linuxdeploy Qt plugin
- appimagetool

Do not finalize tooling before the M1/M6 proof because QML/module deployment details can alter the best choice.

Important:
- build on a deliberate baseline;
- inspect resulting GLIBC/GLIBCXX requirements;
- avoid bundling host graphics/Wayland libraries blindly;
- test the actual downloadable artifact, not only the unbundled executable.

## Research backlog

Before M3:
- extract Hermes RPC contract needed by SuprAI v0;
- map RPC methods/events to domain types;
- identify version/capability handshake;
- document local Hermes spawn/ownership semantics;
- document auth and remote gateway behavior.

Before M5:
- compare QSystemTrayIcon vs StatusNotifier-specific behavior;
- select secret storage library/API;
- investigate XDG Desktop Portal GlobalShortcuts support from Qt/C++;
- define notification implementation.

Before M6:
- test linuxdeploy + Qt plugin against selected Qt version/QML modules;
- compare with custom CMake deployment + appimagetool;
- establish oldest supported runtime environment.
