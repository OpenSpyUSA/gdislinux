#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

check_cmd() {
  local label="$1"
  local cmd="$2"
  if command -v "$cmd" >/dev/null 2>&1; then
    printf "%-18s %s\n" "$label" "$(command -v "$cmd")"
  else
    printf "%-18s %s\n" "$label" "missing"
  fi
}

check_first() {
  local label="$1"
  shift
  local candidate

  for candidate in "$@"; do
    if command -v "$candidate" >/dev/null 2>&1; then
      printf "%-18s %s\n" "$label" "$(command -v "$candidate")"
      return 0
    fi
  done

  printf "%-18s %s\n" "$label" "missing"
}

echo "Open-source optional tool check"
echo
check_cmd "gdis" "$ROOT_DIR/bin/gdis"
check_first "qbox" "$ROOT_DIR/bin/qbox" "qbox"
check_cmd "povray" "povray"
check_cmd "convert" "convert"
check_cmd "obabel" "obabel"
check_cmd "ffmpeg" "ffmpeg"
check_cmd "xmgrace" "xmgrace"

echo
echo "GDIS default executable names:"
echo "  Babel     : babel"
echo "  Qbox      : qbox"
echo "  POVRay    : povray"
echo "  Viewer    : display"
echo
echo "If Open Babel only provides obabel, set Babel manually in GDIS Setup."
