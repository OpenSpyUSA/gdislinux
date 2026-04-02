#!/usr/bin/env bash
set -euo pipefail

packages=(
  povray
  imagemagick
  openbabel
  ffmpeg
  grace
  libcanberra-gtk-module
)

echo "Installing open-source optional GDIS helpers on Ubuntu:"
printf '  %s\n' "${packages[@]}"
echo
echo "This installs rendering, conversion, plotting, movie, and GTK sound-module helpers."
echo "It does not install licensed or vendor-distributed chemistry engines such as VASP or USPEX."
echo

sudo apt-get update
sudo apt-get install -y "${packages[@]}"

cat <<'EOF'

Done.

Next steps:
  1. Re-run ./verify-optional-tools.sh
  2. Launch GDIS and check Executable locations in the Setup dialog

Notes:
  - Open Babel installs `obabel`; if GDIS expects `babel`, point the Babel path to `/usr/bin/obabel`.
  - POVRay and ImageMagick should work once installed.
  - FFmpeg and Grace are useful helpers, but may not appear as explicit paths in the GDIS setup dialog.
EOF
