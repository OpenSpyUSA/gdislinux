#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
QBOX_XYZ_PY="${QBOX_XYZ_PY:-$ROOT_DIR/.localdeps/qbox-public/util/qbox_xyz.py}"
OPEN_RESULT=0
USE_PBC=0
GTK_TARGET="--gtk4"

usage() {
  cat <<'EOF'
Usage:
  ./qbox-out-to-xyz.sh [--pbc] [--open] [--gtk2|--gtk4] <qbox.out> [output.xyz]

Examples:
  ./qbox-out-to-xyz.sh tmp/qbox-gui/model_116.out
  ./qbox-out-to-xyz.sh --pbc --open --gtk4 tmp/qbox-gui/model_116.out
EOF
}

while [ $# -gt 0 ]; do
  case "$1" in
    --open)
      OPEN_RESULT=1
      shift
      ;;
    --pbc)
      USE_PBC=1
      shift
      ;;
    --gtk2|--gtk4)
      GTK_TARGET="$1"
      shift
      ;;
    --help|-h)
      usage
      exit 0
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

if [ $# -lt 1 ] || [ $# -gt 2 ]; then
  usage >&2
  exit 1
fi

INPUT_OUT="$1"
if [ $# -eq 2 ]; then
  OUTPUT_XYZ="$2"
else
  OUTPUT_XYZ="${INPUT_OUT%.*}.xyz"
fi

if [ ! -f "$INPUT_OUT" ]; then
  echo "Missing Qbox output file: $INPUT_OUT" >&2
  exit 1
fi

if [ ! -f "$QBOX_XYZ_PY" ]; then
  echo "Missing qbox_xyz.py: $QBOX_XYZ_PY" >&2
  echo "Build/install local Qbox first with ./install-qbox-local.sh" >&2
  exit 1
fi

if ! python3 -c "import numpy" >/dev/null 2>&1; then
  echo "python3 module 'numpy' is required by qbox_xyz.py." >&2
  echo "Install it (Ubuntu): sudo apt install python3-numpy" >&2
  exit 1
fi

mkdir -p "$(dirname "$OUTPUT_XYZ")"

if [ "$USE_PBC" -eq 1 ]; then
  python3 "$QBOX_XYZ_PY" -pbc "$INPUT_OUT" > "$OUTPUT_XYZ"
else
  python3 "$QBOX_XYZ_PY" "$INPUT_OUT" > "$OUTPUT_XYZ"
fi

echo "Wrote XYZ trajectory: $OUTPUT_XYZ"

if [ "$OPEN_RESULT" -eq 1 ]; then
  exec "$ROOT_DIR/run-gdis.sh" "$GTK_TARGET" "$OUTPUT_XYZ"
fi
