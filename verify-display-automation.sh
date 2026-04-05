#!/usr/bin/env bash
set -eu

status_ok=0

have() {
  command -v "$1" >/dev/null 2>&1
}

show_path() {
  if have "$1"; then
    printf '%-20s %s\n' "$1" "$(command -v "$1")"
  else
    printf '%-20s %s\n' "$1" "missing"
    status_ok=1
  fi
}

printf 'Display automation check\n\n'
printf '%-20s %s\n' "DISPLAY" "${DISPLAY-}"
printf '%-20s %s\n' "WAYLAND_DISPLAY" "${WAYLAND_DISPLAY-}"
printf '%-20s %s\n' "XDG_SESSION_TYPE" "${XDG_SESSION_TYPE-}"
printf '\n'

show_path xdotool
show_path wmctrl
show_path xwininfo
show_path import
show_path python3
printf '\n'

if have gsettings; then
  printf '%-20s %s\n' "toolkit-access" \
    "$(gsettings get org.gnome.desktop.interface toolkit-accessibility 2>/dev/null || printf 'unknown')"
else
  printf '%-20s %s\n' "toolkit-access" "gsettings missing"
  status_ok=1
fi

if have xdotool; then
  printf '%-20s ' "mouse"
  if xdotool getmouselocation >/dev/null 2>&1; then
    xdotool getmouselocation
  else
    printf 'unavailable\n'
    status_ok=1
  fi
fi

if have python3; then
  printf '%-20s ' "pyatspi"
  if python3 - <<'PY' >/dev/null 2>&1
import pyatspi
PY
  then
    printf 'ok\n'
  else
    printf 'missing or broken\n'
    status_ok=1
  fi
fi

printf '\n'
if [ "$status_ok" -eq 0 ]; then
  printf 'Display automation is available.\n'
else
  printf 'Display automation is only partially available.\n'
fi

exit "$status_ok"
