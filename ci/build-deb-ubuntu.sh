#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUTPUT_DIR="$ROOT_DIR/.build/deb-artifacts"
UBUNTU_SERIES=""
UBUNTU_VERSION=""
VERSION_SUFFIX=""

usage() {
  cat <<'EOF'
Usage:
  ./ci/build-deb-ubuntu.sh [--series jammy] [--version-id 22.04] [--suffix ~ubuntu22.04.1] [--output-dir DIR]

Notes:
  - The script restores debian/changelog before exiting.
  - The build output is collected into the chosen output directory.
  - By default, the Ubuntu series/version are detected from /etc/os-release.
EOF
}

need_cmd() {
  command -v "$1" >/dev/null 2>&1 || {
    echo "Missing required command: $1" >&2
    exit 1
  }
}

while [ $# -gt 0 ]; do
  case "$1" in
    --series)
      UBUNTU_SERIES="$2"
      shift 2
      ;;
    --version-id)
      UBUNTU_VERSION="$2"
      shift 2
      ;;
    --suffix)
      VERSION_SUFFIX="$2"
      shift 2
      ;;
    --output-dir)
      OUTPUT_DIR="$2"
      shift 2
      ;;
    --help|-h)
      usage
      exit 0
      ;;
    *)
      echo "Unknown option: $1" >&2
      echo >&2
      usage >&2
      exit 1
      ;;
  esac
done

need_cmd dpkg
need_cmd dpkg-buildpackage
need_cmd dpkg-parsechangelog
need_cmd perl
need_cmd sha256sum

if [ -r /etc/os-release ]; then
  # shellcheck disable=SC1091
  . /etc/os-release
fi

if [ -z "$UBUNTU_SERIES" ]; then
  UBUNTU_SERIES="${UBUNTU_CODENAME:-${VERSION_CODENAME:-unstable}}"
fi

if [ -z "$UBUNTU_VERSION" ]; then
  UBUNTU_VERSION="${VERSION_ID:-unknown}"
fi

if [ -z "$VERSION_SUFFIX" ]; then
  VERSION_SUFFIX="~ubuntu${UBUNTU_VERSION}.1"
fi

cd "$ROOT_DIR"

CHANGELOG_BACKUP="$(mktemp)"
cp debian/changelog "$CHANGELOG_BACKUP"
cleanup() {
  cp "$CHANGELOG_BACKUP" debian/changelog
  rm -f "$CHANGELOG_BACKUP"
}
trap cleanup EXIT

SOURCE_PACKAGE="$(dpkg-parsechangelog -SSource)"
BASE_VERSION="$(dpkg-parsechangelog -SVersion)"
if [[ "$BASE_VERSION" == *"$VERSION_SUFFIX" ]]; then
  TARGET_VERSION="$BASE_VERSION"
else
  TARGET_VERSION="${BASE_VERSION}${VERSION_SUFFIX}"
fi

export SOURCE_PACKAGE BASE_VERSION TARGET_VERSION UBUNTU_SERIES
perl -0pi -e '
  s/^\Q$ENV{SOURCE_PACKAGE}\E \(\Q$ENV{BASE_VERSION}\E\) \S+;/$ENV{SOURCE_PACKAGE} ($ENV{TARGET_VERSION}) $ENV{UBUNTU_SERIES};/m
    or die "failed to update debian/changelog version header\n";
' debian/changelog

dpkg-buildpackage -us -uc -b

ARCH="$(dpkg --print-architecture)"
mkdir -p "$OUTPUT_DIR"

shopt -s nullglob
artifacts=(
  ../*_"${TARGET_VERSION}"_*.deb
  ../*_"${TARGET_VERSION}"_*.ddeb
  ../*_"${TARGET_VERSION}"_*.buildinfo
  ../*_"${TARGET_VERSION}"_*.changes
)

if [ "${#artifacts[@]}" -eq 0 ]; then
  echo "No build artifacts were produced for version: $TARGET_VERSION" >&2
  exit 1
fi

for artifact in "${artifacts[@]}"; do
  if [ ! -f "$artifact" ]; then
    echo "Expected build artifact not found: $artifact" >&2
    exit 1
  fi
  cp "$artifact" "$OUTPUT_DIR/"
done

(
  cd "$OUTPUT_DIR"
  package_files=( *.deb *.ddeb *.buildinfo *.changes )
  if [ "${#package_files[@]}" -eq 0 ]; then
    echo "No copied package files found in $OUTPUT_DIR" >&2
    exit 1
  fi
  checksum_file="SHA256SUMS-${UBUNTU_SERIES}-${ARCH}"
  sha256sum "${package_files[@]}" > "$checksum_file"
)

printf 'Built package artifacts in %s\n' "$OUTPUT_DIR"
printf 'Ubuntu target: %s (%s)\n' "$UBUNTU_SERIES" "$UBUNTU_VERSION"
printf 'Package version: %s\n' "$TARGET_VERSION"
