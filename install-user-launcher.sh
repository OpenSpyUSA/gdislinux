#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN_DIR="${XDG_BIN_HOME:-$HOME/.local/bin}"
LAUNCHER="$BIN_DIR/gdis"
TARGET="$ROOT_DIR/run-gdis.sh"
ROOT_DIR_ESCAPED="$(printf '%q' "$ROOT_DIR")"

usage() {
  cat <<'EOF'
Usage:
  ./install-user-launcher.sh
  ./install-user-launcher.sh --remove

What it does:
  - Installs a user-level `gdis` command in ~/.local/bin (or $XDG_BIN_HOME)
  - `gdis` points to this repository's run-gdis.sh launcher
  - Default runtime stays GTK4, same as run-gdis.sh
EOF
}

case "${1:-}" in
  --help|-h)
    usage
    exit 0
    ;;
  --remove)
    if [ -L "$LAUNCHER" ] || [ -f "$LAUNCHER" ]; then
      rm -f "$LAUNCHER"
      echo "Removed launcher: $LAUNCHER"
    else
      echo "No launcher found at: $LAUNCHER"
    fi
    exit 0
    ;;
  "")
    ;;
  *)
    echo "Unknown option: $1" >&2
    usage >&2
    exit 1
    ;;
esac

if [ ! -x "$TARGET" ]; then
  echo "Missing executable launcher: $TARGET" >&2
  exit 1
fi

mkdir -p "$BIN_DIR"

if [ -L "$LAUNCHER" ]; then
  rm -f "$LAUNCHER"
fi

cat > "$LAUNCHER" <<EOF
#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR=$ROOT_DIR_ESCAPED
exec "\$ROOT_DIR/run-gdis.sh" "\$@"
EOF

chmod +x "$LAUNCHER"

echo "Installed launcher: $LAUNCHER"
echo "Targets repository: $TARGET"
echo
echo "Try:"
echo "  gdis ./models/*"
echo "  gdis --gtk4 examples/methane.gin"
echo
if [[ ":$PATH:" != *":$BIN_DIR:"* ]]; then
  echo "Your PATH does not include $BIN_DIR yet."
  echo "Add this line to ~/.bashrc and open a new shell:"
  echo "  export PATH=\"$BIN_DIR:\$PATH\""
fi
