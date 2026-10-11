# ADR-0016: Secure desktop credential storage via QtKeychain

Status: partially implemented / physical verification pending
Date: 2026-10-06
Updated: 2026-10-11

## Decision

Provider credentials must never be stored as plaintext in QSettings, SQLite,
logs or exported profiles. On supported Linux desktops use QtKeychain as the
initial credential-store adapter. Its Linux backends use KWallet or Secret
Service. The AppImage build stages the Qt6Keychain library and includes the
dependency's BSD license notice.

QtKeychain 0.14.2 (Ubuntu 24.04) is the initial CI packaging dependency. Do
not claim 0.17-specific APIs or functionality from this build.

## Implemented initial integration (alpha.3 candidate)

- Provider endpoint/model are separate non-secret QSettings values.
- Asynchronous credential read on startup precedes NativeSuprAIRuntime
  construction; QML may render while waiting for the desktop wallet.
- API key writes and deletion use asynchronous QtKeychain jobs.
- `setInsecureFallback(false)` is mandatory for every credential job.
- Credential material is never returned to QML; only a boolean stored-state
  and a short error/status message are shown.
- If the wallet fails or is absent, the UI reports that credentials were not
  loaded/saved and never writes a plaintext substitute.
- Updating endpoint metadata does not reuse a previously loaded bearer for a
  different target unless a replacement key is explicitly provided.
- Changes take effect only after restarting the application; the current
  runtime is not reconfigured during an in-progress Turn.
- Public endpoints require HTTPS in the configuration UI.

## Planned boundary

The current settings facade is one active endpoint, not yet a generic
SecretStore interface or multi-profile ProviderRegistry. As more secret
consumers arrive, extract a public SecretStore contract:

```text
SecretStore
  available()
  put(id, bytes)
  get(id)
  remove(id)

QtKeychainSecretStore
```

Stored profile entries will reference opaque IDs, never credential payloads.

## Remaining proof

- desktop KWallet on physical KDE (including locked/unlocked prompts);
- GNOME Secret Service;
- unavailable service and error messaging;
- credential deletion/replacement and reopening the AppImage;
- verify complete QtKeychain dynamic dependency closure in the AppImage;
- dependency and license audit across Qt and third parties.

The alpha.3 code is not production-verified until these tests pass.

## Sources

- https://github.com/frankosterfeld/qtkeychain
- https://packages.ubuntu.com/noble/qtkeychain-qt6-dev
- freedesktop Secret Service specification.
