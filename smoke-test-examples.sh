#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TIMEOUT_SECONDS="${GDIS_SMOKE_TIMEOUT:-8}"

usage() {
  cat <<'EOF'
Usage:
  ./smoke-test-examples.sh
  ./smoke-test-examples.sh methane water adp1

Environment:
  GDIS_SMOKE_TIMEOUT   Per-example timeout in seconds (default: 8)
EOF
}

example_path() {
  case "$1" in
    methane) printf '%s\n' "examples/methane.gin" ;;
    water) printf '%s\n' "examples/water.car" ;;
    adp1) printf '%s\n' "examples/adp1.cif" ;;
    deoxy) printf '%s\n' "examples/deoxy.pdb" ;;
    calc2d) printf '%s\n' "examples/calc2d.xtl" ;;
    vasp) printf '%s\n' "examples/vasprun.xml" ;;
    *)
      echo "Unknown example: $1" >&2
      usage >&2
      return 1
      ;;
  esac
}

if [[ "${1:-}" == "--help" || "${1:-}" == "-h" ]]; then
  usage
  exit 0
fi

if [[ ! -x "$ROOT_DIR/bin/gdis" ]]; then
  echo "Missing executable: $ROOT_DIR/bin/gdis" >&2
  echo "Run ./rebuild-ubuntu.sh first." >&2
  exit 1
fi

timeout_cmd="$(command -v timeout || command -v gtimeout || true)"
if [[ -z "$timeout_cmd" ]]; then
  echo "This smoke test requires timeout or gtimeout." >&2
  exit 1
fi

runner=()
if [[ -n "${DISPLAY:-}" ]]; then
  :
elif command -v xvfb-run >/dev/null 2>&1; then
  runner=(xvfb-run -a)
else
  echo "No DISPLAY is set and xvfb-run is unavailable." >&2
  exit 1
fi

if (($# == 0)); then
  examples=(methane water adp1 deoxy calc2d vasp)
else
  examples=("$@")
fi

for example in "${examples[@]}"; do
  path="$(example_path "$example")"
  log_file="$(mktemp "/tmp/gdis-smoke-${example}.XXXXXX.log")"

  echo "Smoke testing: $example"

  set +e
  "$timeout_cmd" "$TIMEOUT_SECONDS" "${runner[@]}" \
    "$ROOT_DIR/bin/gdis" "$ROOT_DIR/$path" >"$log_file" 2>&1
  status=$?
  set -e

  if [[ $status -ne 0 && $status -ne 124 ]]; then
    echo "Smoke test failed for $example (exit $status)." >&2
    tail -n 80 "$log_file" >&2 || true
    exit $status
  fi

  if rg -n "AddressSanitizer|runtime error:|Segmentation fault|stack-overflow|ABORTING" "$log_file" >/dev/null 2>&1; then
    echo "Smoke test detected a runtime error for $example." >&2
    tail -n 80 "$log_file" >&2 || true
    exit 1
  fi
done

echo "Smoke tests passed."
