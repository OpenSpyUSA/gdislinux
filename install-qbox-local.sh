#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
QBOX_REPO_URL="${QBOX_REPO_URL:-https://github.com/qboxcode/qbox-public.git}"
QBOX_GIT_REF="${QBOX_GIT_REF:-rel1_78_4}"
QBOX_SRC_DIR="${QBOX_SRC_DIR:-$ROOT_DIR/.localdeps/qbox-public}"
QBOX_BUILD_DIR="$QBOX_SRC_DIR/src"
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

pick_first_tool() {
  local tool
  for tool in "$@"; do
    if command -v "$tool" >/dev/null 2>&1; then
      command -v "$tool"
      return 0
    fi
  done
  return 1
}

detect_mpi_cxx() {
  local requested="${QBOX_MPI_CXX:-}"

  if [ -n "$requested" ]; then
    if command -v "$requested" >/dev/null 2>&1; then
      command -v "$requested"
      return 0
    fi
    echo "Requested MPI C++ wrapper was not found: $requested" >&2
    return 1
  fi

  pick_first_tool \
    mpicxx \
    mpic++ \
    mpiCC \
    mpicxx.openmpi \
    mpic++.openmpi \
    mpicxx.mpich \
    mpic++.mpich
}

detect_qbox_platform() {
  case "$(uname -m)" in
    aarch64|arm64)
      printf '%s\n' "Linux_aarch64"
      ;;
    x86_64|amd64)
      printf '%s\n' "Linux_x86_64"
      ;;
    *)
      printf '%s\n' "Linux"
      ;;
  esac
}

detect_qbox_target_tag() {
  case "$(uname -m)" in
    aarch64|arm64)
      printf '%s\n' "aarch64"
      ;;
    x86_64|amd64)
      printf '%s\n' "x86_64"
      ;;
    *)
      uname -m | tr -c '[:alnum:]_-' '_'
      ;;
  esac
}

detect_scalapack_flag() {
  local mpi_cxx="$1"
  local compiler_name

  compiler_name="$(basename "$mpi_cxx")"

  if [[ "$compiler_name" == *mpich* ]]; then
    if pkg-config --exists scalapack-mpich; then
      printf '%s\n' "-lscalapack-mpich"
      return 0
    fi
    if ldconfig -p 2>/dev/null | grep -q "libscalapack-mpich"; then
      printf '%s\n' "-lscalapack-mpich"
      return 0
    fi
  fi

  if pkg-config --exists scalapack-openmpi; then
    printf '%s\n' "-lscalapack-openmpi"
    return 0
  fi
  if pkg-config --exists scalapack-mpich; then
    printf '%s\n' "-lscalapack-mpich"
    return 0
  fi
  if ldconfig -p 2>/dev/null | grep -q "libscalapack-openmpi"; then
    printf '%s\n' "-lscalapack-openmpi"
    return 0
  fi
  if ldconfig -p 2>/dev/null | grep -q "libscalapack-mpich"; then
    printf '%s\n' "-lscalapack-mpich"
    return 0
  fi
  if ldconfig -p 2>/dev/null | grep -q "libscalapack.so"; then
    printf '%s\n' "-lscalapack"
    return 0
  fi

  return 1
}

echo "Preparing local Qbox build in: $QBOX_SRC_DIR"
echo "Requested Qbox upstream ref: $QBOX_GIT_REF"

need_tool git
need_tool make
need_tool g++
need_tool pkg-config

QBOX_MPI_CXX_BIN="$(detect_mpi_cxx)" || {
  echo "Missing MPI C++ compiler wrapper." >&2
  echo "Tried: mpicxx, mpic++, mpiCC, mpicxx.openmpi, mpic++.openmpi, mpicxx.mpich, mpic++.mpich" >&2
  echo "Install OpenMPI/MPICH or set QBOX_MPI_CXX=/path/to/mpi-cxx-wrapper" >&2
  exit 1
}

QBOX_PLT="${QBOX_PLT:-$(detect_qbox_platform)}"
QBOX_ARCH_TAG="${QBOX_ARCH_TAG:-$(detect_qbox_target_tag)}"
QBOX_TARGET="${QBOX_TARGET:-ubuntu24_${QBOX_ARCH_TAG}_mpi}"
QBOX_TARGET_FILE="$QBOX_BUILD_DIR/$QBOX_TARGET.mk"

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

if ! SCALAPACK_FLAG="$(detect_scalapack_flag "$QBOX_MPI_CXX_BIN")"; then
  echo "Missing required library for ScaLAPACK" >&2
  echo "Install libscalapack-openmpi-dev or libscalapack-mpich-dev." >&2
  exit 1
fi

mkdir -p "$(dirname "$QBOX_SRC_DIR")"
if [ -d "$QBOX_SRC_DIR/.git" ]; then
  echo "Updating existing Qbox checkout"
else
  echo "Cloning Qbox upstream"
  git clone --no-checkout "$QBOX_REPO_URL" "$QBOX_SRC_DIR"
fi

git -C "$QBOX_SRC_DIR" fetch --depth 1 origin "$QBOX_GIT_REF"
git -C "$QBOX_SRC_DIR" checkout --detach FETCH_HEAD
git -C "$QBOX_SRC_DIR" clean -fdx

cat >"$QBOX_TARGET_FILE" <<EOF
# Auto-generated local target for Ubuntu Linux + MPI.
PLT=$QBOX_PLT

PLTOBJECTS =

CXX=$QBOX_MPI_CXX_BIN
LD=\$(CXX)

PLTFLAGS += -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 \\
            -DUSE_MPI -DSCALAPACK -DADD_ -DAPP_NO_THREADS \\
            -DXML_USE_NO_THREADS -DUSE_XERCES -DXERCESC_3 \\
            -DMPICH_IGNORE_CXX_SEEK -DUSE_UUID -DUSE_FFTW3 -DFFTW3_2D

CXXFLAGS= -g -O3 -Wunused -D\$(PLT) \$(INCLUDE) \$(PLTFLAGS) \$(DFLAGS)
LIBS += -lfftw3 -lpthread -lxerces-c $SCALAPACK_FLAG -llapack -lblas -luuid
LDFLAGS = \$(LIBPATH) \$(LIBS)
EOF

echo "Using MPI C++ wrapper: $QBOX_MPI_CXX_BIN"
echo "Using platform macro: $QBOX_PLT"
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
