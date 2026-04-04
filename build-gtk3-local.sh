#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MULTIARCH="$(dpkg-architecture -qDEB_HOST_MULTIARCH 2>/dev/null || gcc -dumpmachine)"
PKGCONFIG_DIR="$ROOT_DIR/.localdeps/gtk3/usr/lib/$MULTIARCH/pkgconfig"

"$ROOT_DIR/bootstrap-gtk3-local.sh"

export PKG_CONFIG_PATH="$PKGCONFIG_DIR${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
export GDIS_GTK_TARGET=gtk3

"$ROOT_DIR/rebuild-ubuntu.sh"

if [[ -x "$ROOT_DIR/bin/gdis-gtk2" ]]; then
  cp -f "$ROOT_DIR/bin/gdis-gtk2" "$ROOT_DIR/bin/gdis"
  echo "Restored default executable to bin/gdis-gtk2 for day-to-day use."
else
  echo "GTK3 build completed. bin/gdis currently points to the GTK3 build."
fi
