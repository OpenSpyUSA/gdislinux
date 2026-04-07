#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
QBOX_EXEC="${QBOX_EXEC:-$ROOT_DIR/bin/qbox}"
WORK_DIR="${WORK_DIR:-$ROOT_DIR/tmp/qbox-smoke}"
INPUT_FILE="$WORK_DIR/methane-smoke.in"
LOG_FILE="$WORK_DIR/methane-smoke.out"
SAVE_FILE="$WORK_DIR/methane-smoke.xml"
C_PSP="$ROOT_DIR/.localdeps/qbox-public/test/potentials/C_HSCV_PBE-1.0.xml"
H_PSP="$ROOT_DIR/.localdeps/qbox-public/test/potentials/H_HSCV_PBE-1.0.xml"
OPEN_RESULT=0
GTK_TARGET="--gtk4"
RUN_STEP="${RUN_STEP:-6}"
RUN_TIMEOUT="${RUN_TIMEOUT:-90s}"
ECUT="${ECUT:-20}"
SCF_TOL="${SCF_TOL:-1e-4}"

usage() {
  cat <<'EOF'
Usage:
  ./run-qbox-smoke.sh
  ./run-qbox-smoke.sh --open
  ./run-qbox-smoke.sh --open --gtk2

Purpose:
  Fast, low-heat Qbox sanity check.
  Uses one CPU thread, low scheduling priority, reduced ecut, and short run length.
EOF
}

while [ $# -gt 0 ]; do
  case "$1" in
    --open)
      OPEN_RESULT=1
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
    *)
      echo "Unknown option: $1" >&2
      usage >&2
      exit 1
      ;;
  esac
done

if [ ! -x "$QBOX_EXEC" ]; then
  echo "Missing Qbox executable: $QBOX_EXEC" >&2
  echo "Build/install Qbox first (for this tree: ./install-qbox-local.sh)." >&2
  exit 1
fi

if [ ! -f "$C_PSP" ] || [ ! -f "$H_PSP" ]; then
  echo "Missing bundled C/H pseudopotentials under .localdeps/qbox-public/test/potentials" >&2
  exit 1
fi

mkdir -p "$WORK_DIR"

cat >"$INPUT_FILE" <<EOF
set cell 12 0 0  0 12 0  0 0 12
species carbon $C_PSP
species hydrogen $H_PSP
atom C carbon 0.00000000 0.00000000 0.00000000
atom H1 hydrogen 1.20000000 1.20000000 1.20000000
atom H2 hydrogen 1.20000000 -1.20000000 -1.20000000
atom H3 hydrogen -1.20000000 1.20000000 -1.20000000
atom H4 hydrogen -1.20000000 -1.20000000 1.20000000
set ecut $ECUT
set xc PBE
set wf_dyn PSDA
set ecutprec 2
set scf_tol $SCF_TOL
randomize_wf
run 0 $RUN_STEP
save methane-smoke.xml
quit
EOF

(
  cd "$WORK_DIR"
  export OMP_NUM_THREADS=1
  export OPENBLAS_NUM_THREADS=1
  export MKL_NUM_THREADS=1
  export BLIS_NUM_THREADS=1
  if command -v timeout >/dev/null 2>&1; then
    nice -n 19 timeout "$RUN_TIMEOUT" "$QBOX_EXEC" < "$(basename "$INPUT_FILE")" > "$(basename "$LOG_FILE")"
  else
    nice -n 19 "$QBOX_EXEC" < "$(basename "$INPUT_FILE")" > "$(basename "$LOG_FILE")"
  fi
)

echo "Qbox smoke test complete:"
echo "  Input : $INPUT_FILE"
echo "  Log   : $LOG_FILE"
echo "  Saved : $SAVE_FILE"
echo
echo "Quick pass criteria:"
echo "  - Log contains \"<cmd>run 0\" and no immediate fatal error"
echo "  - Saved XML exists"

if [ "$OPEN_RESULT" -eq 1 ]; then
  exec "$ROOT_DIR/run-gdis.sh" "$GTK_TARGET" "$SAVE_FILE"
fi
