#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CACHE_DIR="$ROOT_DIR/.cache/gtk3-debs"
SYSROOT_DIR="$ROOT_DIR/.localdeps/gtk3"
MULTIARCH="$(dpkg-architecture -qDEB_HOST_MULTIARCH 2>/dev/null || gcc -dumpmachine)"
LIB_DIR="$SYSROOT_DIR/usr/lib/$MULTIARCH"
PKGCONFIG_DIR="$LIB_DIR/pkgconfig"

PACKAGES=(
  libgtk-3-dev
  libepoxy-dev
  libatk-bridge2.0-dev
  libatspi2.0-dev
  libdbus-1-dev
  libxtst-dev
)

mkdir -p "$CACHE_DIR"
rm -rf "$SYSROOT_DIR"
mkdir -p "$SYSROOT_DIR"

(
  cd "$CACHE_DIR"
  apt-get download "${PACKAGES[@]}"
)

for deb in "$CACHE_DIR"/*.deb; do
  dpkg-deb -x "$deb" "$SYSROOT_DIR"
done

if [[ ! -d "$PKGCONFIG_DIR" ]]; then
  echo "Missing pkg-config directory: $PKGCONFIG_DIR" >&2
  exit 1
fi

for pc in "$PKGCONFIG_DIR"/*.pc; do
  perl -0pi -e \
    "s#^prefix=/usr\$#prefix=$SYSROOT_DIR/usr#m;
     s#^original_prefix=/usr\$#original_prefix=$SYSROOT_DIR/usr#m;
     s#^libdir=/usr/lib/$MULTIARCH\$#libdir=$SYSROOT_DIR/usr/lib/$MULTIARCH#m" \
    "$pc"
done

for link in "$LIB_DIR"/*.so; do
  target="$(readlink "$link" || true)"
  if [[ -z "$target" || -e "$LIB_DIR/$target" ]]; then
    continue
  fi

  system_target="$(find "/lib/$MULTIARCH" "/usr/lib/$MULTIARCH" \
    -maxdepth 1 -name "$target" -print -quit 2>/dev/null || true)"

  if [[ -n "$system_target" ]]; then
    ln -sfn "$system_target" "$LIB_DIR/$target"
  fi
done

cat <<EOF
Local GTK3 sysroot prepared under:
  $SYSROOT_DIR

Use it for GTK3 builds with:
  PKG_CONFIG_PATH=$PKGCONFIG_DIR GDIS_GTK_TARGET=gtk3 ./rebuild-ubuntu.sh

Or use the wrapper:
  ./build-gtk3-local.sh
EOF
