# GDIS Renderer Audit

This note records the concrete OpenGL compatibility blockers that showed up
while testing the GTK3 and GTK4 renewal branches on a machine that provides a
modern core-profile context through `GtkGLArea`.

The short version is simple:

- GTK3 and GTK4 now build here
- both branches launch here
- both branches receive a non-legacy context at runtime
- GTK4 now shows atom views again through a limited core-profile renderer
- the renderer is still mostly written around fixed-function OpenGL
- therefore the next renewal step is still renderer work, not more blind
  widget work

## Runtime Reality

Observed during runtime testing with `GDIS_DEBUG_GL=1`:

- GTK3: `GDIS GL context: 4.5 legacy=0 es=0`
- GTK4: `GDIS GL context: 4.5 legacy=0 es=0`

That means the modern branches are no longer failing at first paint, but they
are still constrained by the large amount of rendering code that expects APIs
not available in a core-profile context.

## Main Compatibility Blockers

### 1. Matrix stack and GLU projection math

Examples:

- [`src/gl_main.c`](/home/hym/Desktop/gdislinux/src/gl_main.c)
  uses `glMatrixMode()`, `glLoadIdentity()`, `gluLookAt()`,
  `gluPerspective()`, `glOrtho()`, `glGetDoublev()`, `glGetIntegerv()`,
  `gluProject()`, and `gluUnProject()`.
- [`src/gl_stereo.c`](/home/hym/Desktop/gdislinux/src/gl_stereo.c)
  uses `glMatrixMode()`, `glLoadIdentity()`, `glFrustum()`, and
  `glTranslatef()`.

This ties camera setup, projection setup, and picking math directly to the
 fixed-function matrix stack.

### 2. Fixed-function lighting and material state

Examples in [`src/gl_main.c`](/home/hym/Desktop/gdislinux/src/gl_main.c):

- `glShadeModel()`
- `glLightModeli()`
- `glMaterialf()` and `glMaterialfv()`
- `glColorMaterial()`
- `glFogf()` and `glFogfv()`

These calls assume the old OpenGL lighting pipeline. In a core-profile path,
lighting needs to move into shader code and explicit uniforms.

### 3. Immediate-mode geometry

Heavy `glBegin()` / `glEnd()` usage remains in:

- [`src/gl_main.c`](/home/hym/Desktop/gdislinux/src/gl_main.c)
- [`src/gl_primitives.c`](/home/hym/Desktop/gdislinux/src/gl_primitives.c)
- [`src/gl_graph.c`](/home/hym/Desktop/gdislinux/src/gl_graph.c)

Common patterns include:

- `glBegin(GL_LINES)`
- `glBegin(GL_LINE_STRIP)`
- `glBegin(GL_POLYGON)`
- `glBegin(GL_QUADS)`
- `glBegin(GL_TRIANGLE_FAN)`
- per-vertex `glVertex*()` and `glNormal*()`

Those APIs are removed from core profile.

### 4. Client-side vertex arrays

Examples:

- [`src/gl_main.c`](/home/hym/Desktop/gdislinux/src/gl_main.c)
  enables `GL_VERTEX_ARRAY` and `GL_NORMAL_ARRAY`.
- [`src/gl_varray.c`](/home/hym/Desktop/gdislinux/src/gl_varray.c)
  uses `glVertexPointer()`, `glNormalPointer()`, and `glDrawElements()`
  against client memory.

Indexed drawing is the right general direction, but core profile requires VBOs
and VAOs rather than client-side arrays.

### 5. Bitmap/raster text assumptions

[`src/gl_main.c`](/home/hym/Desktop/gdislinux/src/gl_main.c) still uses
`glBitmap()`. That is another fixed-function-era dependency that should be
treated as overlay work for a later stage.

## First Migration Slice Status

The first slice that is now in place is:

- keep the GTK2 renderer as the stable path
- add a minimal core-profile renderer path for atom spheres first
- build that path on top of the existing sphere mesh logic in
  [`src/gl_varray.c`](/home/hym/Desktop/gdislinux/src/gl_varray.c)

Why this slice is the least risky one that still matters:

- `gl_varray.c` already centralizes reusable indexed sphere geometry
- atoms are the first thing users expect to see when opening a model
- getting spheres visible turns the previous blank/blocked canvas into a usable
  renewal milestone
- this can be done without porting every overlay, graph, label, axis, and bond
  path at the same time

Current limitations of that slice:

- bonds are still not rendered in the core path
- labels and bitmap text are still legacy-only
- graphs and overlay elements still assume legacy rendering
- GTK4 still relies on a compact compatibility menu shim rather than a native
  final menu implementation

## Recommended Order

1. Move camera and projection math to CPU-side matrices stored in GDIS data
   structures instead of querying GL state back with `glGetDoublev()`.
2. Introduce a tiny shader program plus VBO/VAO-backed sphere drawing for the
   `gl_varray.c` path.
3. Gate that code behind the existing runtime context detection so legacy
   contexts keep using the old renderer unchanged.
4. After spheres work, migrate cylinders/pipes for bonds.
5. Then migrate line overlays, axes, cell drawing, and finally text/labels.

## What Not To Do First

Do not start by porting every GTK widget to GTK4 while the renderer still
depends on:

- `glBegin()` / `glEnd()`
- fixed-function lighting
- matrix stack state
- client-side arrays

That work no longer changes a blank-canvas outcome directly, because the atom
renderer is already in place. The next visible improvement is bond and overlay
parity.
