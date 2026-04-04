#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SEARCH_ROOT="${1:-$ROOT_DIR/src}"

if ! command -v rg >/dev/null 2>&1; then
  echo "This script requires rg (ripgrep)." >&2
  exit 1
fi

run_section() {
  local title="$1"
  local pattern="$2"

  echo
  echo "$title"
  echo "$(printf '%*s' "${#title}" '' | tr ' ' '=')"

  if ! rg -n --glob '*.[ch]' "$pattern" "$SEARCH_ROOT"; then
    echo "(no matches)"
  fi
}

cat <<EOF
GDIS core-profile OpenGL audit
Search root: $SEARCH_ROOT

This report highlights legacy OpenGL patterns that block a GtkGLArea core-
profile renderer path.
EOF

run_section \
  "Matrix Stack And GLU" \
  '\bgl(MatrixMode|LoadIdentity|Frustum|Ortho|Translatef|Rotatef|Scalef|PushMatrix|PopMatrix|GetDoublev|GetIntegerv)\b|\bglu(LookAt|Perspective|Project|UnProject)\b'

run_section \
  "Immediate Mode Geometry" \
  '\bgl(Begin|End|Vertex[234][dfis]?|Normal[234][dfis]?|Color[34][dfisub]?|TexCoord[1234][dfis]?)\b'

run_section \
  "Fixed-Function Lighting And Materials" \
  '\bgl(ShadeModel|Light(Model|[fi][v]?)|Material[fi][v]?|ColorMaterial|Fog[fi][v]?|ClipPlane)\b'

run_section \
  "Client-Side Vertex Arrays" \
  '\bgl(EnableClientState|DisableClientState|VertexPointer|NormalPointer|ColorPointer|TexCoordPointer)\b'

run_section \
  "Legacy Bitmap Or Raster Text" \
  '\bgl(Bitmap|RasterPos[234][dfis]?)\b'
