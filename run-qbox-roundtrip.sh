#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
QBOX_EXEC="$ROOT_DIR/bin/qbox"
WORK_DIR="$ROOT_DIR/tmp/qbox-roundtrip"
INPUT_FILE="$WORK_DIR/methane.i"
LOG_FILE="$WORK_DIR/methane.log"
SAVE_FILE="$WORK_DIR/methane.xml"
C_PSP="$ROOT_DIR/.localdeps/qbox-public/test/potentials/C_HSCV_PBE-1.0.xml"
H_PSP="$ROOT_DIR/.localdeps/qbox-public/test/potentials/H_HSCV_PBE-1.0.xml"
OPEN_RESULT=0
GTK_TARGET=""

usage() {
  cat <<'EOF'
Usage:
  ./run-qbox-roundtrip.sh
  ./run-qbox-roundtrip.sh --open
  ./run-qbox-roundtrip.sh --open --gtk4

What it does:
  1. writes a minimal methane Qbox input using the locally built Qbox binary
  2. uses the bundled Qbox C/H pseudopotential XML files from .localdeps/
  3. runs Qbox and saves methane.xml
  4. optionally opens the saved XML in GDIS
EOF
}

while [ $# -gt 0 ]; do
  case "$1" in
    --open)
      OPEN_RESULT=1
      shift
      ;;
    --gtk2|--gtk3|--gtk4)
      GTK_TARGET="$1"
      shift
      ;;
    --help|-h)
      usage
      exit 0
      ;;
    *)
      echo "Unknown option: $1" >&2
      usage >&2
      exit 1
      ;;
  esac
done

if [ ! -x "$QBOX_EXEC" ]; then
  echo "Missing Qbox executable: $QBOX_EXEC" >&2
  echo "Build it first with ./install-qbox-local.sh" >&2
  exit 1
fi

if [ ! -f "$C_PSP" ] || [ ! -f "$H_PSP" ]; then
  echo "Missing bundled Qbox pseudopotentials under .localdeps/qbox-public/test/potentials" >&2
  exit 1
fi

mkdir -p "$WORK_DIR"

cat >"$INPUT_FILE" <<EOF
set cell 8 0 0  0 8 0  0 0 8
species carbon $C_PSP
species hydrogen $H_PSP
atom C carbon 0.00000000 0.00000000 0.00000000
atom H1 hydrogen 1.20000000 1.20000000 1.20000000
atom H2 hydrogen 1.20000000 -1.20000000 -1.20000000
atom H3 hydrogen -1.20000000 1.20000000 -1.20000000
atom H4 hydrogen -1.20000000 -1.20000000 1.20000000
set ecut 35
set xc PBE
randomize_wf
set wf_dyn PSDA
set ecutprec 5
run 0 40
save methane.xml
quit
EOF

(
  cd "$WORK_DIR"
  "$QBOX_EXEC" < "$(basename "$INPUT_FILE")" > "$(basename "$LOG_FILE")"
)

echo "Qbox round-trip files:"
echo "  Input : $INPUT_FILE"
echo "  Log   : $LOG_FILE"
echo "  Saved : $SAVE_FILE"

if [ "$OPEN_RESULT" -eq 1 ]; then
  exec "$ROOT_DIR/run-gdis.sh" ${GTK_TARGET:+$GTK_TARGET} "$SAVE_FILE"
fi
