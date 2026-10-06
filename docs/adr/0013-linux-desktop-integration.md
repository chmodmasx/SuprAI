# ADR-0013: Freedesktop-first Linux desktop integration

Status: accepted
Date: 2026-10-06

## Decision

Linux integration is implemented through stable freedesktop/Qt facilities and capability probes.

Do not build the primary path around X11-only hooks or one desktop environment.

## Application identity / activation

Use a stable reverse-DNS application ID before first packaged release.

Candidate: `org.supralinux.SuprAI`.

The final ID remains to be confirmed before shipping.

Implement `org.freedesktop.Application` semantics through QtDBus for:
- Activate;
- Open;
- ActivateAction;
- forwarding secondary launches to the primary process;
- deep-link/file activation.

The desktop entry keeps an `Exec=` fallback even if DBus activation is enabled.

This provides a Linux-native single-instance/activation foundation without pulling a KDE Framework solely for lifecycle.

## Global shortcuts

Primary path:
- XDG Desktop Portal `org.freedesktop.portal.GlobalShortcuts` version 2 via QtDBus.

Requirements:
- create a portal session;
- let the portal/compositor own binding UX;
- react to Activated/Deactivated;
- consume activation tokens when needed;
- probe interface/version.

Do not implement raw X11 key grabs as the Wayland design.

A later X11 fallback may exist only behind the same capability interface.

If no supported mechanism exists, the feature is unavailable rather than insecurely emulated.

## System tray

Use C++ `QSystemTrayIcon` behind `TrayService`.

This requires Qt::Widgets and `QApplication`, while the visible UI remains Qt Quick/QML.

Reasons:
- Qt 6.12 supports Linux StatusNotifierItem desktops;
- X11 XEmbed fallback exists;
- `isSystemTrayAvailable()` is explicit;
- avoids depending on Qt Labs API compatibility.

Tray is optional. The application must remain fully usable without it.

Do not assume every activation reason/message feature works identically on GNOME/KDE/X11.

## Notifications

`NotificationService` has capability-based backends.

Preferred:
- XDG Desktop Portal Notification v2 when available.

Fallback:
- `org.freedesktop.Notifications` 1.3 via D-Bus.

Rules:
- actions are optional capabilities;
- notification delivery is best-effort;
- app correctness never depends on a popup being shown;
- use XDG activation tokens when returning focus on Wayland;
- persistent notification IDs are SuprAI-owned and scoped.

Do not use tray balloon messages as the primary notification contract.

## Files, URIs, screenshots

Prefer standard portals for operations where Wayland/sandbox/user-consent semantics matter:
- FileChooser;
- OpenURI/OpenFile;
- Screenshot.

Use native Qt APIs when they already route through the correct desktop backend and preserve equivalent semantics.

All external link opens require explicit user action/policy.

## Capability surface

C++ exports one inspected capability model, for example:

```text
DesktopCapabilities
  tray
  notifications
  notificationActions
  globalShortcuts
  fileChooserPortal
  screenshotPortal
  dbusActivation
  secretStore
```

QML consumes capabilities; it does not guess the desktop from environment names.

## Why

Wayland intentionally removes several ambient X11 powers. Portals and D-Bus provide compositor/user-mediated equivalents without making GNOME/KDE-specific assumptions.
