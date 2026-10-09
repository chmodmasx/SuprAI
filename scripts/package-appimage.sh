#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${SUPRAI_BUILD_DIR:-${ROOT}/build}"
DIST_DIR="${SUPRAI_DIST_DIR:-${ROOT}/dist}"
VERSION="${SUPRAI_APPIMAGE_VERSION:-0.1.0-alpha.1}"
APPDIR="${ROOT}/build-appimage/AppDir"
NAME="SuprAI-${VERSION}-x86_64.AppImage"
TOOL_VERSION="1.9.1"
TOOL_SHA256="ed4ce84f0d9caff66f50bcca6ff6f35aae54ce8135408b3fa33abfc3cb384eb0"
TOOL="${ROOT}/build-appimage/appimagetool-${TOOL_VERSION}-x86_64.AppImage"

if [[ ! -f "${BUILD_DIR}/cmake_install.cmake" ]]; then
  echo "No existe un build CMake en ${BUILD_DIR}." >&2
  exit 2
fi

mkdir -p "${ROOT}/build-appimage" "${DIST_DIR}"
rm -rf -- "${APPDIR}"
mkdir -p "${APPDIR}/usr"
cmake --install "${BUILD_DIR}" --prefix "${APPDIR}/usr"

test -x "${APPDIR}/usr/bin/suprai"
test -f "${APPDIR}/usr/bin/qt.conf"
test -f "${APPDIR}/usr/plugins/platforms/libqxcb.so"
test -f "${APPDIR}/usr/plugins/platforms/libqwayland.so"
test -f "${APPDIR}/usr/plugins/sqldrivers/libqsqlite.so"

cp "${ROOT}/packaging/org.supralinux.SuprAI.desktop" "${APPDIR}/"
cp "${ROOT}/packaging/suprai.svg" "${APPDIR}/"
cp "${ROOT}/packaging/AppRun" "${APPDIR}/AppRun"
chmod +x "${APPDIR}/AppRun"
ln -s suprai.svg "${APPDIR}/.DirIcon"

if command -v desktop-file-validate >/dev/null 2>&1; then
  desktop-file-validate "${APPDIR}/org.supralinux.SuprAI.desktop"
fi

if [[ ! -f "${TOOL}" ]] || ! printf '%s  %s\n' "${TOOL_SHA256}" "${TOOL}" | sha256sum -c --status; then
  curl --fail --location --retry 3 \
    "https://github.com/AppImage/appimagetool/releases/download/${TOOL_VERSION}/appimagetool-x86_64.AppImage" \
    --output "${TOOL}"
fi
printf '%s  %s\n' "${TOOL_SHA256}" "${TOOL}" | sha256sum -c
chmod +x "${TOOL}"

ARCH=x86_64 VERSION="${VERSION}" APPIMAGE_EXTRACT_AND_RUN=1 \
  "${TOOL}" --no-appstream "${APPDIR}" "${DIST_DIR}/${NAME}"

test -s "${DIST_DIR}/${NAME}"
chmod +x "${DIST_DIR}/${NAME}"
(
  cd "${DIST_DIR}"
  sha256sum "${NAME}" > "${NAME}.sha256"
)
printf 'APPIMAGE=%s\n' "${DIST_DIR}/${NAME}"
