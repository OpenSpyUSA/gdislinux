# GDISLinux 1.00+git20260417-1

This release tightens the renewed GTK4 workflow around two areas that still
matter most in day-to-day use: interactive building dialogs and reliable atom
selection.

## Highlights

- Building tools are more practical under GTK4.
  The Model Editing, Z-matrix, Surface, Docking, Dislocation, and MD
  initializer dialogs were cleaned up so they open more consistently, size
  more sensibly, and expose the controls that were previously awkward or
  broken in the renewal path.
- Cursor-to-selection alignment is now checked against the actual canvas.
  The pick/unproject/project path now uses canvas-local widget coordinates
  instead of mixing in full-window Y inversion assumptions.
- GTK4 regressions are easier to catch before release.
  The tree now includes scripted checks for the Building dialogs and for
  click-to-pick alignment under X11/XWayland automation.

## Included Changes

- Fix selection math and selection-box overlay rendering in the core-profile
  GTK4 renderer.
- Keep `GtkGLArea` sizing tied to widget dimensions used by the input path.
- Improve docking project lifecycle handling and preserve restored atom/shell
  region assignments.
- Stop certain Building dialogs from being blocked by stale singleton dialog
  handling.
- Add clearer user-facing errors for unsupported Dislocation builder input.
- Limit the MD initializer to supported model counts and warn when extra
  models are ignored.
- Replace the poor GTK4 rendering of the legacy arrow button icon with a
  symbolic icon in that code path.
- Add `verify-gtk4-building-tools.sh` and `verify-pick-alignment.sh`.

## Validation

Validated locally in this tree with:

```bash
GDIS_GTK_TARGET=gtk4 ./rebuild-ubuntu.sh
GDIS_GTK_TARGET=gtk2 ./rebuild-ubuntu.sh
./verify-gtk4-building-tools.sh
./verify-pick-alignment.sh
```

The pick-alignment verification confirmed visible-atom hits in both GTK4 and
GTK2 before and after a window resize.

## Suggested Tag

`v1.00-git20260417-1`
