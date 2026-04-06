#!/usr/bin/env bash
set -eu

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

usage() {
  cat <<'EOF'
Usage:
  ./run-gdis.sh
  ./run-gdis.sh --gtk2
  ./run-gdis.sh --gtk4 examples/methane.gin

Notes:
  - If no target is given, GTK4 is used by default.
  - GTK4 is still experimental, but it now renders atom views in a limited
    core-profile mode. Bonds, labels, graphs, and overlays still use the
    legacy path.
  - Remaining arguments are passed through to GDIS as files to open.
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

  if [ -x "$ROOT_DIR/bin/gdis-gtk4" ]; then
    printf '%s\n' "$ROOT_DIR/bin/gdis-gtk4"
    return
  fi

  if [ -x "$ROOT_DIR/bin/gdis-gtk2" ]; then
    printf '%s\n' "$ROOT_DIR/bin/gdis-gtk2"
    return
  fi

  if [ -x "$ROOT_DIR/bin/gdis" ]; then
    printf '%s\n' "$ROOT_DIR/bin/gdis"
    return
  fi

  echo "Missing executable: $ROOT_DIR/bin/gdis" >&2
  echo "Run ./rebuild-ubuntu.sh first." >&2
  exit 1
}

path_exists_for_run() {
  local candidate="$1"

  [ -e "$candidate" ] && return 0
  [ -e "$ROOT_DIR/$candidate" ] && return 0
  return 1
}

normalize_run_args() {
  local args=()

  while [ $# -gt 0 ]; do
    if [ $# -gt 1 ] && [[ "$1" == */ ]] && path_exists_for_run "$1$2"; then
      args+=("$1$2")
      shift 2
      continue
    fi

    args+=("$1")
    shift
  done

  printf '%s\0' "${args[@]}"
}

resolve_path_for_run() {
  local candidate="$1"

  if [ -e "$candidate" ]; then
    printf '%s\n' "$candidate"
    return
  fi

  if [ -e "$ROOT_DIR/$candidate" ]; then
    printf '%s\n' "$ROOT_DIR/$candidate"
    return
  fi

  printf '\n'
}

is_non_structure_sample() {
  local candidate="$1"

  case "$(basename "$candidate")" in
    INPUT.txt)
      return 0
      ;;
    *)
      return 1
      ;;
  esac
}

filter_run_args() {
  local args=()
  local candidate
  local resolved

  while [ $# -gt 0 ]; do
    candidate="$1"
    resolved="$(resolve_path_for_run "$candidate")"

    if [ -n "$resolved" ] && [ -d "$resolved" ]; then
      echo "Skipping directory argument: $candidate" >&2
      shift
      continue
    fi

    if [ -n "$resolved" ] && [ -f "$resolved" ] && is_non_structure_sample "$resolved"; then
      echo "Skipping non-structure sample file: $candidate" >&2
      shift
      continue
    fi

    args+=("$candidate")
    shift
  done

  printf '%s\0' "${args[@]}"
}

GTK_TARGET="${GDIS_GTK_TARGET:-gtk4}"

while [ $# -gt 0 ]; do
  case "$1" in
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

GDIS_EXEC="$(resolve_gdis_executable "$GTK_TARGET")"

if [ "$GTK_TARGET" = "gtk4" ] && command -v systemd-detect-virt >/dev/null 2>&1; then
  virt_type="$(systemd-detect-virt 2>/dev/null || true)"
  if [ -n "$virt_type" ] && [ "$virt_type" != "none" ]; then
    if [ -z "${LIBGL_ALWAYS_SOFTWARE-}" ]; then
      export LIBGL_ALWAYS_SOFTWARE=1
    fi
    if [ -z "${LIBGL_DRI3_DISABLE-}" ]; then
      export LIBGL_DRI3_DISABLE=1
    fi
  fi
fi

if [ "$GTK_TARGET" = "gtk4" ] &&
   [ -z "${GDIS_SUPPRESS_LIMITED_MODE_NOTICE-}" ] &&
   [ -z "${GDIS_RUN_VERBOSE-}" ]; then
  export GDIS_SUPPRESS_LIMITED_MODE_NOTICE=1
fi

if [ $# -gt 0 ]; then
  mapfile -d '' NORMALIZED_ARGS < <(normalize_run_args "$@")
  mapfile -d '' FILTERED_ARGS < <(filter_run_args "${NORMALIZED_ARGS[@]}")
  if [ "${#FILTERED_ARGS[@]}" -gt 0 ]; then
    exec "$GDIS_EXEC" "${FILTERED_ARGS[@]}"
  fi
fi

exec "$GDIS_EXEC"
