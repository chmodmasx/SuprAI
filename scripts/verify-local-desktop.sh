#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${SUPRAI_BUILD_DIR:-$ROOT_DIR/build-local-verify}"
STAGE_DIR="${SUPRAI_STAGE_DIR:-$ROOT_DIR/stage-local-verify}"

cd "$ROOT_DIR"

cmake -S . -B "$BUILD_DIR" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DSUPRAI_BUILD_TESTS=ON

cmake --build "$BUILD_DIR" --parallel
ctest --test-dir "$BUILD_DIR" --output-on-failure

rm -rf "$STAGE_DIR"
cmake --install "$BUILD_DIR" --prefix "$STAGE_DIR"

test -x "$STAGE_DIR/bin/suprai"
test -f "$STAGE_DIR/bin/qt.conf"
test -f "$STAGE_DIR/plugins/platforms/libqxcb.so"
test -f "$STAGE_DIR/plugins/platforms/libqwayland.so"

run_staged() {
  local platform="$1"

  env -u LD_LIBRARY_PATH \
      -u QT_PLUGIN_PATH \
      -u QML2_IMPORT_PATH \
      -u QT_ROOT_DIR \
      QT_QPA_PLATFORM="$platform" \
      QT_QUICK_BACKEND=software \
      SUPRAI_RUNTIME=mock \
      "$STAGE_DIR/bin/suprai" --smoke-test
}

tested=0

if [[ -n "${WAYLAND_DISPLAY:-}" ]]; then
  echo "==> Physical desktop smoke: Wayland"
  run_staged wayland
  echo "PASS: Wayland"
  tested=$((tested + 1))
else
  echo "SKIP: WAYLAND_DISPLAY is not set"
fi

if [[ -n "${DISPLAY:-}" ]]; then
  echo "==> Physical desktop smoke: XCB/X11"
  run_staged xcb
  echo "PASS: XCB/X11"
  tested=$((tested + 1))
else
  echo "SKIP: DISPLAY is not set"
fi

if [[ "$tested" -eq 0 ]]; then
  echo "ERROR: neither WAYLAND_DISPLAY nor DISPLAY is available." >&2
  exit 2
fi

echo "Desktop verification complete: $tested backend(s) passed."
