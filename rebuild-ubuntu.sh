#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PKG_CONFIG_BIN="${PKG_CONFIG:-pkg-config}"
BASE_PKG_CONFIG_PACKAGES="${GDIS_BASE_PKG_CONFIG_PACKAGES:-gtk+-2.0 gthread-2.0 gmodule-2.0}"
GUI_PKG_CONFIG_PACKAGES="${GDIS_GUI_PKG_CONFIG_PACKAGES:-$BASE_PKG_CONFIG_PACKAGES gtkglext-1.0}"

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
  echo "  sudo apt-get install -y build-essential pkg-config libgtk2.0-dev libgtkglext1-dev"
  exit 1
fi

cd "$ROOT_DIR"
export PKG_CONFIG="$PKG_CONFIG_BIN"
export GDIS_BASE_PKG_CONFIG_PACKAGES="$BASE_PKG_CONFIG_PACKAGES"
export GDIS_GUI_PKG_CONFIG_PACKAGES="$GUI_PKG_CONFIG_PACKAGES"
perl ./install default
