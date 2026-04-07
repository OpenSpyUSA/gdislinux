#!/usr/bin/env bash
set -eu

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TMP_DIR="$ROOT_DIR/tmp/automation"
GDIS_EXEC="$ROOT_DIR/bin/gdis-gtk4"
MODEL_PATH="${1:-examples/methane.gin}"
LOG_FILE="$TMP_DIR/qbox-ui-verify.log"
PNG_FILE="$TMP_DIR/qbox-ui-verify.png"

need_tool() {
  if ! command -v "$1" >/dev/null 2>&1; then
    echo "Missing required tool: $1" >&2
    exit 1
  fi
}

need_tool wmctrl
need_tool import
need_tool python3

if [ ! -x "$GDIS_EXEC" ]; then
  echo "Missing executable: $GDIS_EXEC" >&2
  echo "Build it first with: GDIS_GTK_TARGET=gtk4 ./rebuild-ubuntu.sh" >&2
  exit 1
fi

mkdir -p "$TMP_DIR"

env GDK_BACKEND=x11 NO_AT_BRIDGE=0 GDIS_DEBUG_OPEN_DIALOG=qbox \
  GDIS_DEBUG_QBOX_EXPAND_ADVANCED=1 \
  "$GDIS_EXEC" "$MODEL_PATH" >"$LOG_FILE" 2>&1 &
pid=$!

cleanup() {
  kill "$pid" >/dev/null 2>&1 || true
  wait "$pid" >/dev/null 2>&1 || true
}
trap cleanup EXIT

window_id=""
for _ in $(seq 1 20); do
  window_id="$(wmctrl -lpG | awk -v pid="$pid" '$3 == pid && $9 == "Qbox" { print $1; exit }')"
  if [ -n "$window_id" ]; then
    break
  fi
  sleep 1
done

if [ -z "$window_id" ]; then
  echo "Failed to find the GTK4 Qbox window." >&2
  echo "See: $LOG_FILE" >&2
  exit 1
fi

sleep 2
import -window "$window_id" "png:$PNG_FILE"

python3 - "$ROOT_DIR" "$PNG_FILE" <<'PY'
import os
import sys
import time

from PIL import Image
import pyatspi

root_dir = sys.argv[1]
png_file = sys.argv[2]

required_names = [
    "Qbox",
    "Use MPI launcher",
    "Default run command",
    "Add set ecuts",
    "Append occupation",
    "Advanced: Command Assistants",
    "Advanced: Input Composition",
    "Advanced: Command Templates",
    "Advanced: Pre-default Commands",
    "Advanced: Post/default-override Commands",
]

window = None
end = time.time() + 10
while time.time() < end:
    desktop = pyatspi.Registry.getDesktop(0)
    matches = []
    for app in desktop:
        for child in app:
            try:
                if (child.name or "") != "Qbox":
                    continue
                role = child.getRoleName()
                if role not in ("dialog", "frame"):
                    continue
                matches.append(child)
            except Exception:
                pass
    if matches:
        def descendant_count(node):
            total = 1
            try:
                count = node.childCount
            except Exception:
                count = 0
            for i in range(count):
                try:
                    total += descendant_count(node.getChildAtIndex(i))
                except Exception:
                    pass
            return total

        window = max(matches, key=descendant_count)
        break
    time.sleep(0.2)

if not window:
    print("AT-SPI could not find the Qbox dialog.", file=sys.stderr)
    sys.exit(1)

seen = set()
stack = [window]
while stack:
    node = stack.pop()
    try:
        name = node.name or ""
    except Exception:
        name = ""
    if name:
        seen.add(name)
    try:
        for i in range(node.childCount):
            stack.append(node.getChildAtIndex(i))
    except Exception:
        pass

missing = [name for name in required_names if name not in seen]
if missing:
    print("Missing Qbox UI labels:", ", ".join(missing), file=sys.stderr)
    sys.exit(1)

image = Image.open(png_file)
if image.size[0] < 600 or image.size[1] < 400:
    print(f"Unexpected screenshot size: {image.size}", file=sys.stderr)
    sys.exit(1)

print(f"qbox_window_found=yes")
print(f"qbox_screenshot={png_file}")
print(f"qbox_screenshot_size={image.size[0]}x{image.size[1]}")
print("qbox_required_labels=ok")
PY

echo "GTK4 Qbox UI verification passed."
