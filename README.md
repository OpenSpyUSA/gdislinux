## GDIS

GDIS is a GTK/OpenGL desktop application for building, visualizing, and
analyzing atomistic and crystallographic structures.

This repository is currently being treated as a community-maintained revival of
the upstream codebase. The immediate goal is not a full rewrite. The immediate
goal is to keep GDIS buildable, testable, and gradually modernized while the
larger GTK/OpenGL migration work is planned properly.

GDIS comes with ABSOLUTELY NO WARRANTY.

This is free software. You are welcome to redistribute copies provided the
conditions of the Version 2 GPL (GNU Public License) are met.

Original authorship remains credited to Sean Fleming and Andrew Rohl.

### Current Status

- The default runtime and build target is GTK4.
- GTK2 remains available as a legacy fallback target for compatibility testing.
- The GTK4 renewal branch now builds and launches with a compact compatibility
  menu layer and the same limited atom renderer, so the molecule view is
  visible again instead of blank.
- The project should be treated as a maintenance fork candidate because the GUI
  stack depends on aging toolkit packages that are increasingly fragile on
  modern distributions.
- The repository now includes local helper scripts for rebuilding, example
  launching, and optional tool verification.
- A staged modernization plan lives in
  [`docs/modernization-roadmap.md`](docs/modernization-roadmap.md).
- A renderer-specific blocker audit lives in
  [`docs/renderer-audit.md`](docs/renderer-audit.md).
- A practical GitHub publishing guide lives in
  [`docs/github-fork-guide.md`](docs/github-fork-guide.md).

### Quick Start

Preferred path: Ubuntu 24.04 with the GTK4 build target.

If you want the packaged route, download the matching `gdislinux`,
`gdislinux-qbox`, and `gdislinux-qbox-data` `.deb` files from GitHub Releases
for your architecture (`amd64` or `arm64`), then install them together:

```bash
ARCH="$(dpkg --print-architecture)"
UBUNTU_TAG="ubuntu24.04.1"

sudo apt install \
  ./gdislinux_*"$UBUNTU_TAG"_"$ARCH".deb \
  ./gdislinux-qbox_*"$UBUNTU_TAG"_"$ARCH".deb \
  ./gdislinux-qbox-data_*"$UBUNTU_TAG"_all.deb

gdislinux /usr/share/gdislinux/examples/methane.gin
gdislinux /usr/share/gdislinux/qbox/examples/qbox_methane.qbox
```

Use the Ubuntu tag that matches your system, for example `ubuntu22.04.1` or
`ubuntu24.04.1`. Do not mix both release targets in one `apt install` command.
The main package now ships the full repository `models/` library under
`/usr/share/gdislinux/examples`.
The packaged `qbox_methane.qbox` example is rewritten to use the packaged XML
potentials under `/usr/share/gdislinux/qbox/potentials`, so it is intended to
work directly after installing `gdislinux-qbox-data`.

If you want the source build route instead:

```bash
git clone https://github.com/OpenSpyUSA/gdislinux.git
cd gdislinux

sudo apt-get update
sudo apt-get install -y \
  build-essential \
  perl \
  pkg-config \
  libgtk-4-dev \
  libgl1-mesa-dev \
  libglu1-mesa-dev \
  libepoxy-dev \
  xvfb \
  ripgrep

GDIS_GTK_TARGET=gtk4 ./rebuild-ubuntu.sh
./run-gdis.sh ./models/*
```

If you also want the local Qbox integration on Ubuntu 24.04:

```bash
cd gdislinux

sudo apt-get install -y \
  libxerces-c-dev \
  libopenmpi-dev \
  openmpi-bin \
  libfftw3-dev \
  libscalapack-openmpi-dev \
  libblas-dev \
  liblapack-dev \
  uuid-dev \
  g++ \
  make \
  git \
  pkg-config

./install-qbox-local.sh
```

For an existing clone that just needs the latest code:

```bash
cd gdislinux
git pull --ff-only
GDIS_GTK_TARGET=gtk4 ./rebuild-ubuntu.sh
./run-gdis.sh ./models/*
```

Ubuntu 22.04 can also work, but its GTK4 stack is older. Pull the latest
compatibility fixes before rebuilding.

For a local rebuild into `bin/gdis`:

```bash
./rebuild-ubuntu.sh
```

Each rebuild also saves a target-specific executable such as `bin/gdis-gtk4`
and `bin/gdis-gtk2`. Plain `./rebuild-ubuntu.sh` now defaults to GTK4.

To launch the main application:

```bash
./run-gdis.sh
```

To install a user-level `gdis` command (recommended):

```bash
./install-user-launcher.sh
gdis ./models/*
```

If `gdis` is not found after installation, add this once:

```bash
echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc
```

To launch a curated example:

```bash
./run-example.sh methane
```

To launch a real USPEX results bundle:

```bash
./run-uspex-output.sh /path/to/results-dir
./run-uspex-output.sh /path/to/results-dir/OUTPUT.txt
```

To launch an example with a specific target build:

```bash
./run-example.sh --gtk4 methane
```

To run the legacy GTK2 fallback explicitly:

```bash
./run-gdis.sh --gtk2
./run-example.sh --gtk2 methane
```

### Build Requirements

Today, the practical Linux build requirements for the default GTK4 path are:

- `build-essential`
- `perl`
- `pkg-config`
- `libgtk-4-dev`
- `libgl1-mesa-dev`
- `libglu1-mesa-dev`
- `libepoxy-dev`

On Ubuntu 24.04:

```bash
sudo apt-get update
sudo apt-get install -y \
  build-essential \
  perl \
  pkg-config \
  libgtk-4-dev \
  libgl1-mesa-dev \
  libglu1-mesa-dev \
  libepoxy-dev \
  xvfb \
  ripgrep
GDIS_GTK_TARGET=gtk4 ./rebuild-ubuntu.sh
```

For the optional GTK2 fallback path:

```bash
sudo apt-get update
sudo apt-get install -y build-essential pkg-config libgtk2.0-dev libgtkglext1-dev
GDIS_GTK_TARGET=gtk2 ./rebuild-ubuntu.sh
```

For the Meson/Ninja path on GTK4:

```bash
sudo apt-get update
sudo apt-get install -y build-essential pkg-config meson ninja-build libgtk-4-dev libgl1-mesa-dev libglu1-mesa-dev libepoxy-dev
./rebuild-meson.sh --gtk4
```

If you prefer the original installer flow:

```bash
./install
```

For a non-interactive local build into `bin/gdis`:

```bash
./install default
```

Advanced build overrides:

- `PKG_CONFIG=/path/to/pkg-config`
- `GDIS_BASE_PKG_CONFIG_PACKAGES="..."`
- `GDIS_GUI_PKG_CONFIG_PACKAGES="..."`

### Helper Scripts

- `./rebuild-ubuntu.sh`
  Rebuilds `bin/gdis` non-interactively after checking the required packages.
- `./rebuild-meson.sh`
  Configures a Meson build directory, compiles with Ninja, and stages the
  selected target back into `bin/gdis` plus `bin/gdis-<target>`.
- `./run-example.sh <name>`
  Opens one of the curated examples from [`examples/`](examples/).
- `./run-example.sh --gtk4 <name>`
  Uses a target-specific executable such as `bin/gdis-gtk4` when present.
- `./run-gdis.sh`
  Launches GDIS directly and defaults to GTK4.
- `./install-user-launcher.sh`
  Installs a user-level `gdis` command in `~/.local/bin` so you can launch
  from anywhere (for example: `gdis ./models/*` inside this repository).
- `./run-uspex-output.sh <results-dir-or-output.txt>`
  Validates a USPEX results folder and launches GDIS on its `OUTPUT.txt` file.
  Defaults to GTK4 and accepts `--gtk2` as an explicit fallback.
- `./audit-core-gl.sh`
  Reports the fixed-function OpenGL patterns that still block a core-profile
  GTK4 renderer path.
- `./verify-optional-tools.sh`
  Checks optional helpers such as Qbox, POV-Ray, ImageMagick, Open Babel,
  FFmpeg, and xmgrace.
- `./verify-gtk4-building-tools.sh`
  Opens the renewed GTK4 Building dialogs under X11/XWayland and checks that
  the key editor windows appear without critical runtime warnings.
- `./verify-pick-alignment.sh`
  Launches GDIS under X11/XWayland, detects the visible methane geometry from
  a screenshot, clicks it, and checks the debug log to confirm cursor/pick
  alignment for GTK4 and/or GTK2.
- `./install-optional-ubuntu.sh`
  Installs the open-source optional helper tools available on Ubuntu.
- `./install-qbox-local.sh`
  Clones upstream Qbox, builds it locally in `.localdeps/`, and creates
  `bin/qbox` plus `bin/qb` convenience links for GDIS.
- `./run-qbox-roundtrip.sh`
  Runs a minimal methane Qbox job with the bundled local C/H pseudopotentials,
  saves `tmp/qbox-roundtrip/methane.xml`, and can optionally reopen it in GDIS.
- `./smoke-test-examples.sh`
  Launches curated examples under a timeout and flags obvious startup/runtime
  failures. Uses `xvfb-run` automatically if no display is present and accepts
  `--gtk4` by default and `--gtk2` as a fallback.

### Maintenance Notes

- The most important technical debt is the legacy GUI stack:
  `gtk+2`, `gtkglext`, and many deprecated GTK APIs.
- GTK4 now survives modern non-legacy GL contexts by switching to a limited
  core renderer for atom views.
- GTK4 also now uses a compact compatibility menu layer so the main window no
  longer stretches far off-screen before the canvas is visible.
- The modern renderer is still incomplete: atom spheres, bond cylinders, stick
  geometry, and selection overlays now work in GTK4, but labels, graphs,
  bitmap text, and many fixed-function-era drawing paths still need migration.
- The next realistic renewal target is still renderer modernization, with text,
  labels, graph canvases, and other overlay-era drawing as the next useful
  slices after restored picking and bond geometry.
- The first maintenance priority is repository hygiene and repeatable builds.
- The second priority is isolating GUI and OpenGL integration points so a later
  GTK4 migration is realistic.
- See [`CONTRIBUTING.md`](CONTRIBUTING.md) for development workflow guidance.
- If you want to turn this local tree into your own public fork, start with
  [`docs/github-fork-guide.md`](docs/github-fork-guide.md).

### Platform Notes

If you are compiling on macOS, the historical recommendation was MacPorts for
GTK and `gtkglext`. That advice is preserved here for reference, but it should
be treated as legacy and unverified until someone actively tests and documents a
current macOS path.

### External Projects

Some GDIS functionality depends on external projects and scientific codes.

- CDD for halfspace intersection and morphology display
- GPeriodic for periodic table display/editing
- SgInfo for space-group support
- The brute-force symmetry analyzer by S. Pachkovsky

Optional helpers that improve the GDIS workflow include:

- Qbox
- POV-Ray
- ImageMagick
- Open Babel
- FFmpeg
- xmgrace

GDIS also contains input/output integration points for codes such as GULP,
GAMESS, SIESTA, Monty, VASP, Qbox, and USPEX, but availability depends on
those external engines being installed and licensed where applicable.

### Qbox Notes

This tree now includes first-pass Qbox integration in GDIS:

- Qbox input files: `*.qbox`, `*.qboxin`, `*.qb`
- Qbox restart/output files: `*.r`, `*.qboxr`, and Qbox-style XML detected by
  content sniffing
- Setup dialog field: `Qbox`
- Expert pass-through tab in `Tools > Computation > Qbox...`:
  - preset profiles: `SCF`, `Band`, `GeoOpt`, `FrozenPhonon`, `HOMO-LUMO`
    (auto-fill fields; review/edit before run)
  - optional model export block (`cell/species/atom`) on/off
  - optional default settings block on/off
  - free-form pre/post command blocks
  - optional include command file line (for scripts such as `moves.i`)
  - auto-save/auto-quit toggles

This lets you drive advanced Qbox commands directly (for example `kpoint`,
`set scf_tol`, `set nempty`, `compute_mlwf`, `response`, `spectrum`, `move`,
custom `run` schedules) without waiting for one-by-one hardcoded widgets.

Two smoke-test samples are included:

- `models/qbox_methane.qbox`
- `models/qbox_methane.xml`
- `models/qbox_expert_demo.i` (advanced command include template)

For source-tree use, `models/qbox_methane.qbox` stays generic on purpose and
may need either `Tools > Computation > Qbox > Use Demo Potentials` or manual
species XML filename updates for your local Qbox pseudopotential set. The
packaged `/usr/share/gdislinux/qbox/examples/qbox_methane.qbox` copy is
rewritten during Debian packaging to point at the packaged XML demo files.

After that, a working local smoke round-trip is:

```bash
./run-qbox-roundtrip.sh
./run-qbox-roundtrip.sh --open --gtk4
```

For a fresh clone, GDIS now auto-detects Qbox from the usual local and system
locations in this order:

- configured `qbox_path`
- packaged `/usr/lib/gdislinux/qbox`
- repo/local `bin/qbox`
- repo/local `bin/qb`
- system `qbox`
- system `qb`

That means a user who installs `gdislinux-qbox` or has already run
`./install-qbox-local.sh` should not need to fill in
`View > Executable paths > Qbox` manually.

The Qbox writer emits `species` lines that point to per-element XML
pseudopotentials. In the GUI, `Use Demo Potentials` now auto-resolves files
from the packaged demo directory `/usr/share/gdislinux/qbox/potentials`,
bundled legacy demos, and `external/pseudos/qbox-xml-oncv-sr` (when present).
For generation and lookup details, see `docs/qbox-potentials.md`.

### USPEX Notes

The USPEX viewer path expects an actual USPEX results directory, not just an
input file. From the current source, the loader is wired around `OUTPUT.txt`
and then reads sibling files from the same folder such as:

- `Parameters.txt`
- `Individuals`
- `gatheredPOSCARS` or `gatheredPOSCARS_relaxed`

This repository currently includes USPEX input-side examples under
`models/INPUT.txt` and `models/Specific/`, but it does not ship a full USPEX
results example. That means a command shown by another user such as:

```bash
./run-gdis.sh --gtk2 /usr/local/USPEX-10.3/application/archive/examples/EX01/reference/OUTPUT.txt
```

only works when that separate USPEX installation exists on disk. On your
machine, use the path to your own USPEX results folder instead, for example:

```bash
./run-uspex-output.sh --gtk2 ~/somewhere/USPEX-job/reference
```
