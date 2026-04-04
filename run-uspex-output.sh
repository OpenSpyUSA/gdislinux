#!/usr/bin/env bash
set -eu

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

usage() {
  cat <<'EOF'
Usage:
  ./run-uspex-output.sh <results-dir>
  ./run-uspex-output.sh <results-dir>/OUTPUT.txt
  ./run-uspex-output.sh --gtk2 <results-dir>
  ./run-uspex-output.sh --gtk3 <results-dir>
  ./run-uspex-output.sh --gtk4 <results-dir>

Notes:
  - GDIS's USPEX loader expects a real USPEX results bundle.
  - Minimum practical files are:
      OUTPUT.txt
      Parameters.txt
      Individuals
      gatheredPOSCARS or gatheredPOSCARS_relaxed
  - The local models/INPUT.txt file is USPEX input, not a loadable result.
EOF
}

GTK_TARGET="${GDIS_GTK_TARGET:-}"

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
    --)
      shift
      break
      ;;
    -*)
      echo "Unknown option: $1" >&2
      echo >&2
      usage >&2
      exit 1
      ;;
    *)
      break
      ;;
  esac
done

if [ "${1:-}" = "" ]; then
  usage
  exit 1
fi

input_path="$1"

if [ -d "$input_path" ]; then
  results_dir="$input_path"
  output_file="$results_dir/OUTPUT.txt"
else
  output_file="$input_path"
  if [ "$(basename "$output_file")" != "OUTPUT.txt" ]; then
    echo "Expected a USPEX results directory or an OUTPUT.txt path." >&2
    exit 1
  fi
  results_dir="$(dirname "$output_file")"
fi

missing=0

check_required() {
  local path="$1"
  local label="$2"

  if [ ! -e "$path" ]; then
    echo "Missing $label: $path" >&2
    missing=1
  fi
}

check_required "$output_file" "USPEX output file"
check_required "$results_dir/Parameters.txt" "USPEX parameter file"
check_required "$results_dir/Individuals" "USPEX Individuals file"

if [ ! -e "$results_dir/gatheredPOSCARS" ] && [ ! -e "$results_dir/gatheredPOSCARS_relaxed" ]; then
  echo "Missing gathered POSCAR data in: $results_dir" >&2
  echo "Expected one of: gatheredPOSCARS or gatheredPOSCARS_relaxed" >&2
  missing=1
fi

if [ "$missing" -ne 0 ]; then
  echo >&2
  echo "This repository does not bundle a full USPEX results example." >&2
  echo "The local models/INPUT.txt file is only a USPEX input template." >&2
  exit 1
fi

if [ -n "$GTK_TARGET" ]; then
  exec "$ROOT_DIR/run-gdis.sh" "--$GTK_TARGET" "$output_file"
fi

exec "$ROOT_DIR/run-gdis.sh" "$output_file"
