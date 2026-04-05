#!/usr/bin/env bash
set -eu

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

usage() {
  cat <<'EOF'
Usage:
  ./capture-gdis-window.sh --gtk4 examples/methane.gin
  ./capture-gdis-window.sh --gtk4 models/qbox_methane.qbox
  ./capture-gdis-window.sh --gtk4 --output tmp/automation/methane.png examples/methane.gin
  ./capture-gdis-window.sh --gtk4 --keep-open examples/methane.gin

What it does:
  1. launches GDIS under X11/Xwayland for reliable shell-side automation
  2. finds the GDIS window by the launched process PID
  3. captures a PNG screenshot of that window with ImageMagick `import`
  4. optionally leaves the window open for manual inspection
EOF
}

resolve_gdis_executable() {
  local gtk_target="${1:-}"
  local gdis_exec

  if [ -n "$gtk_target" ]; then
    gdis_exec="$ROOT_DIR/bin/gdis-$gtk_target"
    if [ ! -x "$gdis_exec" ]; then
      echo "Missing executable: $gdis_exec" >&2
      echo "Build it first with: GDIS_GTK_TARGET=$gtk_target ./rebuild-ubuntu.sh" >&2
      exit 1
    fi
    printf '%s\n' "$gdis_exec"
    return
  fi

  if [ -x "$ROOT_DIR/bin/gdis-gtk2" ]; then
    printf '%s\n' "$ROOT_DIR/bin/gdis-gtk2"
    return
  fi

  if [ -x "$ROOT_DIR/bin/gdis" ]; then
    printf '%s\n' "$ROOT_DIR/bin/gdis"
    return
  fi

  echo "Missing executable under $ROOT_DIR/bin" >&2
  exit 1
}

need_tool() {
  if ! command -v "$1" >/dev/null 2>&1; then
    echo "Missing required tool: $1" >&2
    exit 1
  fi
}

GTK_TARGET="${GDIS_GTK_TARGET:-}"
KEEP_OPEN=0
WAIT_SECS=15
OUTPUT_FILE=""

while [ $# -gt 0 ]; do
  case "$1" in
    --help|-h)
      usage
      exit 0
      ;;
    --gtk2|--gtk3|--gtk4)
      GTK_TARGET="${1#--}"
      shift
      ;;
    --keep-open)
      KEEP_OPEN=1
      shift
      ;;
    --wait)
      WAIT_SECS="${2:-}"
      shift 2
      ;;
    --output)
      OUTPUT_FILE="${2:-}"
      shift 2
      ;;
    --)
      shift
      break
      ;;
    -*)
      echo "Unknown option: $1" >&2
      usage >&2
      exit 1
      ;;
    *)
      break
      ;;
  esac
done

if [ $# -lt 1 ]; then
  usage >&2
  exit 1
fi

need_tool wmctrl
need_tool import

GDIS_EXEC="$(resolve_gdis_executable "$GTK_TARGET")"

mkdir -p "$ROOT_DIR/tmp/automation"

if [ -z "$OUTPUT_FILE" ]; then
  stem="$(basename "$1")"
  stem="${stem%.*}"
  target="${GTK_TARGET:-default}"
  OUTPUT_FILE="$ROOT_DIR/tmp/automation/${stem}-${target}.png"
elif [ "${OUTPUT_FILE#/}" = "$OUTPUT_FILE" ]; then
  OUTPUT_FILE="$ROOT_DIR/$OUTPUT_FILE"
fi

mkdir -p "$(dirname "$OUTPUT_FILE")"

env GDK_BACKEND=x11 NO_AT_BRIDGE=0 "$GDIS_EXEC" "$@" \
  >"$ROOT_DIR/tmp/automation/gdis-capture.log" 2>&1 &
pid=$!

cleanup() {
  if [ "$KEEP_OPEN" -eq 0 ]; then
    kill "$pid" >/dev/null 2>&1 || true
    wait "$pid" >/dev/null 2>&1 || true
  fi
}
trap cleanup EXIT

window_id=""
for _ in $(seq 1 "$WAIT_SECS"); do
  window_id="$(wmctrl -lpG | awk -v pid="$pid" '$3 == pid { print $1; exit }')"
  if [ -n "$window_id" ]; then
    break
  fi
  sleep 1
done

if [ -z "$window_id" ]; then
  echo "Failed to find a GDIS window for PID $pid." >&2
  echo "See $ROOT_DIR/tmp/automation/gdis-capture.log" >&2
  exit 1
fi

import -window "$window_id" "png:$OUTPUT_FILE"

printf 'pid=%s\n' "$pid"
printf 'window_id=%s\n' "$window_id"
printf 'screenshot=%s\n' "$OUTPUT_FILE"

if [ "$KEEP_OPEN" -eq 1 ]; then
  trap - EXIT
fi
