#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PKG_CONFIG_BIN="${PKG_CONFIG:-pkg-config}"
GTK_TARGET="${GDIS_GTK_TARGET:-gtk2}"

case "$GTK_TARGET" in
  gtk2)
    DEFAULT_BASE_PKG_CONFIG_PACKAGES="gtk+-2.0 gthread-2.0 gmodule-2.0"
    DEFAULT_GUI_PKG_CONFIG_PACKAGES="$DEFAULT_BASE_PKG_CONFIG_PACKAGES gtkglext-1.0"
    UBUNTU_PACKAGES=(build-essential pkg-config libgtk2.0-dev libgtkglext1-dev)
    ;;
  gtk3)
    DEFAULT_BASE_PKG_CONFIG_PACKAGES="gtk+-3.0 gthread-2.0 gmodule-2.0"
    DEFAULT_GUI_PKG_CONFIG_PACKAGES="$DEFAULT_BASE_PKG_CONFIG_PACKAGES gl glu epoxy"
    UBUNTU_PACKAGES=(build-essential pkg-config libgtk-3-dev libgl1-mesa-dev libglu1-mesa-dev libepoxy-dev)
    ;;
  gtk4)
    DEFAULT_BASE_PKG_CONFIG_PACKAGES="gtk4 gthread-2.0 gmodule-2.0"
    DEFAULT_GUI_PKG_CONFIG_PACKAGES="$DEFAULT_BASE_PKG_CONFIG_PACKAGES gl glu epoxy"
    UBUNTU_PACKAGES=(build-essential pkg-config libgtk-4-dev libgl1-mesa-dev libglu1-mesa-dev libepoxy-dev)
    ;;
  *)
    echo "Unsupported GDIS_GTK_TARGET: $GTK_TARGET" >&2
    echo "Supported values: gtk2, gtk3, gtk4" >&2
    exit 1
    ;;
esac

BASE_PKG_CONFIG_PACKAGES="${GDIS_BASE_PKG_CONFIG_PACKAGES:-$DEFAULT_BASE_PKG_CONFIG_PACKAGES}"
GUI_PKG_CONFIG_PACKAGES="${GDIS_GUI_PKG_CONFIG_PACKAGES:-$DEFAULT_GUI_PKG_CONFIG_PACKAGES}"

missing_tools=()
required_tools=(perl gcc make "$PKG_CONFIG_BIN")

for tool in "${required_tools[@]}"; do
  if ! command -v "$tool" >/dev/null 2>&1; then
    missing_tools+=("$tool")
  fi
done

missing_packages=()
missing_modules=()
pkg_map=(
  "gtk+-2.0:libgtk2.0-dev"
  "gtk+-3.0:libgtk-3-dev"
  "gtk4:libgtk-4-dev"
  "gl:libgl1-mesa-dev"
  "glu:libglu1-mesa-dev"
  "epoxy:libepoxy-dev"
  "gthread-2.0:libglib2.0-dev"
  "gmodule-2.0:libglib2.0-dev"
  "gtkglext-1.0:libgtkglext1-dev"
)
declare -A apt_hints=()

for entry in "${pkg_map[@]}"; do
  pc_name="${entry%%:*}"
  apt_name="${entry##*:}"
  apt_hints["$pc_name"]="$apt_name"
done

if command -v "$PKG_CONFIG_BIN" >/dev/null 2>&1; then
  read -r -a required_pkg_modules <<<"$GUI_PKG_CONFIG_PACKAGES"
  for pc_name in "${required_pkg_modules[@]}"; do
    if ! "$PKG_CONFIG_BIN" --exists "$pc_name"; then
      missing_modules+=("$pc_name")
      if [[ -n "${apt_hints[$pc_name]:-}" ]]; then
        missing_packages+=("${apt_hints[$pc_name]}")
      fi
    fi
  done
fi

if ((${#missing_tools[@]} > 0 || ${#missing_modules[@]} > 0)); then
  echo "Missing build prerequisites for gdis."
  echo "GTK target: $GTK_TARGET"
  if ((${#missing_tools[@]} > 0)); then
    echo "Tools: ${missing_tools[*]}"
  fi
  if ((${#missing_modules[@]} > 0)); then
    echo "Pkg-config modules: ${missing_modules[*]}"
  fi
  if ((${#missing_packages[@]} > 0)); then
    unique_packages=($(printf '%s\n' "${missing_packages[@]}" | sort -u))
    echo "Ubuntu packages: ${unique_packages[*]}"
  fi
  echo
  echo "On Ubuntu 24.04, install them with:"
  echo "  sudo apt-get update"
  echo "  sudo apt-get install -y ${UBUNTU_PACKAGES[*]}"
  exit 1
fi

echo "GTK target: $GTK_TARGET"
if [[ "$GTK_TARGET" == "gtk4" ]]; then
  echo "GTK4 support is still experimental. The current renewal build has a compact menu shim plus a modern core renderer for atoms, bond/stick geometry, cell frames, graph plots/text, iso-surfaces, and several overlays, while some legacy overlays and interaction paths still need porting."
fi

cd "$ROOT_DIR"
export PKG_CONFIG="$PKG_CONFIG_BIN"
export GDIS_GTK_TARGET="$GTK_TARGET"
export GDIS_BASE_PKG_CONFIG_PACKAGES="$BASE_PKG_CONFIG_PACKAGES"
export GDIS_GUI_PKG_CONFIG_PACKAGES="$GUI_PKG_CONFIG_PACKAGES"
perl ./install default
