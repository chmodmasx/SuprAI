# ADR-0010: Stage AppImage with Qt CMake deployment APIs

Status: proposed
Date: 2026-10-06

## Proposed decision

Make the CMake install tree / AppDir the source of truth.

Preferred pipeline:

```text
CMake build
  -> cmake --install into AppDir
  -> qt_generate_deploy_qml_app_script / Qt deployment APIs
  -> verify Qt libs/plugins/QML modules
  -> add desktop metadata/AppRun
  -> AppImage finalizer
  -> smoke tests
```

Use `appimagetool` directly or linuxdeploy narrowly as a finalizer if it proves useful.

Do not make linuxdeploy-plugin-qt the only authority for discovering the Qt dependency closure.

## Why

Qt 6.12 provides CMake deployment APIs that:
- inspect runtime dependencies;
- deploy Qt libraries;
- deploy plugins;
- deploy QML imports;
- generate qt.conf.

linuxdeploy-plugin-qt does support Qt 6/QML and optional Wayland packaging, but its history includes Wayland/QML edge cases. SuprAI is Wayland-first, so the portable artifact should have an inspectable deterministic staging step under our CMake configuration.

## ABI strategy

AppImage does not erase glibc/libstdc++ compatibility.

Build ingredients must be compiled on a base no newer than the oldest supported target.

Qt 6.12 officially supports Ubuntu 22.04 x86_64/GCC 11, making Ubuntu 22.04 a strong candidate x86_64 build baseline.

Important: official Qt Online Installer binaries are built on Ubuntu 24.04/glibc 2.39, so using those binaries would undermine an Ubuntu-22.04-compatible AppImage. For the old-baseline build we may need Qt built on/from the baseline or another verified compatible Qt artifact.

## Required proof before acceptance

Build an M1 AppImage and test:
- Ubuntu 22.04;
- Ubuntu 24.04;
- Ubuntu 26.04;
- Debian 12;
- KDE Wayland;
- GNOME Wayland;
- X11/XWayland fallback.

Inspect:
- GLIBC symbol floor;
- GLIBCXX symbol floor;
- QPA plugins;
- Wayland plugin closure;
- QML imports;
- SQLite driver;
- SVG/image plugins;
- NSS/SSL assumptions if introduced.

## Licensing gate

Before public distribution:
- audit Qt modules and third-party components;
- prefer dynamic Qt libraries;
- include LGPL notices;
- provide required Qt source availability/relinking information;
- avoid GPL-only Qt modules unless SuprAI's license intentionally permits them.

## Sources

- https://doc.qt.io/qt-6/linux-deployment.html
- https://doc.qt.io/qt-6/qt-generate-deploy-qml-app-script.html
- https://doc.qt.io/qt-6.12/supported-platforms.html
- https://docs.appimage.org/reference/best-practices.html
- https://github.com/linuxdeploy/linuxdeploy-plugin-qt
