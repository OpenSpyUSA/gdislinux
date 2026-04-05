#!/usr/bin/env bash
set -eu

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TMP_DIR="$ROOT_DIR/tmp/automation"
GDIS_EXEC="$ROOT_DIR/bin/gdis-gtk4"
DIFF_MODEL="${1:-examples/adp1.cif}"
ISO_MODEL="${2:-examples/methane.gin}"

need_tool() {
  if ! command -v "$1" >/dev/null 2>&1; then
    echo "Missing required tool: $1" >&2
    exit 1
  fi
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

capture_now() {
  local output_png="$1"
  local model_path="$2"

  bash "$ROOT_DIR/capture-gdis-window.sh" --gtk4 \
    --output "$output_png" "$model_path"
}

capture_after_delay() {
  local log_file="$1"
  local output_png="$2"
  local model_path="$3"
  local delay_secs="$4"
  shift 4

  env GDK_BACKEND=x11 NO_AT_BRIDGE=0 "$@" "$GDIS_EXEC" "$model_path" \
    >"$log_file" 2>&1 &
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
    echo "Failed to find a GDIS window for PID $pid." >&2
    return 1
  fi

  sleep "$delay_secs"
  import -window "$window_id" "png:$output_png"
}

need_tool wmctrl
need_tool import
need_tool python3

if [ ! -x "$GDIS_EXEC" ]; then
  echo "Missing executable: $GDIS_EXEC" >&2
  exit 1
fi

mkdir -p "$TMP_DIR"

capture_now "$TMP_DIR/science-diff-baseline.png" "$DIFF_MODEL"
capture_after_delay \
  "$TMP_DIR/science-diff.log" \
  "$TMP_DIR/science-diff.png" \
  "$DIFF_MODEL" \
  6 \
  GDIS_DEBUG_OPEN_DIALOG=diffraction \
  GDIS_DEBUG_AUTO_EXECUTE_DIFFRACTION=1 \
  GDIS_DEBUG_GRAPH_STATE=1

capture_now "$TMP_DIR/science-iso-baseline.png" "$ISO_MODEL"
capture_after_delay \
  "$TMP_DIR/science-iso.log" \
  "$TMP_DIR/science-iso.png" \
  "$ISO_MODEL" \
  6 \
  GDIS_DEBUG_OPEN_DIALOG=isosurface \
  GDIS_DEBUG_AUTO_EXECUTE_ISOSURF=1

diff_pixels="$(pixel_diff_count "$TMP_DIR/science-diff-baseline.png" "$TMP_DIR/science-diff.png")"
iso_pixels="$(pixel_diff_count "$TMP_DIR/science-iso-baseline.png" "$TMP_DIR/science-iso.png")"

printf 'diffraction_diff_pixels=%s\n' "$diff_pixels"
printf 'isosurface_diff_pixels=%s\n' "$iso_pixels"

if [ "$diff_pixels" -le 0 ]; then
  echo "Diffraction produced no visible GTK4 change." >&2
  exit 1
fi

if [ "$iso_pixels" -le 0 ]; then
  echo "Iso-surface produced no visible GTK4 change." >&2
  exit 1
fi

if ! grep -q 'graph_active=0x' "$TMP_DIR/science-diff.log"; then
  echo "Diffraction did not activate a graph in the GTK4 run." >&2
  exit 1
fi

if grep -q 'GtkDialog mapped without a transient parent' "$TMP_DIR/science-diff.log" ||
   grep -q 'GtkDialog mapped without a transient parent' "$TMP_DIR/science-iso.log"; then
  echo "Dialog transient-parent warning is still present." >&2
  exit 1
fi

echo "GTK4 diffraction + iso-surface verification passed."
