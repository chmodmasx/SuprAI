# ADR-0010: Stage AppImage with Qt CMake deployment APIs

Status: proposed
Date: 2026-10-06  
Updated: 2026-10-07

## Proposed decision

Make the CMake install tree / AppDir the source of truth.

The staging part of this pipeline is now implemented and CI-verified:

```text
CMake build
  -> cmake --install into portable stage
  -> qt_deploy_qml_imports
  -> qt_deploy_runtime_dependencies
  -> explicit Qt6Widgets runtime artifact
  -> explicit QWaylandIntegrationPlugin runtime artifact
  -> qt.conf
  -> XCB staged smoke under Xvfb
  -> Wayland staged smoke under Weston headless

Still pending:
  -> desktop metadata/AppRun
  -> AppImage finalizer
  -> ABI-floor/clean-VM smoke tests
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

## Implemented partial proof

The CMake-owned portable staging mechanism is now implemented.

Verified in GitHub Actions with Qt 6.12:
- `cmake --install` creates a standalone staging tree;
- `qt_deploy_qml_imports()` deploys QML imports;
- `qt_deploy_runtime_dependencies()` deploys runtime closure and `qt.conf`;
- Qt Widgets is staged explicitly as an imported runtime artifact because the Linux dependency scan omitted that deliberate `QApplication` dependency in the proof environment;
- `Qt6::QWaylandIntegrationPlugin` is staged explicitly and its runtime dependencies are included;
- application plugin policy explicitly includes `QWaylandIntegrationPlugin` and `QXcbIntegrationPlugin`;
- staged tree contains both `libqwayland.so` and `libqxcb.so`;
- staged application launches with Qt build/install environment variables removed under XCB via Xvfb;
- staged application launches under Wayland via a headless Weston compositor;
- runtime and persistence worker shutdown complete without timeout.

This is a staging proof, not an AppImage/ABI-floor proof. Therefore this ADR remains proposed.

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


## Verified staging proof

Verified on GitHub Actions Ubuntu 24.04 with Qt 6.12:
- normal build succeeds;
- 7/7 tests pass;
- regular QML startup smoke succeeds;
- install-stage creation succeeds;
- stage contains qt.conf;
- stage contains XCB and Wayland QPA plugins;
- stage launches under XCB/Xvfb with LD_LIBRARY_PATH, QT_PLUGIN_PATH, QML2_IMPORT_PATH and QT_ROOT_DIR removed;
- stage launches under Wayland/Weston headless under the same isolation;
- clean worker shutdown is required by CI.

Implementation detail:
Qt 6.12 deployment did not automatically include the deliberate QApplication/Qt6Widgets dependency nor the Wayland QPA plugin in this configuration. SuprAI therefore installs the official imported runtime artifacts explicitly through CMake and still uses Qt deployment APIs for dependency/QML closure. This is preferable to hard-coded filesystem copies.

This proof does not establish the final AppImage ABI floor or support for Ubuntu 22.04/Debian 12. Those remain release-gate work.
