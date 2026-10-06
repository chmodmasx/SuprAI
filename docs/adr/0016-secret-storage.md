# ADR-0016: QtKeychain as SecretStore implementation candidate

Status: proposed
Date: 2026-10-06

## Goal

Store provider API keys, remote tokens and other credentials without plaintext application config.

## Candidate

QtKeychain 0.17.x.

Current properties:
- Qt 6 is the default build path;
- Linux supports libsecret/GNOME Keyring and KWallet fallback;
- exposes secure-backend availability;
- Modified BSD license;
- unsupported environments return an error;
- unencrypted fallback is not used unless explicitly requested.

## Proposed design

`SecretStore` is a SuprAI interface.

```text
SecretStore
  available()
  put(id, bytes)
  get(id)
  remove(id)

QtKeychainSecretStore
```

SQLite/config stores only opaque secret IDs and non-secret metadata.

Never call `setInsecureFallback(true)` in production code.

If no secure store is available:
- session-only secret entry may be allowed in memory;
- persistent credential storage is disabled;
- UI explains the limitation;
- never silently write plaintext.

## Lookup metadata

Secret Service attributes/QtKeychain service/key names are not secret material. Do not put API keys or sensitive payloads in labels/attributes.

Use stable namespaced identifiers.

## Required proof before acceptance

- build 0.17.x with selected Qt 6.12 toolchain;
- test KDE/KWallet;
- test GNOME/libsecret;
- test locked wallet/keyring prompt;
- test unavailable service;
- inspect AppImage dependency closure;
- verify no insecure fallback;
- verify deletion and replacement semantics.

## Sources

- https://github.com/frankosterfeld/qtkeychain
- freedesktop Secret Service specification.
