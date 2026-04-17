#!/usr/bin/env bash
set -eu

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TMP_DIR="$ROOT_DIR/tmp/automation/building"
GDIS_EXEC="$ROOT_DIR/bin/gdis-gtk4"

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

run_case() {
  local case_name="$1"
  local action="$2"
  local title_substring="$3"
  local required_patterns="$4"
  shift 4

  local log_file="$TMP_DIR/${case_name}.log"
  local png_file="$TMP_DIR/${case_name}.png"

  env GDK_BACKEND=x11 NO_AT_BRIDGE=0 GDIS_DEBUG_OPEN_DIALOG="$action" \
    "$GDIS_EXEC" "$@" >"$log_file" 2>&1 &
  local pid=$!

  cleanup() {
    kill "$pid" >/dev/null 2>&1 || true
    wait "$pid" >/dev/null 2>&1 || true
  }
  trap cleanup RETURN

  local window_id=""
  for _ in $(seq 1 20); do
    window_id="$(wmctrl -lp 2>/dev/null | awk -v pid="$pid" -v title="$title_substring" '
      $3 == pid {
        idx = index($0, $5);
        name = idx ? substr($0, idx) : "";
        if (index(name, title)) {
          print $1;
          exit;
        }
      }')"
    if [ -n "$window_id" ]; then
      break
    fi
    sleep 1
  done

  if [ -z "$window_id" ]; then
    echo "Failed to find GTK4 Building window for case: $case_name" >&2
    echo "Expected title substring: $title_substring" >&2
    echo "See: $log_file" >&2
    return 1
  fi

  sleep 2
  import -window "$window_id" "png:$png_file"

  python3 - "$case_name" "$title_substring" "$required_patterns" "$png_file" <<'PY'
import sys
import time

from PIL import Image
import pyatspi

case_name = sys.argv[1]
title_substring = sys.argv[2]
required_patterns = [p for p in sys.argv[3].split("||") if p]
png_file = sys.argv[4]

window = None
end = time.time() + 10
while time.time() < end:
    desktop = pyatspi.Registry.getDesktop(0)
    matches = []
    for app in desktop:
        for child in app:
            try:
                name = child.name or ""
                role = child.getRoleName()
            except Exception:
                continue
            if title_substring not in name:
                continue
            if role not in ("dialog", "frame"):
                continue
            matches.append(child)
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
    print(f"AT-SPI could not find window for case: {case_name}", file=sys.stderr)
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

missing = []
for pattern in required_patterns:
    if not any(pattern in name for name in seen):
        missing.append(pattern)

if missing:
    print(f"Missing Building UI labels for {case_name}: {', '.join(missing)}", file=sys.stderr)
    sys.exit(1)

image = Image.open(png_file)
if image.size[0] < 300 or image.size[1] < 200:
    print(f"Unexpected screenshot size for {case_name}: {image.size}", file=sys.stderr)
    sys.exit(1)

print(f"{case_name}_window_found=yes")
print(f"{case_name}_screenshot={png_file}")
print(f"{case_name}_screenshot_size={image.size[0]}x{image.size[1]}")
print(f"{case_name}_required_labels=ok")
PY

  if grep -E '(Gtk-CRITICAL|GLib-GObject-CRITICAL|assertion .* failed|Segmentation fault|Aborted)' \
      "$log_file" >/dev/null 2>&1; then
    echo "GTK/runtime warnings found for case: $case_name" >&2
    grep -E '(Gtk-CRITICAL|GLib-GObject-CRITICAL|assertion .* failed|Segmentation fault|Aborted)' \
      "$log_file" >&2 || true
    return 1
  fi

  trap - RETURN
  cleanup
}

run_case \
  editing \
  editing \
  "Model editing" \
  "Builder||Spatials||Transformations||Regions||Labelling||Library||Add atoms||Make supercell||Create nanotube" \
  examples/methane.gin

run_case \
  zmatrix \
  zmatrix \
  "ZMATRIX editor" \
  "Distance units||Angle units" \
  examples/methane.gin

run_case \
  dynamics \
  dynamics \
  "MD initializer" \
  "" \
  examples/methane.gin \
  examples/water.car

run_case \
  surfaces \
  surfaces \
  "surfaces" \
  "Miller||Shift||Depths||Create surface||Converge regions||Calculate energy||Calculation setup" \
  examples/adp1.cif

run_case \
  dislocations \
  dislocation \
  "Dislocation builder" \
  "Orientation vector||Burgers vector||Defect origin||Defect center||Region sizes||Charge neutralization" \
  examples/adp1.cif

run_case \
  docking \
  docking \
  "Docking setup" \
  "Translational sampling||Rotational sampling||Treat as a rigid body||axes fractions||grid size" \
  examples/calc2d.xtl

echo "GTK4 Building tools verification passed."
