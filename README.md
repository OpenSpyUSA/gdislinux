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

- The codebase currently targets `gtk+2` and `gtkglext`.
- It builds successfully on Ubuntu 24.04 with the legacy development packages
  installed.
- The project should be treated as a maintenance fork candidate because the GUI
  stack depends on aging toolkit packages that are increasingly fragile on
  modern distributions.
- The repository now includes local helper scripts for rebuilding, example
  launching, and optional tool verification.
- A staged modernization plan lives in
  [`docs/modernization-roadmap.md`](docs/modernization-roadmap.md).
- A practical GitHub publishing guide lives in
  [`docs/github-fork-guide.md`](docs/github-fork-guide.md).

### Quick Start

For a local build into `bin/gdis`:

```bash
./rebuild-ubuntu.sh
```

To override the pkg-config module set for an experimental backend branch:

```bash
GDIS_GUI_PKG_CONFIG_PACKAGES="gtk+-2.0 gthread-2.0 gmodule-2.0 gtkglext-1.0" ./rebuild-ubuntu.sh
```

To launch the main application:

```bash
./bin/gdis
```

To launch a curated example:

```bash
./run-example.sh methane
```

### Build Requirements

Today, the practical Linux build requirements are:

- `build-essential`
- `perl`
- `pkg-config`
- `libgtk2.0-dev`
- `libgtkglext1-dev`

On Ubuntu 24.04:

```bash
sudo apt-get update
sudo apt-get install -y build-essential pkg-config libgtk2.0-dev libgtkglext1-dev
./rebuild-ubuntu.sh
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
- `./run-example.sh <name>`
  Opens one of the curated examples from [`examples/`](examples/).
- `./verify-optional-tools.sh`
  Checks optional helpers such as POV-Ray, ImageMagick, Open Babel, FFmpeg, and
  xmgrace.
- `./install-optional-ubuntu.sh`
  Installs the open-source optional helper tools available on Ubuntu.
- `./smoke-test-examples.sh`
  Launches curated examples under a timeout and flags obvious startup/runtime
  failures. Uses `xvfb-run` automatically if no display is present.

### Maintenance Notes

- The most important technical debt is the legacy GUI stack:
  `gtk+2`, `gtkglext`, and many deprecated GTK APIs.
- The first maintenance priority is repository hygiene and repeatable builds.
- The second priority is isolating GUI and OpenGL integration points so a later
  GTK3/GTK4 migration is realistic.
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

- POV-Ray
- ImageMagick
- Open Babel
- FFmpeg
- xmgrace

GDIS also contains input/output integration points for codes such as GULP,
GAMESS, SIESTA, Monty, VASP, and USPEX, but availability depends on those
external engines being installed and licensed where applicable.
