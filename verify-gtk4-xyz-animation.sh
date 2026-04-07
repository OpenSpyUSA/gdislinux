#!/usr/bin/env bash
set -eu

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TMP_DIR="$ROOT_DIR/tmp/automation"
GDIS_EXEC="$ROOT_DIR/bin/gdis-gtk4"
MODEL_A="${1:-$ROOT_DIR/tmp/qbox-gui/waterii.xyz}"
MODEL_B="${2:-$ROOT_DIR/tmp/qbox-gui/waterkk.xyz}"

need_tool() {
  if ! command -v "$1" >/dev/null 2>&1; then
    echo "Missing required tool: $1" >&2
    exit 1
  fi
}

check_python_module() {
  python3 - "$1" <<'PY'
import importlib
import sys

name = sys.argv[1]
importlib.import_module(name)
PY
}

capture_main_window() {
  local pid="$1"
  local output_png="$2"
  local window_id=""

  for _ in $(seq 1 30); do
    window_id="$(wmctrl -lpG | awk -v pid="$pid" '$3 == pid && /GTK Display Interface for Structures/ { print $1; exit }')"
    if [ -n "$window_id" ]; then
      break
    fi
    sleep 1
  done

  if [ -z "$window_id" ]; then
    echo "Failed to find the GTK4 main window for PID $pid." >&2
    return 1
  fi

  import -window "$window_id" "png:$output_png"
}

check_canvas_pixels() {
  python3 - "$1" <<'PY'
from PIL import Image
import sys

img = Image.open(sys.argv[1]).convert("RGB")
left = max(220, img.width // 6)
top = max(80, img.height // 12)
bottom = max(top + 1, img.height - 55)
crop = img.crop((left, top, img.width, bottom))
count = 0
for pixel in crop.getdata():
    if pixel != (0, 0, 0):
        count += 1
print(count)
PY
}

open_animation_from_menu() {
  python3 - <<'PY'
import pyatspi
import sys
import time

def walk(node):
    try:
        yield node
        for i in range(node.childCount):
            yield from walk(node.getChildAtIndex(i))
    except Exception:
        return

def find(node, pred):
    for child in walk(node):
        try:
            if pred(child):
                return child
        except Exception:
            pass
    return None

app = None
for _ in range(40):
    app = next((a for a in pyatspi.Registry.getDesktop(0)
                if a and a.name == "Unnamed"), None)
    if app:
        break
    time.sleep(0.25)

if app is None:
    sys.exit("No GDIS accessibility application was found.")

tools = find(app, lambda n: n.getRoleName() == "toggle button" and n.name == "Tools")
if tools is None:
    sys.exit("Could not find the Tools menu button.")

tools.queryAction().doAction(0)
time.sleep(0.8)

item = find(app, lambda n: n.getRoleName() == "push button" and n.name == "Visualization > Animation...")
if item is None:
    sys.exit("Could not find Tools > Visualization > Animation....")

action = item.queryAction()
if action.nActions < 1:
    sys.exit("Animation menu item has no accessibility action.")

action.doAction(0)
PY
}

wait_for_animation_window() {
  local pid="$1"
  local output_png="$2"
  local window_id=""

  for _ in $(seq 1 30); do
    window_id="$(wmctrl -lpG | awk -v pid="$pid" '$3 == pid && /Animation:/ { print $1; exit }')"
    if [ -n "$window_id" ]; then
      break
    fi
    sleep 1
  done

  if [ -z "$window_id" ]; then
    echo "Animation dialog did not map for PID $pid." >&2
    return 1
  fi

  import -window "$window_id" "png:$output_png"
}

run_case() {
  local model_path="$1"
  local stem
  local log_file
  local scene_png
  local anim_png
  stem="$(basename "$model_path")"
  stem="${stem%.*}"
  log_file="$TMP_DIR/${stem}-verify.log"
  scene_png="$TMP_DIR/${stem}-scene.png"
  anim_png="$TMP_DIR/${stem}-animation.png"

  (
    local pid
    local non_black

    killall -q gdis-gtk4 gdis gdis-gtk2 || true

    env GDK_BACKEND=x11 NO_AT_BRIDGE=0 "$GDIS_EXEC" "$model_path" >"$log_file" 2>&1 &
    pid=$!

    cleanup_case() {
      kill "$pid" >/dev/null 2>&1 || true
      wait "$pid" >/dev/null 2>&1 || true
    }
    trap cleanup_case EXIT

    capture_main_window "$pid" "$scene_png"
    non_black="$(check_canvas_pixels "$scene_png")"
    if [ "${non_black:-0}" -le 5000 ]; then
      echo "Rendered canvas for $model_path still looks blank ($non_black coloured pixels)." >&2
      exit 1
    fi

    open_animation_from_menu
    wait_for_animation_window "$pid" "$anim_png"

    if grep -q "Gtk-CRITICAL" "$log_file"; then
      echo "GTK reported a critical warning while verifying $model_path." >&2
      cat "$log_file" >&2
      exit 1
    fi

    printf 'verified=%s canvas_pixels=%s scene=%s animation=%s\n' \
      "$model_path" "$non_black" "$scene_png" "$anim_png"
  )
}

need_tool wmctrl
need_tool import
need_tool python3
check_python_module PIL
check_python_module pyatspi

if [ ! -x "$GDIS_EXEC" ]; then
  echo "Missing executable: $GDIS_EXEC" >&2
  echo "Build it first with: GDIS_GTK_TARGET=gtk4 ./rebuild-ubuntu.sh" >&2
  exit 1
fi

mkdir -p "$TMP_DIR"

run_case "$MODEL_A"
run_case "$MODEL_B"

killall -q gdis-gtk4 gdis gdis-gtk2 || true

echo "GTK4 XYZ + Animation verification passed."
