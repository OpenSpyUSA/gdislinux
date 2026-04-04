#!/usr/bin/env bash
set -eu

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

usage() {
  cat <<'EOF'
Usage:
  ./run-example.sh <name>
  ./run-example.sh --gtk3 <name>
  ./run-example.sh --gtk4 <name>
  ./run-example.sh --list

Available examples:
  methane   Small GULP input (.gin), fastest first test
  water     Small Biosym/InsightII file (.car)
  adp1      Crystal structure CIF example (.cif)
  deoxy     Molecule/protein-style PDB example (.pdb)
  calc2d    Simple XTL example (.xtl)
  vasp      VASP XML output example (.xml)
  vasprun   Alias for the same VASP XML example

Examples:
  ./run-example.sh methane
  ./run-example.sh --gtk3 methane
  ./run-example.sh --gtk4 methane
  ./run-example.sh adp1
EOF
}

resolve_gdis_executable() {
  local gtk_target="${1:-}"
  local gdis_exec

  if [ -n "$gtk_target" ]; then
    gdis_exec="$ROOT_DIR/bin/gdis-$gtk_target"
    if [ ! -x "$gdis_exec" ]; then
      echo "Missing executable: $gdis_exec" >&2
      echo "Build it first with: GDIS_GTK_TARGET=$gtk_target ./rebuild-ubuntu.sh" >&2
      exit 1
    fi
    printf '%s\n' "$gdis_exec"
    return
  fi

  if [ -x "$ROOT_DIR/bin/gdis-gtk2" ]; then
    printf '%s\n' "$ROOT_DIR/bin/gdis-gtk2"
    return
  fi

  gdis_exec="$ROOT_DIR/bin/gdis"
  if [ ! -x "$gdis_exec" ]; then
    echo "Missing executable: $gdis_exec" >&2
    echo "Run ./rebuild-ubuntu.sh first." >&2
    exit 1
  fi

  printf '%s\n' "$gdis_exec"
}

GTK_TARGET="${GDIS_GTK_TARGET:-}"

while [ $# -gt 0 ]; do
  case "$1" in
    --list|-l)
      usage
      exit 0
      ;;
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
  exit 0
fi

case "$1" in
  methane) example_file="examples/methane.gin" ;;
  water) example_file="examples/water.car" ;;
  adp1) example_file="examples/adp1.cif" ;;
  deoxy) example_file="examples/deoxy.pdb" ;;
  calc2d) example_file="examples/calc2d.xtl" ;;
  vasp|vasprun) example_file="examples/vasprun.xml" ;;
  *)
    echo "Unknown example: $1" >&2
    echo >&2
    usage >&2
    exit 1
    ;;
esac

GDIS_EXEC="$(resolve_gdis_executable "$GTK_TARGET")"

exec "$GDIS_EXEC" "$ROOT_DIR/$example_file"
