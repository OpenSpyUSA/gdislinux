# GDIS Modernization Roadmap

This document is intentionally pragmatic. The codebase is large, old, and still
useful. The right way to modernize it is to reduce risk in stages, not to
declare an all-at-once rewrite.

## Why GDIS Needs Maintenance

The main pressure points are visible already:

- the Linux build depends on `gtk+2`
- GUI OpenGL integration depends on `gtkglext`
- many source files explicitly suppress GTK/GLib deprecation warnings
- the repository still contains generated build outputs from local builds
- contributor onboarding was previously too dependent on tribal knowledge

These are manageable problems, but they need ordering.

## Current Status

Completed baseline maintenance work in this repository:

- Ubuntu 24.04 rebuild helper and optional-tool helpers are in place
- GitHub Actions compile validation exists
- build scripts now support `PKG_CONFIG`, `GDIS_EXTRA_CFLAGS`,
  `GDIS_EXTRA_INCS`, and `GDIS_EXTRA_LIBS`
- GUI pkg-config module lists can now be overridden with
  `GDIS_BASE_PKG_CONFIG_PACKAGES` and `GDIS_GUI_PKG_CONFIG_PACKAGES`
- direct live `gtkglext` calls in the application code are centralized in
  `src/gui_gl.c`
- source-level build targets now exist for `gtk+2` and `gtk4`
- GTK3 build targeting has been removed to reduce maintenance surface
- GTK4 and GTK2 are the only maintained toolkit targets in this tree
- the GTK4 branch survives non-legacy `GtkGLArea` contexts by switching to a
  limited core renderer that already restores atom spheres,
  bond cylinders, stick geometry, and visible atom selection
- the GTK4 branch now also uses a compact compatibility menu layer, which
  avoids the earlier giant off-screen window layout and makes the molecule
  canvas visible again
- the stable runtime path is still GTK2 because labels, graphs, bitmap text,
  and many fixed-function drawing features are not yet ported

That means the project is still on GTK2 and `gtkglext`, but the dependency is
now behind a narrower seam and the build metadata is less scattered.

## Phase 0: Keep It Alive

Goal: maintain a known-good baseline for users and contributors.

Tasks:

- keep Ubuntu 24.04 builds working
- document current dependencies honestly
- add CI for compile validation
- keep helper scripts for examples and optional tools working
- stop treating generated local build outputs as source artifacts

Exit criteria:

- a fresh contributor can build `bin/gdis`
- CI can rebuild the application automatically
- README and contributor docs reflect reality

Progress so far:

- effectively complete for the current Linux baseline

## Phase 1: Build and Repo Hygiene

Goal: make the project easier to maintain without changing architecture yet.

Tasks:

- remove tracked generated objects and dependency files from version control
- make the build scripts less dependent on hardcoded command names
- allow environment-driven compiler and linker overrides
- centralize dependency checks so future toolkit migration touches fewer places
- document supported and unsupported build paths

Suggested outputs:

- cleaned repository index
- reproducible Linux build workflow
- GitHub Actions build job
- contributor checklist

## Phase 2: Isolate Legacy GUI Boundaries

Goal: reduce the blast radius of a future toolkit migration.

Hotspots worth isolating:

- `src/gui_main.c`
- `src/gui_canvas.c`
- `src/gui_shorts.c`
- `src/gui_dialog.c`
- `src/gui_render.c`
- any direct `gtkgl` integration

Tasks:

- identify the smallest possible UI abstraction seams
- isolate OpenGL-context setup from the rest of the rendering code
- separate model logic from GTK widget creation where practical
- document which widgets are using old GTK 2-only APIs

Exit criteria:

- GUI construction and core model logic are less entangled
- OpenGL setup is easier to swap behind a narrower interface

Progress so far:

- OpenGL widget/context setup is now routed through `src/gui_gl.c`
- the remaining GTK2 migration work is broader widget and dialog cleanup, not
  scattered GL setup calls

## Phase 3: Replace `gtkglext`

Goal: remove the dependency that blocks many modern distributions.

Possible directions:

- GTK4 plus `GtkGLArea`
- a different maintained OpenGL area binding
- a custom compatibility layer if absolutely necessary

Recommended order:

- focus directly on GTK4 as the modern toolkit target
- use runtime context checks to keep the GTK2 path stable while renderer
  migration continues
- prioritize renderer work over widget-only migration when core-profile context
  constraints appear

This phase should not start until Phase 2 has reduced the number of direct
touchpoints.

Risks:

- rendering regressions
- event-handling regressions
- platform-specific GL context behavior

Current concrete blocker surface:

- fixed-function matrix stack and GLU projection math in
  `src/gl_main.c` and `src/gl_stereo.c`
- immediate-mode drawing in `src/gl_main.c`, `src/gl_primitives.c`, and
  `src/gl_graph.c`
- client-side vertex arrays in `src/gl_varray.c`
- fixed-function lighting/material state in `src/gl_main.c`

Recommended first renderer slice:

- keep GTK2 as the stable runtime path
- use `src/gl_varray.c` as the first core-profile migration seam
- get atom spheres visible with CPU-side matrices plus a minimal shader/VBO
  path before attempting full bond, overlay, text, or widget parity

Progress on that slice:

- complete enough to show atom views again on GTK4
- still incomplete for bond cylinders, overlays, labels, graphs, and text

See also:

- [`docs/renderer-audit.md`](renderer-audit.md)

## Phase 4: Toolkit Migration Beyond GTK2

Goal: move off GTK2 and deprecated widgets.

This is the largest change and should be treated as an incremental port, not a
single giant branch.

Likely tasks:

- replace removed GTK 2 widgets and factory APIs
- update signal wiring and model/view code
- update text, tree, menu, and combo-box usage
- retest core workflows:
  - file loading
  - model editing
  - measurements
  - periodic images
  - symmetry
  - animation

## Testing Priorities During Any Migration

The following workflows are good smoke tests and should remain easy to run:

- `./run-example.sh methane`
- `./run-example.sh water`
- `./run-example.sh adp1`
- `./run-example.sh deoxy`
- `./run-example.sh vasp`

Feature checks:

- view rotation, zoom, and pan
- atom selection modes
- model editing and bond editing
- measurements
- Z-matrix generation and geometry recompute
- periodic images and supercell generation
- symmetry inspection
- animation dialog opening on multiframe data

## What Not To Do First

Avoid starting with:

- a GTK4 rewrite branch
- long stretches of GTK4 signal or container work without solving the renderer
  compatibility problem first
- a full rendering rewrite
- cross-platform packaging for every OS
- broad style-only code churn

Those changes can happen later, but they are not the fastest path to a healthy
project.
