# ADR-0001: Native Qt desktop stack

Status: accepted
Date: 2026-10-06

## Decision

SuprAI's primary desktop implementation will use:
- C++20;
- Qt 6;
- Qt Quick/QML;
- CMake/Ninja;
- Linux as the only initial desktop target;
- AppImage as the first portable distribution artifact.

Electron, Tauri and browser-shell architectures are not the primary implementation.

## Context

The product goal is a Linux-native AI workspace with deep desktop integration, low runtime overhead, Wayland support and no mandatory Chromium/Node runtime.

Hermes Desktop demonstrates a strong frontend/backend separation but currently uses Electron/React. OpenClaw's Linux companion demonstrates the value of treating Linux integration and packaging as first-class architecture, but uses Tauri/WebKit.

## Consequences

Positive:
- Qt-native process/window model;
- direct Qt D-Bus/network/filesystem integration;
- QML suitable for animated agent UI;
- no web runtime required for core chat;
- natural AppImage path.

Costs:
- custom UI components require more work than React ecosystem equivalents;
- rich HTML/browser preview may later require Qt WebEngine;
- Linux deployment of Qt/QML modules must be tested carefully;
- Qt licensing obligations must be reviewed before public binaries are released.

## Revisit if

Revisit only if a proof shows Qt cannot meet a required interaction/platform capability, or packaging/runtime cost becomes materially worse than alternatives.
