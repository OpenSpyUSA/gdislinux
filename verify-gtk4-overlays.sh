#!/usr/bin/env bash
set -eu

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TMP_DIR="$ROOT_DIR/tmp/automation"
GDIS_EXEC="$ROOT_DIR/bin/gdis-gtk4"
MODEL_PATH="${1:-examples/methane.gin}"

need_tool() {
  if ! command -v "$1" >/dev/null 2>&1; then
    echo "Missing required tool: $1" >&2
    exit 1
  fi
}

run_capture() {
  local label="$1"
  shift

  env "$@" bash "$ROOT_DIR/capture-gdis-window.sh" --gtk4 \
    --output "$TMP_DIR/$label.png" "$MODEL_PATH"
}

pixel_diff_count() {
  python3 - "$1" "$2" <<'PY'
from PIL import Image
import sys

a = Image.open(sys.argv[1]).convert("RGBA")
b = Image.open(sys.argv[2]).convert("RGBA")
count = 0
for y in range(a.height):
    for x in range(a.width):
        if a.getpixel((x, y)) != b.getpixel((x, y)):
            count += 1
print(count)
PY
}

run_click_selection_check() {
  local before_png="$TMP_DIR/methane-select-before.png"
  local after_png="$TMP_DIR/methane-select-after.png"
  local log_file="$TMP_DIR/gdis-click-single.log"

  env GDK_BACKEND=x11 NO_AT_BRIDGE=0 GDIS_DEBUG_INPUT=1 \
    "$GDIS_EXEC" "$MODEL_PATH" >"$log_file" 2>&1 &
  local pid=$!

  cleanup() {
    kill "$pid" >/dev/null 2>&1 || true
    wait "$pid" >/dev/null 2>&1 || true
  }
  trap cleanup RETURN

  local window_id=""
  for _ in $(seq 1 20); do
    window_id="$(wmctrl -lpG | awk -v pid="$pid" '$3 == pid { print $1; exit }')"
    if [ -n "$window_id" ]; then
      break
    fi
    sleep 1
  done

  if [ -z "$window_id" ]; then
    echo "Failed to find GDIS window for click-selection check." >&2
    return 1
  fi

  eval "$(xwininfo -id "$window_id" | awk '
    /Absolute upper-left X:/ { print "WX=" $4 }
    /Absolute upper-left Y:/ { print "WY=" $4 }
  ')"

  import -window "$window_id" "png:$before_png"

  # Target derived from the methane example window capture under GTK4.
  xdotool mousemove --sync "$((WX + 836))" "$((WY + 170))" click 1
  sleep 1

  import -window "$window_id" "png:$after_png"

  local diff_count
  diff_count="$(pixel_diff_count "$before_png" "$after_png")"

  printf 'click_selection_diff_pixels=%s\n' "$diff_count"
  tail -n 20 "$log_file"

  if ! grep -q 'core=C' "$log_file"; then
    echo "Selection click did not hit the methane carbon." >&2
    return 1
  fi

  if ! grep -q 'selected=1' "$log_file"; then
    echo "Selection click did not leave one atom selected." >&2
    return 1
  fi

  if [ "$diff_count" -le 0 ]; then
    echo "Selection click produced no visible image delta." >&2
    return 1
  fi
}

need_tool wmctrl
need_tool xwininfo
need_tool import
need_tool xdotool
need_tool python3

if [ ! -x "$GDIS_EXEC" ]; then
  echo "Missing executable: $GDIS_EXEC" >&2
  exit 1
fi

mkdir -p "$TMP_DIR"

run_capture methane-baseline
run_capture methane-overlay GDIS_DEBUG_VISUAL_OVERLAYS=1 GDIS_DEBUG_GL_VERBOSE=1

overlay_diff="$(pixel_diff_count "$TMP_DIR/methane-baseline.png" "$TMP_DIR/methane-overlay.png")"

printf 'overlay_diff_pixels=%s\n' "$overlay_diff"

if [ "$overlay_diff" -le 0 ]; then
  echo "No visible delta for debug selection/measurement overlays." >&2
  exit 1
fi

if grep -q 'overlay tex upload' "$TMP_DIR/gdis-capture.log"; then
  if grep -q 'GL error 0x502' "$TMP_DIR/gdis-capture.log"; then
    echo "Overlay GL error still present in capture log." >&2
    exit 1
  fi
fi

run_click_selection_check

echo "GTK4 overlay verification passed."
