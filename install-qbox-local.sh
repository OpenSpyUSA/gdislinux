#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
QBOX_REPO_URL="${QBOX_REPO_URL:-https://github.com/qboxcode/qbox-public.git}"
QBOX_SRC_DIR="${QBOX_SRC_DIR:-$ROOT_DIR/.localdeps/qbox-public}"
QBOX_BUILD_DIR="$QBOX_SRC_DIR/src"
QBOX_TARGET="${QBOX_TARGET:-ubuntu24_aarch64_openmpi}"
QBOX_TARGET_FILE="$QBOX_BUILD_DIR/$QBOX_TARGET.mk"
QBOX_BINARY="$QBOX_BUILD_DIR/qb"
QBOX_LINK="$ROOT_DIR/bin/qbox"
QB_LINK="$ROOT_DIR/bin/qb"

need_tool() {
  local tool="$1"
  if ! command -v "$tool" >/dev/null 2>&1; then
    echo "Missing required tool: $tool" >&2
    return 1
  fi
}

echo "Preparing local Qbox build in: $QBOX_SRC_DIR"

need_tool git
need_tool make
need_tool g++
need_tool mpicxx

if ! pkg-config --exists xerces-c; then
  echo "Missing pkg-config module: xerces-c" >&2
  echo "Install the Ubuntu build dependencies first." >&2
  exit 1
fi

if ! pkg-config --exists fftw3; then
  echo "Missing required pkg-config module: fftw3" >&2
  echo "Install the Ubuntu build dependencies first." >&2
  exit 1
fi

if pkg-config --exists scalapack-openmpi; then
  SCALAPACK_FLAG="-lscalapack-openmpi"
elif ldconfig -p 2>/dev/null | grep -q "libscalapack.so"; then
  SCALAPACK_FLAG="-lscalapack"
else
  echo "Missing required library for ScaLAPACK" >&2
  exit 1
fi

mkdir -p "$(dirname "$QBOX_SRC_DIR")"
if [ -d "$QBOX_SRC_DIR/.git" ]; then
  echo "Updating existing Qbox checkout"
  git -C "$QBOX_SRC_DIR" fetch --depth 1 origin master
  git -C "$QBOX_SRC_DIR" reset --hard origin/master
else
  echo "Cloning Qbox upstream"
  git clone --depth 1 "$QBOX_REPO_URL" "$QBOX_SRC_DIR"
fi

cat >"$QBOX_TARGET_FILE" <<EOF
# Auto-generated local target for Ubuntu 24.04 arm64 + OpenMPI.
PLT=Linux_aarch64

PLTOBJECTS =

CXX=mpicxx
LD=\$(CXX)

PLTFLAGS += -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 \\
            -DUSE_MPI -DSCALAPACK -DADD_ -DAPP_NO_THREADS \\
            -DXML_USE_NO_THREADS -DUSE_XERCES -DXERCESC_3 \\
            -DMPICH_IGNORE_CXX_SEEK -DUSE_UUID -DUSE_FFTW3 -DFFTW3_2D

CXXFLAGS= -g -O3 -Wunused -D\$(PLT) \$(INCLUDE) \$(PLTFLAGS) \$(DFLAGS)
LIBS += -lfftw3 -lpthread -lxerces-c $SCALAPACK_FLAG -llapack -lblas -luuid
LDFLAGS = \$(LIBPATH) \$(LIBS)
EOF

echo "Using ScaLAPACK linker flag: $SCALAPACK_FLAG"
echo "Building qb"
make -C "$QBOX_BUILD_DIR" TARGET="$QBOX_TARGET" clean qb

if [ ! -x "$QBOX_BINARY" ]; then
  echo "Qbox build did not produce: $QBOX_BINARY" >&2
  exit 1
fi

mkdir -p "$ROOT_DIR/bin"
ln -sfn "../.localdeps/qbox-public/src/qb" "$QBOX_LINK"
ln -sfn "../.localdeps/qbox-public/src/qb" "$QB_LINK"

echo
echo "Qbox build successful."
echo "Local binary:"
echo "  $QBOX_BINARY"
echo "Convenience links:"
echo "  $QBOX_LINK"
echo "  $QB_LINK"
echo
echo "Point GDIS Setup > Executable locations > Qbox to:"
echo "  $QBOX_LINK"
