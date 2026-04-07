#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
UPF2QSO_SRC="$ROOT_DIR/.localdeps/qbox-public/util/upf2qso/src"
UPF2QSO_BIN="$UPF2QSO_SRC/upf2qso"

INPUT_DIR=""
OUTPUT_DIR=""
RCUT="8.0"

usage() {
  cat <<'EOF'
Usage:
  ./generate-qbox-xml-from-upf.sh --input-dir /path/to/upf-dir
  ./generate-qbox-xml-from-upf.sh --input-dir /path/to/upf-dir --output-dir /path/to/xml-dir
  ./generate-qbox-xml-from-upf.sh --input-dir /path/to/upf-dir --rcut 8.0

What it does:
  - Builds Qbox's upf2qso converter if needed
  - Converts all *.UPF / *.upf in input-dir to Qbox XML species files
  - Writes <stem>.xml for each UPF file

Notes:
  - upf2qso only supports norm-conserving UPF pseudopotentials.
  - Ultrasoft/PAW UPF files are not supported by this converter.
EOF
}

while [ $# -gt 0 ]; do
  case "$1" in
    --help|-h)
      usage
      exit 0
      ;;
    --input-dir)
      INPUT_DIR="${2:-}"
      shift 2
      ;;
    --output-dir)
      OUTPUT_DIR="${2:-}"
      shift 2
      ;;
    --rcut)
      RCUT="${2:-}"
      shift 2
      ;;
    *)
      echo "Unknown option: $1" >&2
      usage >&2
      exit 1
      ;;
  esac
done

if [ -z "$INPUT_DIR" ]; then
  echo "Missing required argument: --input-dir" >&2
  usage >&2
  exit 1
fi

if [ ! -d "$INPUT_DIR" ]; then
  echo "Input directory does not exist: $INPUT_DIR" >&2
  exit 1
fi

if [ -z "$OUTPUT_DIR" ]; then
  OUTPUT_DIR="$INPUT_DIR"
fi

mkdir -p "$OUTPUT_DIR"

if [ ! -x "$UPF2QSO_BIN" ]; then
  if [ ! -d "$UPF2QSO_SRC" ]; then
    echo "Missing upf2qso source directory: $UPF2QSO_SRC" >&2
    echo "Build/install Qbox local deps first (./install-qbox-local.sh)." >&2
    exit 1
  fi
  make -C "$UPF2QSO_SRC"
fi

mapfile -t UPF_FILES < <(find "$INPUT_DIR" -maxdepth 1 -type f \( -name "*.UPF" -o -name "*.upf" \) | sort)

if [ "${#UPF_FILES[@]}" -eq 0 ]; then
  echo "No UPF files found in: $INPUT_DIR" >&2
  exit 1
fi

ok=0
failed=0

for upf in "${UPF_FILES[@]}"; do
  base="$(basename "$upf")"
  stem="${base%.*}"
  out_xml="$OUTPUT_DIR/$stem.xml"
  log_file="$OUTPUT_DIR/$stem.upf2qso.log"

  if "$UPF2QSO_BIN" "$RCUT" < "$upf" > "$out_xml" 2> "$log_file"; then
    if grep -qi "not a Norm-conserving potential" "$log_file"; then
      echo "FAIL  $base -> not norm-conserving (unsupported by upf2qso)"
      rm -f "$out_xml"
      failed=$((failed+1))
      continue
    fi
    if grep -q "<fpmd:species" "$out_xml"; then
      echo "OK    $base -> $(basename "$out_xml")"
      ok=$((ok+1))
    else
      echo "FAIL  $base -> converter produced invalid XML"
      rm -f "$out_xml"
      failed=$((failed+1))
    fi
  else
    echo "FAIL  $base -> converter error (see $log_file)"
    rm -f "$out_xml"
    failed=$((failed+1))
  fi
done

echo
echo "Summary: ok=$ok failed=$failed output_dir=$OUTPUT_DIR"

if [ "$failed" -gt 0 ]; then
  exit 1
fi
