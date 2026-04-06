#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
GTK_TARGET="${GDIS_GTK_TARGET:-gtk4}"
BUILD_DIR=""
WITH_GUI="true"
WITH_GRISU="false"
SETUP_ONLY=0
INSTALL_AFTER_BUILD=0
WIPE_BUILD=0

usage() {
  cat <<'EOF'
Usage:
  ./rebuild-meson.sh
  ./rebuild-meson.sh --gtk2
  ./rebuild-meson.sh --gtk4
  ./rebuild-meson.sh --builddir .build/custom
  ./rebuild-meson.sh --no-gui
  ./rebuild-meson.sh --grisu
  ./rebuild-meson.sh --setup-only
  ./rebuild-meson.sh --install
  ./rebuild-meson.sh --wipe

Notes:
  - This is a Meson/Ninja-based alternative to the legacy Perl + make build.
  - GTK4 is the default target unless you pass `--gtk2`.
  - The built executable is staged back into `bin/gdis` and `bin/gdis-<target>`
    so the existing launcher scripts keep working.
  - Runtime data files remain in `bin/`, matching how GDIS locates them.
EOF
}

while [ $# -gt 0 ]; do
  case "$1" in
    --help|-h)
      usage
      exit 0
      ;;
    --gtk2|--gtk4)
      GTK_TARGET="${1#--}"
      shift
      ;;
    --builddir)
      BUILD_DIR="${2:?missing build directory}"
      shift 2
      ;;
    --no-gui)
      WITH_GUI="false"
      shift
      ;;
    --grisu)
      WITH_GRISU="true"
      shift
      ;;
    --setup-only)
      SETUP_ONLY=1
      shift
      ;;
    --install)
      INSTALL_AFTER_BUILD=1
      shift
      ;;
    --wipe)
      WIPE_BUILD=1
      shift
      ;;
    *)
      echo "Unknown option: $1" >&2
      echo >&2
      usage >&2
      exit 1
      ;;
  esac
done

if [ "$GTK_TARGET" != "gtk2" ] && [ "$GTK_TARGET" != "gtk4" ]; then
  echo "Unsupported GDIS_GTK_TARGET: $GTK_TARGET" >&2
  echo "Supported values: gtk2, gtk4" >&2
  exit 1
fi

if [ -z "$BUILD_DIR" ]; then
  BUILD_DIR="$ROOT_DIR/.build/meson-$GTK_TARGET"
fi

LOCAL_MESON_PY="$ROOT_DIR/.cache/tools/meson/meson.py"
LOCAL_NINJA="$ROOT_DIR/.cache/tools/ninja/ninja"

if [ -n "${MESON:-}" ]; then
  MESON_CMD=("$MESON")
elif command -v meson >/dev/null 2>&1; then
  MESON_CMD=("meson")
elif [ -f "$LOCAL_MESON_PY" ]; then
  MESON_CMD=("python3" "$LOCAL_MESON_PY")
else
  echo "Missing build tool: meson" >&2
  echo "Install it with: sudo apt-get update && sudo apt-get install -y meson ninja-build" >&2
  echo "Or bootstrap local tools into .cache/tools/ and rerun this script." >&2
  exit 1
fi

if command -v ninja >/dev/null 2>&1; then
  :
elif command -v ninja-build >/dev/null 2>&1; then
  :
elif [ -x "$LOCAL_NINJA" ]; then
  PATH="$(dirname "$LOCAL_NINJA"):$PATH"
  export PATH
else
  echo "Missing build tool: ninja" >&2
  echo "Install it with: sudo apt-get update && sudo apt-get install -y meson ninja-build" >&2
  echo "Or bootstrap local tools into .cache/tools/ and rerun this script." >&2
  exit 1
fi

if [ "$WIPE_BUILD" -eq 1 ] && [ -d "$BUILD_DIR" ]; then
  rm -rf "$BUILD_DIR"
fi

mkdir -p "$(dirname "$BUILD_DIR")" "$ROOT_DIR/bin"

setup_args=(
  setup
  "$BUILD_DIR"
  "$ROOT_DIR"
  "-Dgtk_target=$GTK_TARGET"
  "-Dgui=$WITH_GUI"
  "-Dgrisu=$WITH_GRISU"
)

if [ -d "$BUILD_DIR" ]; then
  setup_args+=(--reconfigure)
fi

"${MESON_CMD[@]}" "${setup_args[@]}"

if [ "$SETUP_ONLY" -eq 1 ]; then
  echo "Meson setup complete: $BUILD_DIR"
  exit 0
fi

"${MESON_CMD[@]}" compile -C "$BUILD_DIR"

BUILT_EXE="$BUILD_DIR/src/gdis"
if [ ! -x "$BUILT_EXE" ]; then
  echo "Expected built executable not found: $BUILT_EXE" >&2
  exit 1
fi

install -m 755 "$BUILT_EXE" "$ROOT_DIR/bin/gdis"
install -m 755 "$BUILT_EXE" "$ROOT_DIR/bin/gdis-$GTK_TARGET"

echo "Meson build successful."
echo "GTK target: $GTK_TARGET"
echo "Build directory: $BUILD_DIR"
echo "Staged executable: $ROOT_DIR/bin/gdis-$GTK_TARGET"

if [ "$INSTALL_AFTER_BUILD" -eq 1 ]; then
  "${MESON_CMD[@]}" install -C "$BUILD_DIR"
fi
