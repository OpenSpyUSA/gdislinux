#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TMP_DIR="$ROOT_DIR/tmp/automation"
MODEL_PATH="models/methane.gin"
KEEP_OPEN=0
WAIT_SECS=15
TARGETS=()

usage() {
  cat <<'EOF'
Usage:
  ./verify-pick-alignment.sh
  ./verify-pick-alignment.sh --gtk4
  ./verify-pick-alignment.sh --gtk2 --keep-open
  ./verify-pick-alignment.sh --model models/methane.gin

What it does:
  1. launches GDIS under X11/XWayland with pick/input debug logging enabled
  2. captures a screenshot of the live window
  3. detects the main black OpenGL canvas from the screenshot
  4. detects the visible molecule inside that canvas
  5. clicks the molecule twice (current size and resized window)
  6. checks the debug log to confirm the click picked visible geometry

This is intended as a practical cursor/pick alignment regression check for
the GTK2 and GTK4 builds on Ubuntu desktops.
EOF
}

need_tool() {
  if ! command -v "$1" >/dev/null 2>&1; then
    echo "Missing required tool: $1" >&2
    exit 1
  fi
}

resolve_target_exec() {
  local target="$1"
  local exe="$ROOT_DIR/bin/gdis-$target"

  if [ ! -x "$exe" ]; then
    echo "Missing executable: $exe" >&2
    echo "Build it first with: GDIS_GTK_TARGET=$target ./rebuild-ubuntu.sh" >&2
    exit 1
  fi

  printf '%s\n' "$exe"
}

wait_for_window() {
  local pid="$1"
  local wid=""
  local i

  for i in $(seq 1 "$WAIT_SECS"); do
    wid="$(wmctrl -lpG | awk -v pid="$pid" '$3 == pid { print $1; exit }')"
    if [ -n "$wid" ]; then
      printf '%s\n' "$wid"
      return 0
    fi
    sleep 1
  done

  return 1
}

largest_component() {
  local image="$1"
  local threshold="$2"

  convert "$image" \
    -colorspace Gray \
    -threshold "$threshold" \
    -negate \
    -define connected-components:verbose=true \
    -connected-components 8 \
    null: 2>&1 | awk '
      $1 ~ /^[0-9]+:/ {
        split($1, idparts, ":");
        id = idparts[1] + 0;
        if (id == 0)
          next;

        split($2, bboxparts, /x|\+/);
        w = bboxparts[1] + 0;
        h = bboxparts[2] + 0;
        x = bboxparts[3] + 0;
        y = bboxparts[4] + 0;

        split($3, centroidparts, /,/);
        cx = centroidparts[1] + 0;
        cy = centroidparts[2] + 0;
        area = $4 + 0;

        if (area > best_area)
          {
          best_area = area;
          best_line = w " " h " " x " " y " " cx " " cy " " area;
          }
      }

      END {
        if (best_area > 0)
          print best_line;
      }'
}

detect_click_point() {
  local screenshot="$1"
  local crop_file="$2"
  local canvas_line
  local molecule_line
  local canvas_w canvas_h canvas_x canvas_y
  local mol_cx mol_cy

  canvas_line="$(largest_component "$screenshot" "5%")"
  if [ -z "$canvas_line" ]; then
    echo "Failed to detect the main canvas in $screenshot" >&2
    return 1
  fi

  read -r canvas_w canvas_h canvas_x canvas_y _ <<<"$canvas_line"
  convert "$screenshot" -crop "${canvas_w}x${canvas_h}+${canvas_x}+${canvas_y}" "$crop_file"

  molecule_line="$(largest_component "$crop_file" "8%")"
  if [ -z "$molecule_line" ]; then
    echo "Failed to detect visible model geometry in $crop_file" >&2
    return 1
  fi

  read -r _ _ _ _ mol_cx mol_cy _ <<<"$molecule_line"

  awk -v canvas_x="$canvas_x" \
      -v canvas_y="$canvas_y" \
      -v mol_cx="$mol_cx" \
      -v mol_cy="$mol_cy" \
      'BEGIN {
         click_x = int(canvas_x + mol_cx + 0.5);
         click_y = int(canvas_y + mol_cy + 0.5);
         print click_x, click_y;
       }'
}

run_pick_pass() {
  local target="$1"
  local window_id="$2"
  local pass_name="$3"
  local log_file="$4"
  local screenshot="$TMP_DIR/pick-align-${target}-${pass_name}.png"
  local crop_file="$TMP_DIR/pick-align-${target}-${pass_name}-canvas.png"
  local before_lines
  local click_x click_y
  local result

  import -window "$window_id" "png:$screenshot"
  read -r click_x click_y < <(detect_click_point "$screenshot" "$crop_file")

  before_lines=0
  if [ -f "$log_file" ]; then
    before_lines="$(wc -l < "$log_file")"
  fi

  xdotool windowactivate --sync "$window_id" \
          mousemove --window "$window_id" "$click_x" "$click_y" \
          click 1
  sleep 1

  result="$(tail -n +"$((before_lines + 1))" "$log_file" | awk -F'result=' '/GDIS pick: result=/{print $2}' | awk '{print $1}' | tail -n 1)"
  if [ -z "$result" ]; then
    echo "No pick result was recorded for $target/$pass_name." >&2
    echo "See log: $log_file" >&2
    return 1
  fi
  if [ "$result" = "(none)" ]; then
    echo "Pick missed visible geometry for $target/$pass_name." >&2
    echo "Screenshot: $screenshot" >&2
    echo "Crop: $crop_file" >&2
    echo "Log: $log_file" >&2
    return 1
  fi

  printf '%s %s: click=%s,%s picked=%s\n' "$target" "$pass_name" "$click_x" "$click_y" "$result"
}

run_target() {
  local target="$1"
  local gdis_exec
  local log_file="$TMP_DIR/pick-align-${target}.log"
  local pid
  local window_id
  local status=0

  gdis_exec="$(resolve_target_exec "$target")"
  : > "$log_file"

  env GDK_BACKEND=x11 \
      GDIS_DEBUG_PICK=1 \
      GDIS_DEBUG_INPUT=1 \
      GDIS_SUPPRESS_LIMITED_MODE_NOTICE=1 \
      NO_AT_BRIDGE=0 \
      "$gdis_exec" "$ROOT_DIR/$MODEL_PATH" >"$log_file" 2>&1 &
  pid=$!

  window_id="$(wait_for_window "$pid")" || {
    echo "Failed to find a GDIS window for PID $pid ($target)." >&2
    echo "See log: $log_file" >&2
    if [ "$KEEP_OPEN" -eq 0 ]; then
      kill "$pid" >/dev/null 2>&1 || true
      wait "$pid" >/dev/null 2>&1 || true
    fi
    return 1
  }

  sleep 1
  if run_pick_pass "$target" "$window_id" "initial" "$log_file"; then
    :
  else
    status=$?
  fi

  if [ "$status" -eq 0 ]; then
    wmctrl -ir "$window_id" -e 0,140,120,1180,820 >/dev/null 2>&1 || true
    sleep 1
    if run_pick_pass "$target" "$window_id" "resized" "$log_file"; then
      :
    else
      status=$?
    fi
  fi

  if [ "$KEEP_OPEN" -eq 0 ]; then
    kill "$pid" >/dev/null 2>&1 || true
    wait "$pid" >/dev/null 2>&1 || true
  fi

  if [ "$status" -ne 0 ]; then
    return "$status"
  fi

  printf '%s: log=%s\n' "$target" "$log_file"
  printf '%s: screenshots=%s %s\n' \
    "$target" \
    "$TMP_DIR/pick-align-${target}-initial.png" \
    "$TMP_DIR/pick-align-${target}-resized.png"
}

while [ $# -gt 0 ]; do
  case "$1" in
    --help|-h)
      usage
      exit 0
      ;;
    --gtk2|--gtk4)
      TARGETS+=("${1#--}")
      shift
      ;;
    --model)
      MODEL_PATH="${2:-}"
      shift 2
      ;;
    --wait)
      WAIT_SECS="${2:-}"
      shift 2
      ;;
    --keep-open)
      KEEP_OPEN=1
      shift
      ;;
    --)
      shift
      break
      ;;
    *)
      echo "Unknown option: $1" >&2
      usage >&2
      exit 1
      ;;
  esac
done

need_tool wmctrl
need_tool xdotool
need_tool import
need_tool convert

mkdir -p "$TMP_DIR"

if [ ! -f "$ROOT_DIR/$MODEL_PATH" ]; then
  echo "Missing model file: $ROOT_DIR/$MODEL_PATH" >&2
  exit 1
fi

if [ "${#TARGETS[@]}" -eq 0 ]; then
  if [ -x "$ROOT_DIR/bin/gdis-gtk4" ]; then
    TARGETS+=("gtk4")
  fi
  if [ -x "$ROOT_DIR/bin/gdis-gtk2" ]; then
    TARGETS+=("gtk2")
  fi
fi

if [ "${#TARGETS[@]}" -eq 0 ]; then
  echo "No runnable GDIS target found under $ROOT_DIR/bin." >&2
  exit 1
fi

for target in "${TARGETS[@]}"; do
  run_target "$target"
done
