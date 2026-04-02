#!/usr/bin/env bash
set -eu

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

usage() {
  cat <<'EOF'
Usage:
  ./run-example.sh <name>
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
  ./run-example.sh adp1
EOF
}

if [ "${1:-}" = "" ] || [ "${1:-}" = "--list" ] || [ "${1:-}" = "-l" ]; then
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

exec "$ROOT_DIR/bin/gdis" "$ROOT_DIR/$example_file"
