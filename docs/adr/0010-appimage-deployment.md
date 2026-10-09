# ADR-0010: Qt-managed AppDir + appimagetool for AppImage

Status: proposed (preview packaging implemented; release compatibility gates open)
Date: 2026-10-06
Updated: 2026-10-08

## Current decision

CMake's Qt deployment APIs own the dependency closure. The finalizer consumes that staged AppDir; it must not re-resolve Qt dependencies through an unrelated plugin.

```text
CMake / Qt 6.12 build
  -> CTest
  -> cmake --install into build-appimage/AppDir/usr
  -> Qt-managed QML imports, libraries, plugins, qt.conf
  -> AppDir root: executable AppRun + desktop entry + icon
  -> pinned appimagetool 1.9.1, SHA-256 checked
  -> SuprAI-0.1.0-alpha.1-x86_64.AppImage + SHA256
  -> AppImage extract-and-run smoke under XCB/Xvfb
  -> AppImage extract-and-run smoke under Wayland/Weston
  -> CI artifact
  -> main-only GitHub pre-release after all tests pass
```

Implemented in `scripts/package-appimage.sh`, `packaging/` and `.github/workflows/appimage.yml`. The first AppImage preview build passed on the PR head `a580d4a164be9fb451d45ae4ea7a6555fe6f9f71`, including its packaged desktop smoke tests. The main-branch pre-release pipeline is separate and must pass before the release asset is treated as published.

## Architecture boundaries

- No Qt/SQLite/provider object is handed to QML for packaging purposes.
- The AppRun wrapper only establishes the bundled library search path and executes `usr/bin/suprai`.
- QML imports and Qt platform/plugin locations are deployed by Qt's CMake APIs.
- Explicitly stage Qt Widgets, SQLite's QSQLITE plugin, and Wayland/XCB QPA plugins; the previous CI proof showed these are not all discovered automatically.
- Never ship a system's graphics kernel drivers inside the AppImage.
- The application keeps its host-owned secrets, networking and XDG user state.

## Compatibility and security limits

The preview uses GitHub Actions Ubuntu 24.04 with Qt 6.12.0, so an Ubuntu 22.04 or Debian 12 compatible GLIBC/GLIBCXX floor must **not** be claimed. FUSE may be absent on some hosts; extract-and-run is supported for testing but carries a startup overhead.

Still required for release-grade acceptance:
- determine actual GLIBC/GLIBCXX symbol floor from shipped binaries and libraries;
- clean-host Ubuntu 22.04, 24.04, 26.04 and Debian 12 runs;
- physical KDE/GNOME Wayland and X11 compatibility, including the local NVIDIA host;
- verify real NInfer Chat Completions behavior and SQLite recovery;
- audit dynamically bundled Qt and third-party modules, include applicable LGPL and other license notices, and document corresponding sources/relinking obligations;
- publish project license/distribution notices.

No production cross-distro guarantee or completed legal-distribution audit is claimed for the alpha preview.

## Sources

- https://doc.qt.io/qt-6/linux-deployment.html
- https://doc.qt.io/qt-6/qt-generate-deploy-qml-app-script.html
- https://docs.appimage.org/reference/appdir.html
- https://docs.appimage.org/reference/architecture.html
- https://github.com/AppImage/appimagetool/releases/tag/1.9.1
