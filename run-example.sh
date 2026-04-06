#!/usr/bin/env bash
set -eu

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

usage() {
  cat <<'EOF'
Usage:
  ./run-example.sh <name>
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
  ./run-example.sh --gtk4 methane
  ./run-example.sh adp1
EOF
}

GTK_TARGET="${GDIS_GTK_TARGET:-gtk4}"

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
    --gtk2|--gtk4)
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

if [ -n "$GTK_TARGET" ] &&
   [ "$GTK_TARGET" != "gtk2" ] &&
   [ "$GTK_TARGET" != "gtk4" ]; then
  echo "Unsupported GTK target: $GTK_TARGET" >&2
  echo "Supported targets are gtk2 and gtk4." >&2
  exit 1
fi

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

if [ -n "$GTK_TARGET" ]; then
  exec "$ROOT_DIR/run-gdis.sh" "--$GTK_TARGET" "$ROOT_DIR/$example_file"
fi

exec "$ROOT_DIR/run-gdis.sh" "$ROOT_DIR/$example_file"
