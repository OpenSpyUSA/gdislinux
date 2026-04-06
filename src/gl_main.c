/*
Copyright (C) 2003 by Sean David Fleming

sean@ivec.org

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

The GNU GPL can also be found at http://www.gnu.org
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#define G_DISABLE_DEPRECATED
#define GDK_DISABLE_DEPRECATED
#define GDK_PIXBUF_DISABLE_DEPRECATED
#define GTK_DISABLE_DEPRECATED
#define GLIB_DISABLE_DEPRECATION_WARNINGS
#include <gtk/gtk.h>
#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#if GTK_MAJOR_VERSION >= 3
#include <epoxy/gl.h>
#else
#include <GL/gl.h>
#endif
#include <GL/glu.h>
#endif
#ifdef CAIRO_HAS_PS_SURFACE
#include <cairo-ps.h>
#endif


#include "gdis.h"
#include "coords.h"
#include "edit.h"
#include "geometry.h"
#include "graph.h"
#include "matrix.h"
#include "molsurf.h"
#include "morph.h"
#include "model.h"
#include "spatial.h"
#include "zone.h"
#include "opengl.h"
#include "render.h"
#include "select.h"
#include "surface.h"
#include "numeric.h"
#include "measure.h"
#include "quaternion.h"
#include "gui_gl.h"
#include "interface.h"
#include "dialog.h"
#include "gl_varray.h"

/* externals */
extern struct sysenv_pak sysenv;
extern struct elem_pak elements[];

#define DRAW_PICTURE 0

/* transformation/projection matrices */
/*
GLint viewport[4];
GLdouble mvmatrix[16], projmatrix[16];
*/

gdouble gl_acm[9];
gdouble halo_fade[16];
gint halo_segments=16;
gpointer gl_font;
gint gl_fontsize=10;

#if GTK_MAJOR_VERSION >= 3
struct gl_core_renderer_pak
{
guint program;
guint line_program;
guint surface_program;
guint overlay_program;
guint vao;
guint vbo;
guint ebo;
guint cylinder_vao;
guint cylinder_vbo;
guint cylinder_ebo;
guint line_vao;
guint line_vbo;
guint surface_vao;
guint surface_vbo;
guint overlay_vao;
guint overlay_vbo;
guint overlay_texture;
gint u_mvp;
gint u_colour;
gint u_line_mvp;
gint u_line_colour;
gint u_surface_mvp;
gint u_surface_alpha;
gint u_overlay_texture;
gint sphere_quality;
gint cylinder_quality;
gint overlay_width;
gint overlay_height;
gpointer sphere_mesh;
gpointer cylinder_mesh;
gboolean warned_limited;
gboolean warned_graph;
gboolean warned_snapshot;
};

static struct gl_core_renderer_pak gl_core_renderer = {0};
#endif

static gboolean gl_core_refresh(void);
#if GTK_MAJOR_VERSION >= 3
static void gl_core_draw_colour_scale_overlay(struct canvas_pak *canvas,
                                              gint x,
                                              gint y,
                                              struct model_pak *data);
#else
static inline void gl_core_draw_colour_scale_overlay(struct canvas_pak *canvas,
                                                     gint x,
                                                     gint y,
                                                     struct model_pak *data)
{
(void) canvas;
(void) x;
(void) y;
(void) data;
}
#endif

/***********************************/
/* world to canvas size conversion */
/***********************************/
gdouble gl_pixel_offset(gdouble r, struct canvas_pak *canvas)
{
gint p[2];
gdouble x[3];

VEC3SET(x, r, 0.0, 0.0);
gl_unproject(p, x, canvas);

return(sqrt(p[0]*p[0]+p[1]*p[1]));
}

/*****************************************************/
/* is a given normal aligned with the viewing vector */
/*****************************************************/
gint gl_visible(gdouble *n, struct model_pak *model)
{
gdouble v[3];
struct camera_pak *camera;

g_assert(model != NULL);

camera = model->camera;

ARR3SET(v, camera->v);
if (camera->mode == LOCKED)
  quat_rotate(v, camera->q);

if (vector_angle(v, n, 3) < 0.5*G_PI)
  return(FALSE);

return(TRUE);
}

/****************************************************/
/* set up the visual for subsequent canvas creation */
/****************************************************/
gint gl_init_visual(void)
{
/* attempt to get best visual */
/* order: stereo, double buffered, depth buffered */
/* CURRENT - stereo-capable visuals remain optional as they are slower even
 * when windowed stereo is not active. */

	sysenv.glconfig = gdis_gl_config_new(TRUE);

/* windowed stereo possible? */
sysenv.render.stereo_use_frustum = TRUE;
if (sysenv.glconfig)
  {
  sysenv.stereo_windowed = TRUE;
  sysenv.render.stereo_quadbuffer = TRUE;
  }
else
  {
	  sysenv.glconfig = gdis_gl_config_new(FALSE);
  sysenv.stereo_windowed = FALSE;
  sysenv.render.stereo_quadbuffer = FALSE;
  }

if (!sysenv.glconfig)
  {
  printf("WARNING: cannot create a double-buffered visual.\n");
	  sysenv.glconfig = gdis_gl_config_new_basic();
  if (!sysenv.glconfig)
    {
    printf("ERROR: no appropriate visual could be acquired.\n");
    return(1);
    }
  }
return(0);
}

/*****************************************/
/* set up the camera and projection mode */
/*****************************************/
#define DEBUG_INIT_PROJ 0
void gl_init_projection(struct canvas_pak *canvas, struct model_pak *model)
{
gdouble r, a;
//gdouble pix2ang;/*FIX: 533acf*/
gdouble x[3], o[3], v[3];
struct camera_pak *camera;

g_assert(canvas != NULL);

/* set up matrices even if no model in the current canvas */
glViewport(canvas->x, canvas->y, canvas->width, canvas->height);
if (!model)
  {
  if (canvas)
    {
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glGetIntegerv(GL_VIEWPORT, canvas->viewport);
    glGetDoublev(GL_MODELVIEW_MATRIX, canvas->modelview);
    glGetDoublev(GL_PROJECTION_MATRIX, canvas->projection);
    }
  return;
  }

g_assert(model->camera != NULL);
/* pad model with a small amount of space */
r = 1.0 + model->rmax;

/* yet another magic number (0.427) - works reasonably well */
//pix2ang = sysenv.size;/*FIX: 533acf*/
//pix2ang *= 0.427 / model->rmax;/*FIX: 533acf*/
sysenv.rsize = r;

/* viewing */
glMatrixMode(GL_MODELVIEW);
glLoadIdentity();

/* set up camera */
camera = model->camera;
ARR3SET(x, camera->x);
ARR3SET(o, camera->o);
ARR3SET(v, camera->v);

#if DEBUG_INIT_PROJ
camera_dump(camera);
#endif

switch (camera->mode)
  {
  case FREE:
    break;

  default:
  case LOCKED:
    quat_rotate(x, camera->q);
    quat_rotate(o, camera->q);
    quat_rotate(v, camera->q);
    break;
  }

/* convert viewing vector to a location */
ARR3ADD(v, x);
gluLookAt(x[0], x[1], x[2], v[0], v[1], v[2], o[0], o[1], o[2]);

/* CURRENT - projection volume defined AFTER modelview has been set */
/* it's easier to get the right effect with the new free moving camera */
glMatrixMode(GL_PROJECTION);
glLoadIdentity();

/* prevent inversion due to -ve zoom */
if (camera->zoom < 0.05)
  camera->zoom = 0.05;

/* TODO - r = fn of zoom */
if (canvas)
  {
  a = canvas->width;
  a /= canvas->height;
  }
else
  {
/* stereo - doesn't get the canvas passed to it (wholescreen) */
  a = sysenv.width;
  a /= sysenv.height;
  }

sysenv.aspect = a;

if (camera->perspective)
  {
/* NB: if near distance is 0.0 it causes drawing problems */
  gluPerspective(camera->fov, a, 0.1, 4.0*sysenv.rsize);
  }
else
  {
  r *= camera->zoom;
  if (a > 1.0)
    glOrtho(-r*a, r*a, -r, r, 0.0, 4.0*sysenv.rsize);
  else
    glOrtho(-r, r, -r/a, r/a, 0.0, 4.0*sysenv.rsize);
  }

/* store matrices for proj/unproj operations */
/* FIXME - this will break stereo ... */
if (canvas)
  {
  glGetIntegerv(GL_VIEWPORT, canvas->viewport);
  glGetDoublev(GL_MODELVIEW_MATRIX, canvas->modelview);
  glGetDoublev(GL_PROJECTION_MATRIX, canvas->projection);
  }

/* opengl -> gdis coordinate conversion */
VEC3SET(&gl_acm[0], -1.0,  0.0,  0.0);
VEC3SET(&gl_acm[3],  0.0, -1.0,  0.0);
VEC3SET(&gl_acm[6],  0.0,  0.0, -1.0);
}

/****************************/
/* set up all light sources */
/****************************/
void gl_init_lights(struct model_pak *data)
{
gint i;
gfloat light[4];
gdouble x, tmp[4];
GSList *list;
struct light_pak *ldata;
struct camera_pak *camera;

g_assert(data != NULL);
g_assert(data->camera != NULL);

camera = data->camera;

/* go through all OpenGL lights (enable/disable as required) */
list = sysenv.render.light_list;
for (i=GL_LIGHT0 ; i<=GL_LIGHT7 ; i++)
  {
/* do we have an active light */
  if (list)
    {
    glEnable(i);
/* get light data */
    ldata = list->data;
/* position/direction */
    ARR3SET(light, ldata->x);
    ARR3SET(tmp, light);
    vecmat(gl_acm, tmp);

/* FIXME - FREE camera mode case */
    if (camera->mode == LOCKED)
      quat_rotate(tmp, camera->q);

    ARR3SET(light, tmp);

    switch (ldata->type)
      {
      case DIRECTIONAL:
        light[3] = 0.0;
        break;
      case POSITIONAL:
      default:
        VEC3MUL(light, -1.0);
        light[3] = 1.0;
      }
    glLightfv(i, GL_POSITION, light);
/* light properties */
    ARR3SET(light, ldata->colour);
    VEC3MUL(light, ldata->ambient);
    glLightfv(i, GL_AMBIENT, light);

    ARR3SET(light, ldata->colour);
    VEC3MUL(light, ldata->diffuse);
    glLightfv(i, GL_DIFFUSE, light);

    ARR3SET(light, ldata->colour);
    VEC3MUL(light, ldata->specular);
    glLightfv(i, GL_SPECULAR, light);
/* next */
    list = g_slist_next(list);
    }
  else
    glDisable(i);
  }

/* halo diminishing function */
for (i=0 ; i<halo_segments ; i++)
  {
  x = (gdouble) i / (gdouble) halo_segments;
  x *= x;
  halo_fade[i] = exp(-5.0 * x);
  }
}

/************************************/
/* compute sphere radius for a core */
/************************************/
gdouble gl_get_radius(struct core_pak *core, struct model_pak *model)
{
gdouble radius=1.0;
struct elem_pak elem;

g_assert(model != NULL);
g_assert(core != NULL);

switch (core->render_mode)
  {
  case CPK:
/* TODO - calling get_elem_data() all the time is inefficient */
    get_elem_data(core->atom_code, &elem, model);
    radius *= sysenv.render.cpk_scale * elem.vdw;
    break;

  case LIQUORICE:
/* only one bond - omit as it's a terminating atom */
/* FIXME - this will skip isolated atoms with one periodic bond */
    if (g_slist_length(core->bonds) == 1)
      radius *= -1.0;
/* more than one bond - put in a small sphere to smooth bond joints */
/* no bonds (isolated) - use normal ball radius */
    if (core->bonds)
      radius *= sysenv.render.stick_radius;
    else
      radius *= sysenv.render.ball_radius;
    break;

  case STICK:
    radius *= sysenv.render.stick_radius;
    if (core->bonds)
      radius *= -1.0;
    break;

  case BALL_STICK:
    if (sysenv.render.scale_ball_size)
      {
/* TODO - calling get_elem_data() all the time is inefficient */
      get_elem_data(core->atom_code, &elem, model);
      radius *= sysenv.render.cpk_scale * elem.vdw;
      }
    else
      radius *= sysenv.render.ball_radius;
    break;
  }
return(radius);
}

/*********************************************/
/* window to real space conversion primitive */
/*********************************************/
void gl_project(gdouble *w, gint x, gint y, struct canvas_pak *canvas)
{
gint ry;
GLdouble r[3];

ry = sysenv.height - y - 1;

/* z = 0.0 (near clipping plane) z = 1.0 (far clipping plane) */
/* z = 0.5 is in the middle of the viewing volume (orthographic) */
/* and is right at the fore for perspective projection */
gluUnProject(x, ry, 0.5, canvas->modelview, canvas->projection, canvas->viewport, &r[0], &r[1], &r[2]);
ARR3SET(w, r);
}

/*********************************************/
/* real space to window conversion primitive */
/*********************************************/
void gl_unproject(gint *x, gdouble *w, struct canvas_pak *canvas)
{
GLdouble r[3];

gluProject(w[0],w[1],w[2],canvas->modelview,canvas->projection,canvas->viewport,&r[0],&r[1],&r[2]);
x[0] = r[0];
x[1] = sysenv.height - r[1] - 1;
}

/********************************/
/* checks if a point is visible */
/********************************/
/*
gint gl_vertex_visible(gdouble *x)
{
gint p[2];

gl_get_window_coords(x, p);

if (p[0] < 0 || p[0] > sysenv.width)
  return(FALSE);
if (p[1] < 0 || p[1] > sysenv.height)
  return(FALSE);
return(TRUE);
}
*/

/*************************************************/
/* checks if a point is visible (with tolerance) */
/*************************************************/
/*
gint gl_vertex_tolerate(gdouble *x, gint dx)
{
gint p[2];

gl_get_window_coords(x, p);

if (p[0] < -dx || p[0] > sysenv.width+dx)
  return(FALSE);
if (p[1] < -dx || p[1] > sysenv.height+dx)
  return(FALSE);
return(TRUE);
}
*/

/*******************************************/
/* set opengl RGB colour for gdis RGB data */
/*******************************************/
void set_gl_colour(gint *rgb)
{
gdouble col[3];

col[0] = (gdouble) *rgb;
col[1] = (gdouble) *(rgb+1);
col[2] = (gdouble) *(rgb+2);
VEC3MUL(col, INV_COLOUR_SCALE);
glColor4f(col[0], col[1], col[2], 1.0);
}

/*************************************************************/
/* adjust fg colour for visibility against current bg colour */
/*************************************************************/
void make_fg_visible(void)
{
gdouble fg[3], bg[3];

ARR3SET(fg, sysenv.render.fg_colour);
ARR3SET(bg, sysenv.render.bg_colour);
VEC3MUL(fg, COLOUR_SCALE);
VEC3MUL(bg, COLOUR_SCALE);
/* XOR to get a visible colour */
fg[0] = (gint) bg[0] ^ COLOUR_SCALE;
fg[1] = (gint) bg[1] ^ COLOUR_SCALE;
fg[2] = (gint) bg[2] ^ COLOUR_SCALE;
VEC3MUL(fg, INV_COLOUR_SCALE);
ARR3SET(sysenv.render.fg_colour, fg);

/* adjust label colour for visibility against the current background */
ARR3SET(fg, sysenv.render.label_colour);
VEC3MUL(fg, COLOUR_SCALE);
/* XOR to get a visible colour */
fg[0] = (gint) bg[0] ^ COLOUR_SCALE;
fg[1] = (gint) bg[1] ^ COLOUR_SCALE;
VEC3MUL(fg, INV_COLOUR_SCALE);
/* force to zero, so we get yellow (not white) for a black background */
fg[2] = 0.0;
ARR3SET(sysenv.render.label_colour, fg);

/* adjust title colour for visibility against the current background */
/*
ARR3SET(fg, sysenv.render.title_colour);
VEC3MUL(fg, COLOUR_SCALE);
*/
/* force to zero, so we get cyan (not white) for a black background */
fg[0] = 0.0;
/* XOR to get a visible colour */
fg[0] = (gint) bg[0] ^ COLOUR_SCALE;
fg[1] = (gint) bg[1] ^ COLOUR_SCALE;
fg[2] = (gint) bg[2] ^ COLOUR_SCALE;

/* faded dark blue */
fg[0] *= 0.3;
fg[1] *= 0.65;
fg[2] *= 0.85;

/* apricot */
/*
fg[0] *= 0.9;
fg[1] *= 0.7;
fg[2] *= 0.4;
*/

VEC3MUL(fg, INV_COLOUR_SCALE);
ARR3SET(sysenv.render.title_colour, fg);

/*
printf("fg: %lf %lf %lf\n",fg[0],fg[1],fg[2]);
*/
}

/*******************************/
/* return approx. string width */
/*******************************/
gint gl_text_width(gchar *str)
{
return(strlen(str) * gl_fontsize);
}
/**********************/
/* update gl_fontsize */
/**********************/
void gl_get_fontsize(void){
PangoFontDescription *pfd;
pfd = pango_font_description_from_string(sysenv.gl_fontname);
gl_fontsize = pango_font_description_get_size(pfd) / PANGO_SCALE;
pango_font_description_free(pfd);
}

/***********************************/
/* pango print:  new routine as a  */
/* replacement for gl_print_window */
/* 2D print.              --OVHPA  */
/***********************************/
void pango_print(const gchar *str, gint x, gint y, struct canvas_pak *canvas, guint font_size, gint rotate)
{
/* PANGO */
PangoLayout *pl;
PangoFontDescription *pfd;
/* CAIRO */
cairo_t *cr;

if (!str || !sysenv.cairo_surface)
  return;

cairo_surface_flush(sysenv.cairo_surface);
cr = cairo_create(sysenv.cairo_surface);

cairo_translate(cr, x, y);
pl = pango_cairo_create_layout(cr);
pango_layout_set_markup(pl, str, strlen(str));
pango_layout_set_single_paragraph_mode(pl, TRUE);
pango_layout_set_width(pl, -1);
pfd = pango_font_description_from_string(sysenv.gl_fontname);
pango_font_description_set_absolute_size(pfd, font_size * PANGO_SCALE);
pango_layout_set_font_description(pl, pfd);
pango_font_description_free(pfd);
cairo_set_source_rgb(cr, (gdouble)sysenv.render.fg_colour[0],
                         (gdouble)sysenv.render.fg_colour[1],
                         (gdouble)sysenv.render.fg_colour[2]);
if(rotate != 0)
  {
  cairo_rotate(cr,(double)(rotate) * G_PI / -180.);
  pango_cairo_update_layout(cr, pl);
  }
pango_cairo_show_layout(cr, pl);
g_object_unref(pl);
cairo_destroy (cr);
cairo_surface_mark_dirty(sysenv.cairo_surface);
}

/**************************************/
/* pango print_world:  new routine as */
/* a replacement for gl_print_world   */
/* 3D print.                 --OVHPA  */
/**************************************/
#define DEBUG_PANGO_TEXT 0
void pango_print_world(gchar *str, gdouble *v, struct canvas_pak *canvas) 
{
gint width = 0;
gint height = 0;
gint rx[2];
/* cairo */
cairo_t *render;
cairo_surface_t *surface;
unsigned char* surface_data = NULL;
/* pango */
PangoRectangle prect;
PangoFontDescription *pfd;
PangoLayout *pl;
PangoContext *pc;

if (!str || !v || !canvas)
  return;

if (!gdis_gl_context_is_legacy(sysenv.glarea) && sysenv.cairo_surface)
  {
  gl_unproject(rx, v, canvas);
  pango_print(str, rx[0], rx[1], canvas, gl_fontsize, 0);
  return;
  }

pc = gtk_widget_create_pango_context(sysenv.glarea);
pl = pango_layout_new(pc);
pfd = pango_font_description_from_string(sysenv.gl_fontname);
pango_font_description_set_absolute_size(pfd, gl_fontsize * PANGO_SCALE);
pango_layout_set_font_description(pl, pfd);
pango_font_description_free(pfd);
pango_layout_set_markup(pl, str, strlen(str));
pango_layout_set_single_paragraph_mode(pl, TRUE);
pango_layout_set_width(pl, -1);

/* new */
pango_layout_get_pixel_extents (pl, NULL, &prect);
pango_layout_get_size(pl, &width, &height);
width /= PANGO_SCALE;
height /= PANGO_SCALE;
surface_data = g_malloc0(4*width*height*sizeof(unsigned char));
surface = cairo_image_surface_create_for_data(surface_data, CAIRO_FORMAT_ARGB32, width, height, 4*width);
render = cairo_create(surface);
cairo_translate(render, -prect.x, -prect.y); /* important? */
cairo_set_source_rgba(render, sysenv.render.fg_colour[0], sysenv.render.fg_colour[1], sysenv.render.fg_colour[2], 1.0);
cairo_move_to(render, 0, 0);
pango_cairo_show_layout(render, pl);
cairo_destroy(render);
g_object_unref(pl);
g_object_unref(pc);

//#if DEBUG_PANGO_TEXT
//cairo_surface_write_to_png(surface,"./test.png"); /* PROVE OK */
//#endif

/* copy to framebuffer */
glRasterPos3f(v[0], v[1], v[2]);
glPixelZoom( 1, -1);
glDrawPixels(width, height, GL_BGRA, GL_UNSIGNED_BYTE, surface_data);

/* for DEBUG we draw a box around the text, via unproject */
#if DEBUG_PANGO_TEXT
gint rx[2];
gdouble w[3];
w[0] = v[0];
w[1] = v[1];
w[2] = v[2];
gl_unproject(rx, w, canvas);
gl_draw_box(rx[0], rx[1], rx[0]+width, rx[1]+height, canvas);
#endif

/* destroy */
cairo_surface_destroy(surface);
g_free(surface_data);
}
/********************************/
/* vertex at 2D screen position */
/********************************/
void gl_vertex_window(gint x, gint y, struct canvas_pak *canvas)
{
gdouble w[3];

gl_project(w, x, y, canvas);
glVertex3dv(w);
}

/*********************************/
/* draw a box at screen position */
/*********************************/
void gl_draw_box(gint px1, gint py1, gint px2, gint py2, struct canvas_pak *canvas)
{
glBegin(GL_LINE_LOOP);
gl_vertex_window(px1, py1, canvas);
gl_vertex_window(px1, py2, canvas);
gl_vertex_window(px2, py2, canvas);
gl_vertex_window(px2, py1, canvas);
glEnd();
}

/*********************************************************/
/* determines if input position is close to a drawn core */
/*********************************************************/
#define PIXEL_TOLERANCE 5
gint gl_core_proximity(gint x, gint y, struct core_pak *core, struct canvas_pak *canvas)
{
gint dx, dy, dr, p[2], itol;
gdouble tol;
struct model_pak *model;

if (!canvas || !core)
  return(G_MAXINT);

model = canvas->model;

if (!model)
  return(G_MAXINT);

gl_unproject(p, core->rx, canvas);

tol = gl_get_radius(core, model);

tol *= sysenv.size;
tol /= model->rmax;

/* HACK - ensure tolerance is at least a few pixels */
itol = nearest_int(tol);
if (itol < PIXEL_TOLERANCE)
  itol = PIXEL_TOLERANCE;
itol *= itol;

dx = p[0] - x;
dy = p[1] - y;
dr = dx*dx + dy*dy;

if (dr < itol)
  return(dr);

return(G_MAXINT);
}

/**********************************************/
/* seek nearest core to window pixel position */
/**********************************************/
#define DEBUG_SEEK_CORE 0
gpointer gl_seek_core(gint x, gint y, struct model_pak *model)
{
gint dr, rmin;
GSList *list;
struct core_pak *core, *found=NULL;
struct canvas_pak *canvas;
gboolean debug_pick;

#if DEBUG_SEEK_CORE
printf("mouse: [%d, %d]\n", x, y);
#endif

/* canvas aware */
canvas = canvas_find(model);
g_assert(canvas != NULL);

debug_pick = (g_getenv("GDIS_DEBUG_PICK") != NULL);
if (debug_pick)
  g_printerr("GDIS pick: seek core at %d,%d\n", x, y);

/* default tolerance */
rmin = sysenv.size;

for (list=model->cores ; list ; list=g_slist_next(list))
  {
  core = list->data;

  dr = gl_core_proximity(x, y, core, canvas);
  if (debug_pick)
    {
    gint p[2];

    gl_unproject(p, core->rx, canvas);
    g_printerr("GDIS pick:   atom=%s screen=%d,%d dr=%d\n",
               core->atom_label, p[0], p[1], dr);
    }
  if (dr < rmin)
    {
    found = core;
    rmin = dr;
    }
  }

if (debug_pick)
  {
  g_printerr("GDIS pick: result=%s best=%d\n",
             found ? found->atom_label : "(none)", rmin);
  }
return(found);
}

/********************************/
/* OpenGL atom location routine */
/********************************/
#define DEBUG_GL_SEEK_BOND 0
gpointer gl_seek_bond(gint x, gint y, struct model_pak *model)
{
gint p1[2], p2[2];
gdouble r[3];
gdouble dx, dy, d2;
gdouble tol;
gpointer match=NULL;
GSList *list;
struct bond_pak *bdata;
struct core_pak *core1, *core2;

/* default (squared) tolerance */
tol = sysenv.size;
tol /= model->rmax;

#if DEBUG_GL_SEEK_BOND
printf("input: %d,%d [%f]\n", x, y, tol);
#endif

/* search */
for (list=model->bonds ; list ; list=g_slist_next(list))
  {
  bdata = list->data; 
  core1 = bdata->atom1;
  core2 = bdata->atom2;

  ARR3SET(r, core1->rx);
  gl_unproject(p1, r, canvas_find(model));

  ARR3SET(r, core2->rx);
  gl_unproject(p2, r, canvas_find(model));

#if DEBUG_GL_SEEK_BOND
printf("[%s-%s] @ [%f,%f]\n", core1->atom_label, core2->atom_label, 0.5*(p1[0]+p2[0]), 0.5*(p1[1]+p2[1]));
#endif

  dx = 0.5*(p1[0]+p2[0]) - x;
  dy = 0.5*(p1[1]+p2[1]) - y;

  d2 = dx*dx + dy*dy;
  if (d2 < tol)
    {
/* keep searching - return the best match */
    tol = d2;
    match = bdata;
    }
  }
return(match);
}

/***************************************************/
/* compute atoms that lie within the selection box */
/***************************************************/
#define DEBUG_GL_SELECT_BOX 0
#define GL_SELECT_TOLERANCE 10
void gl_select_box(GtkWidget *w)
{
gint i, tmp, x[2];
gdouble r[3], piv[3];
GSList *list, *ilist=NULL;
struct model_pak *data;
struct core_pak *core;
struct image_pak *image;

/* valid model */
data = sysenv.active_model;
if (!data)
  return;
if (data->graph_active)
  return;
if (data->picture_active)
  return;

/* ensure box limits have the correct order (ie low to high) */
for (i=0 ; i<2 ; i++)
  {
  if (data->select_box[i] > data->select_box[i+2])
    {
    tmp = data->select_box[i];
    data->select_box[i] = data->select_box[i+2];
    data->select_box[i+2] = tmp;
    }
  }

/* add pixel tolerance for (approx) single clicks */
if ((data->select_box[2] - data->select_box[0]) < GL_SELECT_TOLERANCE)
  {
  data->select_box[0] -= GL_SELECT_TOLERANCE;
  data->select_box[2] += GL_SELECT_TOLERANCE;
  }
if ((data->select_box[1] - data->select_box[3]) < GL_SELECT_TOLERANCE)
  {
  data->select_box[1] -= GL_SELECT_TOLERANCE;
  data->select_box[3] += GL_SELECT_TOLERANCE;
  }

#if DEBUG_GL_SELECT_BOX
printf("[%d,%d] - [%d,%d]\n", data->select_box[0], data->select_box[1], 
                              data->select_box[2], data->select_box[3]);
#endif

/* find matches */
do
  {
/* periodic images */
  if (ilist)
    {
    image = ilist->data;
    ARR3SET(piv, image->rx);
    ilist = g_slist_next(ilist);
    }
  else
    {
    VEC3SET(piv, 0.0, 0.0, 0.0);
    ilist = data->images;
    }
/* cores */
  for (list=data->cores ; list ; list=g_slist_next(list))
    {
    core = list->data;
    if (core->status & DELETED)
      continue;

/* get core pixel position */
/* CURRENT */
    ARR3SET(r, core->rx);
    ARR3ADD(r, piv);
    gl_unproject(x, r, canvas_find(data));

/* check bounds */
    if (x[0] > data->select_box[0] && x[0] < data->select_box[2])
      {
      if (x[1] > data->select_box[1] && x[1] < data->select_box[3])
        {
        select_core(core, FALSE, data);
        }
      }
    }
  }
while (ilist);

redraw_canvas(SINGLE);
}

/********************************************/
/* build lists for respective drawing types */
/********************************************/
#define DEBUG_BUILD_CORE_LISTS 0
void gl_build_core_lists(GSList **solid, GSList **ghost, GSList **wire, struct model_pak *model)
{
GSList *list;
struct core_pak *core;

g_assert(model != NULL);

*solid = *ghost = *wire = NULL;
for (list=model->cores ; list ; list=g_slist_next(list))
  {
  core = list->data;

/* bailout checks */
  if (core->status & (HIDDEN | DELETED))
    continue;
  if (core->render_mode == ZONE)
    continue;

/* Checks for MARVIN regions */
  if ((!model->show_region1A && core->region == REGION1A) ||
      (!model->show_region1B && core->region == REGION1B) ||
      (!model->show_region2A && core->region == REGION2A) ||
      (!model->show_region2B && core->region == REGION2B))
    continue;

/* build appropriate lists */
  if (core->render_wire)
    *wire = g_slist_prepend(*wire, core);
  else if (core->ghost)
    *ghost = g_slist_prepend(*ghost, core);
  else
    *solid = g_slist_prepend(*solid, core);
  }

#if DEBUG_BUILD_CORE_LISTS
printf("solid: %d\n", g_slist_length(*solid));
printf("ghost: %d\n", g_slist_length(*ghost));
printf(" wire: %d\n", g_slist_length(*wire));
#endif
}

/************************/
/* draw a list of cores */
/************************/
#define DEBUG_DRAW_CORES 0
void gl_draw_cores(GSList *cores, struct model_pak *model)
{
gint max, quality;
#ifdef UNUSED_BUT_SET
gint dx;
#endif
gdouble radius, x[3], colour[4];
GSList *list, *ilist;
struct core_pak *core;
struct point_pak sphere;
struct image_pak *image;

/* NEW - limit the quality based on physical model size */
/* FIXME - broken due to moving camera */
quality = sysenv.render.sphere_quality;
if (sysenv.render.auto_quality)
  {
  radius = sysenv.size / model->zoom;
  max = radius/10;
#ifdef UNUSED_BUT_SET
  dx = 10;
#endif

#if DEBUG_DRAW_CORES
printf("%f : %d [%d]\n", radius, max, quality);
#endif

  if (quality > max)
    sysenv.render.sphere_quality = max;
  }

/* set up geometric primitive */
gl_init_sphere(&sphere, model);

/* draw desired cores */
for (list=cores ; list ; list=g_slist_next(list))
  {
  core = list->data;

/* set colour */
  ARR3SET(colour, core->colour);
  VEC3MUL(colour, INV_COLOUR_SCALE);
  colour[3] = core->colour[3];
  glColor4dv(colour);

  radius = gl_get_radius(core, model);
  if (radius > 0.0)
    {
/* original + image iteration */
    ilist = NULL;
    do
      {
      ARR3SET(x, core->rx);
      if (ilist)
        {
/* image translation */
        image = ilist->data;
        ARR3ADD(x, image->rx);
        ilist = g_slist_next(ilist);
        }
      else
        ilist = model->images;
/*
      if (gl_vertex_tolerate(x, dx))
*/
      gl_draw_sphere(&sphere, x, radius);
      }
    while (ilist);
    }
  }
gl_free_points(&sphere);
sysenv.render.sphere_quality = quality;
}

/* CURRENT - implement fractional image_limits */
/* possibility is draw extra whole unit (ie plus bonds) then use clipping planes to trim */
#if EXPERIMENTAL
{
gint i, j, a, b, c, flag, limit[6], test[6];
gdouble t[4], frac[6], whole;
struct mol_pak *mol;

/* set up integer limits */
for (i=6 ; i-- ; )
  {
  frac[i] = modf(data->image_limit[i], &whole);
  limit[i] = (gint) whole;
/* if we have a fractional part - extend the periodic image boundary */
  if (frac[i] > FRACTION_TOLERANCE)
    {
/*
printf("i = %d, frac = %f\n", i, frac[i]);
*/
    test[i] = TRUE;
    limit[i]++;
    }
  else
    test[i] = FALSE;
  }

limit[0] *= -1;
limit[2] *= -1;
limit[4] *= -1;

/* setup for pic iteration */
a = limit[0];
b = limit[2];
c = limit[4];

for (;;)
  {
/* image increment */
  if (a == limit[1])
    {
    a = limit[0];
    b++;
    if (b == limit[3])
      {
      b = limit[2];
      c++;
      if (c == limit[5])
        break;
      }
    }

  VEC3SET(t, a, b, c);

/* NEW - include fractional cell images */
/* TODO - include the testing as part of image increment testing? (ie above) */
flag = TRUE;
for (i=0 ; i<data->periodic ; i++)
  {
/* +ve fractional extent test */
  if (test[2*i+1])
  if (t[i] == limit[2*i+1]-1) 
    {
    for (j=0 ; j<data->periodic ; j++)
      if (test[2*j+1])
        {
/* check molecule centroid, rather than atom coords */
        mol = core->mol;

        if (mol->centroid[j] > frac[2*j+1])
          flag = FALSE;
        }
    }

/* -ve fractional extent test */
  if (test[2*i])
  if (t[i] == limit[2*i]) 
    {
/* + ve fractional extent test */
    for (j=0 ; j<data->periodic ; j++)
      if (test[2*j])
        {
/* check molecule centroid, rather than atom coords */
        mol = core->mol;

/* TODO - pre-sub 1.0 from frac for this test */
        if (mol->centroid[j] < (1.0-frac[2*j]))
          flag = FALSE;
        }
    }
  }

if (flag)
  {

  t[3] = 0.0;
  vec4mat(data->display_lattice, t);

  ARR3SET(vec, core->rx);
  ARR3ADD(vec, t);

  gl_draw_sphere(&sphere, vec, radius);
  }

  a++;
  }
}
#endif

/**********************************/
/* draw the selection halo/circle */
/**********************************/
void gl_draw_halo_list(GSList *list, struct model_pak *data)
{
gint h;
gdouble radius, dr;
gdouble vec[3], halo[4];
GSList *item, *ilist;
struct point_pak circle;
struct core_pak *core;
struct image_pak *image;

g_assert(data != NULL);

/* variable quality halo */
h = 2 + sysenv.render.sphere_quality;
h = h*h;
gl_init_circle(&circle, h, data);

/* halo colour */
VEC4SET(halo, 1.0, 0.95, 0.45, 1.0);
for (item=list ; item ; item=g_slist_next(item))
  {
  core = item->data;

  glColor4dv(halo);
  radius = gl_get_radius(core, data);
  if (radius == 0.0)
    continue;

/* halo ring size increment */
/* a fn of the radius? eg 1/2 */
  dr = 0.6*radius/halo_segments;

/* original + image iteration */
  ilist = NULL;
  do
    {
    ARR3SET(vec, core->rx);
    if (ilist)
      {
/* image translation */
      image = ilist->data;
      ARR3ADD(vec, image->rx);
      ilist = g_slist_next(ilist);
      }
    else
      ilist = data->images;

    if (sysenv.render.halos)
      {
/* halo fade loop */
      for (h=0 ; h<halo_segments ; h++)
        {
        halo[3] = halo_fade[h];

        glColor4dv(halo);
        gl_draw_ring(&circle, vec, radius+h*dr-0.5*dr, radius+h*dr+0.5*dr);
        }
/* reset halo transparancy */
      halo[3] = 1.0;
      }
    else
      gl_draw_ring(&circle, vec, 1.2*radius, 1.4*radius);
    }
  while (ilist);
  }

gl_free_points(&circle);
}

/*****************************************/
/* translucent atom depth buffer sorting */
/*****************************************/
gint gl_depth_sort(struct core_pak *c1, struct core_pak *c2)
{
if (c1->rx[2] > c2->rx[2])
  return(-1);
return(1);
}

/***********************/
/* draw model's shells */
/***********************/
void gl_draw_shells(struct model_pak *data)
{
gint omit_atom, mode;
gdouble radius;
gdouble vec[3], colour[4];
GSList *list, *ilist;
struct point_pak sphere;
struct core_pak *core;
struct shel_pak *shel;
struct image_pak *image;

g_assert(data != NULL);

gl_init_sphere(&sphere, data);

for (list=data->shels ; list ; list=g_slist_next(list))
  {
  shel = list->data;
  if (shel->status & (DELETED | HIDDEN))
    continue;

/* Checks for MARVIN regions */
  if ((!data->show_region1A && shel->region==REGION1A) ||
      (!data->show_region1B && shel->region==REGION1B) ||
      (!data->show_region2A && shel->region==REGION2A) ||
      (!data->show_region2B && shel->region==REGION2B))
    continue;

/* shell colour */
  ARR3SET(colour, shel->colour);
  colour[3] = 0.5;

/* render mode */
  core = shel->core;

  if (core)
    mode = core->render_mode;
  else
    mode = BALL_STICK;

/* bailout modes */
  switch (mode)
    {
    case LIQUORICE:
    case ZONE:
      continue;
    }

/* position */
  ilist=NULL;
  do
    {
    ARR3SET(vec, shel->rx);
    if (ilist)
      {
/* image */
      image = ilist->data;
      ARR3ADD(vec, image->rx);
      ilist = g_slist_next(ilist);
      }
    else
      {
      ilist = data->images;
      }

/* set appropriate radius */
    omit_atom = FALSE;
    radius = 1.0;
    switch (mode)
      {
      case BALL_STICK:
        radius *= sysenv.render.ball_radius;
        break;

      case CPK:
        radius *= elements[shel->atom_code].vdw;
        break;

      case STICK:
        radius *= sysenv.render.stick_radius;
        break;
      }
    if (!omit_atom)
      {
      glColor4dv(colour);
      gl_draw_sphere(&sphere, vec, radius);
      }
    }
  while (ilist);
  }

glEnable(GL_LIGHTING);

gl_free_points(&sphere);
}

/*******************************************/
/* draw model pipes (separate bond halves) */
/*******************************************/
/* stage (drawing stage) ie colour materials/lines/etc. */
/* TODO - only do minimum necessary for each stage */
void gl_draw_pipes(gint line, GSList *pipe_list, struct model_pak *model)
{
#ifdef UNUSED_BUT_SET
gint dx;
#endif
guint q;
gdouble v1[3], v2[3];
GSList *list, *ilist;
struct pipe_pak *pipe;
struct image_pak *image;

/* setup for bond drawing */
q = sysenv.render.cylinder_quality;

/* FIXME - this is broken by the new camera code */
if (sysenv.render.auto_quality)
  {
  if (!line)
    {
/* only use desired quality if less than our guess at the useful maximum */
    q = sysenv.size / (10.0 * model->rmax);
    q++;
    if (q > sysenv.render.cylinder_quality)
      q = sysenv.render.cylinder_quality;
    }
  }
#ifdef UNUSED_BUT_SET
dx = 10;
#endif
/* enumerate the supplied pipes (half bonds) */
for (list=pipe_list ; list ; list=g_slist_next(list))
  {
  pipe = list->data;

/* original + image iteration */
  ilist = NULL;
  do
    {
/* original */
    ARR3SET(v1, pipe->v1);
    ARR3SET(v2, pipe->v2);
    if (ilist)
      {
      image = ilist->data;
/* image */
      ARR3ADD(v1, image->rx);
      ARR3ADD(v2, image->rx);
      ilist = g_slist_next(ilist);
      }
    else
      ilist = model->images;

/* NEW - don't render if both endpoints are off screen */
/*
    if (gl_vertex_tolerate(v1, dx) && gl_vertex_tolerate(v2, dx))
*/
//    {
    glColor4dv(pipe->colour);
    if (line)
      {
      glBegin(GL_LINES);
      glVertex3dv(v1);
      glVertex3dv(v2);
      glEnd();
      }
    else
      gl_draw_cylinder(v1, v2, pipe->radius, q);
//    }
    }
  while (ilist);
  }
}

/***************************/
/* draw crystal morphology */
/***************************/
/* deprecated */
#define DEBUG_DRAW_MORPH 0
void gl_draw_morph(struct model_pak *data)
{
gdouble x[3];
GSList *list1, *list2;
struct plane_pak *plane;
struct vertex_pak *v;

/* checks */
g_assert(data != NULL);

/* turn lighting off for wire frame drawing - looks esp ugly */
/* when hidden (stippled) lines and normal lines are overlayed */
//if (0.5*sysenv.render.wire_surface)
if (((gint)0.5*sysenv.render.wire_surface)!=0)
  glDisable(GL_LIGHTING);

glLineWidth(sysenv.render.frame_thickness);

/* visibility calculation */
for (list1=data->planes ; list1 ; list1=g_slist_next(list1))
  {
  plane = list1->data;

/* CURRENT */
/*
  if (plane->present)
{
struct vertex_pak *v1, *v2, *v3;

if (plane->m[0] == 0 && plane->m[1] == 2 && plane->m[2] == -1)
  {
printf(" ==== (%f %f %f)\n", plane->m[0], plane->m[1], plane->m[2]);
P3VEC("n: ", plane->norm);

for (list2=plane->vertices ; list2 ; list2=g_slist_next(list2))
  {
v1 = list2->data;
P3VEC("v: ", v1->rx);
  }

  }
}
*/


  if (plane->present)
    plane->visible = facet_visible(data, plane);
  }

/* draw hidden lines first */
if (sysenv.render.wire_surface && sysenv.render.wire_show_hidden)
  {
  glEnable(GL_LINE_STIPPLE);
  glLineStipple(1, 0x0303);

  for (list1=data->planes ; list1 ; list1=g_slist_next(list1))
    {
    plane = list1->data;
    if (!plane->present)
      continue;

/* start the face */
    glBegin(GL_POLYGON);
    for (list2=plane->vertices ; list2 ; list2=g_slist_next(list2))
      {
      v = list2->data;

      ARR3SET(x, v->rx);

      glNormal3dv(plane->norm);
      glVertex3dv(x);
      }
    glEnd();
    }
  }

/* draw the visible facets */
glDisable(GL_LINE_STIPPLE);
for (list1=data->planes ; list1 ; list1=g_slist_next(list1))
  {
  plane = list1->data;

  if (!plane->visible)
    continue;
  if (!plane->present)
    continue;

#if DEBUG_DRAW_MORPH
printf("(%f %f %f) :\n", plane->m[0], plane->m[1], plane->m[2]);
#endif

/* start the face */
  glBegin(GL_POLYGON);
  for (list2=plane->vertices ; list2 ; list2=g_slist_next(list2))
    {
    v = list2->data;

#if DEBUG_DRAW_MORPH
P3VEC("  : ", v->rx);
#endif

    ARR3SET(x, v->rx);
    glNormal3dv(plane->norm);
    glVertex3dv(x);
    }
  glEnd();
  }

/* if wire frame draw - turn lighting back on */
if (sysenv.render.wire_surface)
  glEnable(GL_LIGHTING);
}

/***********************************/
/* draw the cartesian/lattice axes */
/***********************************/
void gl_draw_axes(gint mode, struct canvas_pak *canvas, struct model_pak *data)
{
gint i, ry;
gdouble f, x1[3], x2[3];
gchar label[3];

if (!canvas)
  return;
if (!data)
  return;

glDisable(GL_FOG);

/* axes type setup */
if (data->axes_type == CARTESIAN)
  strcpy(label, " x");
else
  strcpy(label, " a");

/* inverted y sense correction */
ry = sysenv.height - canvas->y - canvas->height + 40;
gl_project(x1, canvas->x+40, ry, canvas);
gl_project(x2, canvas->x+20, ry, canvas);
ARR3SUB(x2, x1);

/* yet another magic number */
f = 20.0 * VEC3MAG(x2) / data->rmax;

if (mode)
  {
/* set colour */
  glColor4f(sysenv.render.fg_colour[0], sysenv.render.fg_colour[1],
            sysenv.render.fg_colour[2], 1.0);
/* draw the axes */
  for (i=3 ; i-- ; )
    {
    ARR3SET(x2, data->axes[i].rx);
    VEC3MUL(x2, f);
    ARR3ADD(x2, x1);

/* yet another magic number for the vector thickness */
    draw_vector(x1, x2, 0.005*f*data->rmax);
    }
  }
else
  {
/* draw the labels - offset by fontsize? */
  glColor4f(sysenv.render.title_colour[0], sysenv.render.title_colour[1],
            sysenv.render.title_colour[2], 1.0);
  for (i=0 ; i<3 ; i++)
    {
    ARR3SET(x2, data->axes[i].rx);
    VEC3MUL(x2, f);
    ARR3ADD(x2, x1);
    pango_print_world(label, x2, canvas);
    label[1]++;
    }
  }

if (sysenv.render.fog)
  glEnable(GL_FOG);
}

/*******************************************/
/* draw the cell frame for periodic models */
/*******************************************/
void gl_draw_cell(struct model_pak *data)
{
gint i, j;
gdouble v1[3], v2[3], v3[3], v4[3];

/* draw the opposite ends of the frame */
for (i=0 ; i<5 ; i+=4)
  {
  glBegin(GL_LINE_LOOP);
  ARR3SET(v1, data->cell[i+0].rx);
  ARR3SET(v2, data->cell[i+1].rx);
  ARR3SET(v3, data->cell[i+2].rx);
  ARR3SET(v4, data->cell[i+3].rx);
  glVertex3dv(v1);
  glVertex3dv(v2);
  glVertex3dv(v3);
  glVertex3dv(v4);
  glEnd();
  }
/* draw the sides of the frame */
glBegin(GL_LINES);
for (i=4 ; i-- ; )
  {
  j = i+4;
/* retrieve coordinates */
  ARR3SET(v1, data->cell[i].rx);
  ARR3SET(v2, data->cell[j].rx);
/* draw */
  glVertex3dv(v1);
  glVertex3dv(v2);
  }
glEnd();
}

/***************************************/
/* draw the cell frame periodic images */
/***************************************/
void gl_draw_cell_images(struct model_pak *model)
{
gint i, j;
gdouble v1[3], v2[3], v3[3], v4[3];
GSList *ilist;
struct image_pak *image;

/* image iteration (don't do original) */
//ilist = model->images;/*FIX: b38968*/
for (ilist=model->images ; ilist ; ilist=g_slist_next(ilist))
  {
/* image translation */
  image = ilist->data;

/* draw the opposite ends of the frame */
  for (i=0 ; i<5 ; i+=4)
    {
    glBegin(GL_LINE_LOOP);
    ARR3SET(v1, model->cell[i+0].rx);
    ARR3SET(v2, model->cell[i+1].rx);
    ARR3SET(v3, model->cell[i+2].rx);
    ARR3SET(v4, model->cell[i+3].rx);
    ARR3ADD(v1, image->rx);
    ARR3ADD(v2, image->rx);
    ARR3ADD(v3, image->rx);
    ARR3ADD(v4, image->rx);
    glVertex3dv(v1);
    glVertex3dv(v2);
    glVertex3dv(v3);
    glVertex3dv(v4);
    glEnd();
    }
/* draw the sides of the frame */
  glBegin(GL_LINES);
  for (i=4 ; i-- ; )
    {
    j = i+4;
/* retrieve coordinates */
    ARR3SET(v1, model->cell[i].rx);
    ARR3SET(v2, model->cell[j].rx);
    ARR3ADD(v1, image->rx);
    ARR3ADD(v2, image->rx);
/* draw */
    glVertex3dv(v1);
    glVertex3dv(v2);
    }
  glEnd();
  }
}

/***************************/
/* draw core - shell links */
/***************************/
void gl_draw_links(struct model_pak *model)
{
GSList *list;
struct core_pak *core;
struct shel_pak *shell;

g_assert(model != NULL);

glBegin(GL_LINES);
for (list=model->cores ; list ; list=g_slist_next(list))
  {
  core = list->data;

  if (core->shell)
    {
    shell = core->shell;

    glVertex3dv(core->rx);
    glVertex3dv(shell->rx);
    }
  }
glEnd();
}

/*********************/
/* draw measurements */
/*********************/
void gl_draw_measurements(struct model_pak *model)
{
gint type;
gdouble colour[3], a1[3], a2[3], a3[3], a4[3], v1[3], v2[3], v3[3], n[3];
GSList *list;

/* draw the lines */
for (list=model->measure_list ; list ; list=g_slist_next(list))
  {
  type = measure_type_get(list->data);

  measure_colour_get(colour, list->data);
  glColor4f(colour[0], colour[1], colour[2], 1.0);

  switch (type)
    {
    case MEASURE_BOND:
    case MEASURE_DISTANCE:
    case MEASURE_INTER:
    case MEASURE_INTRA:
      measure_coord_get(v1, 0, list->data, model);
      measure_coord_get(v2, 1, list->data, model);
      glBegin(GL_LINES);
      glVertex3dv(v1);
      glVertex3dv(v2);
      glEnd();
      break;

    case MEASURE_ANGLE:
      measure_coord_get(v1, 0, list->data, model);
      measure_coord_get(v2, 1, list->data, model);
      measure_coord_get(v3, 2, list->data, model);
/* NB: central atom should be first */
      draw_arc(v2, v1, v3);
      break;

    case MEASURE_TORSION:
/* get constituent core coordinates */
      measure_coord_get(a1, 0, list->data, model);
      measure_coord_get(a2, 1, list->data, model);
      measure_coord_get(a3, 2, list->data, model);
      measure_coord_get(a4, 3, list->data, model);
/* middle 2 cores define the axis */
      ARR3SET(n, a3);
      ARR3SUB(n, a2);
      normalize(n, 3);
/* arm 1 */
      ARR3SET(v3, a1);
      ARR3SUB(v3, a2);
      proj_vop(v1, v3, n);
      normalize(v1, 3);
/* arm 2 */
      ARR3SET(v3, a4);
      ARR3SUB(v3, a3);
      proj_vop(v2, v3, n);
      normalize(v2, 3);
/* axis centre */
      ARR3SET(v3, a2);
      ARR3ADD(v3, a3);
      VEC3MUL(v3, 0.5);
/* arm endpoints are relative to axis centre */
      ARR3ADD(v1, v3);
      ARR3ADD(v2, v3);
/* draw arc */
      draw_arc(v3, v1, v2);
/* draw lines */
      glBegin(GL_LINE_STRIP);
      glVertex3dv(v1);
      glVertex3dv(v3);
      glVertex3dv(v2);
      glEnd();
      break;

    }
  }
}

/***************************/
/* draw morphology indices */
/***************************/
void gl_draw_miller(struct model_pak *data)
{
gchar *label;
gdouble vec[3];
GSList *plist;
struct plane_pak *plane;

/* draw facet labels */
plist = data->planes;
while (plist != NULL)
  {
  plane = plist->data;
  if (plane->present && plane->visible)
    {
/* TODO - scale the font with data->scale? (vanishes if too small) */
/* print the hkl label */
    label = g_strdup_printf("%d%d%d", plane->index[0],
                                      plane->index[1],
                                      plane->index[2]);
    ARR3SET(vec, plane->rx);
    pango_print_world(label, vec, canvas_find(data));
    g_free(label);

/* TODO - vector font (display list) with number + overbar number */
/*
    glBegin(GL_LINES);
    if (plane->index[0] < 0)
      {
      glVertex3d(plane->rx, plane->ry-gl_fontsize, plane->rz);
      glVertex3d(plane->rx+gl_fontsize, plane->ry-gl_fontsize, plane->rz);
      }
    if (plane->index[1] < 0)
      {
      glVertex3d(plane->rx+1*gl_fontsize, plane->ry-gl_fontsize, plane->rz);
      glVertex3d(plane->rx+2*gl_fontsize, plane->ry-gl_fontsize, plane->rz);
      }
    if (plane->index[2] < 0)
      {
      glVertex3d(plane->rx+2*gl_fontsize, plane->ry-gl_fontsize, plane->rz);
      glVertex3d(plane->rx+3*gl_fontsize, plane->ry-gl_fontsize, plane->rz);
      }
    glEnd();
*/

    }
  plist = g_slist_next(plist);
  }
}

/************************************************/
/* draw the colour scale for molecular surfaces */
/************************************************/
/* FIXME - siesta epot */
void gl_draw_colour_scale(gint x, gint y, struct model_pak *data)
{
gint i, n;
gdouble z1, dz;
gdouble colour[3], w1[3], w2[3];
GString *text;
struct canvas_pak *canvas = canvas_find(data);

g_assert(data != NULL);

if (!gdis_gl_context_is_legacy(sysenv.glarea) && sysenv.cairo_surface)
  {
  gl_core_draw_colour_scale_overlay(canvas, x, y, data);
  return;
  }

/* init */
n = data->epot_div;
z1 = data->epot_max;
dz = (data->epot_max - data->epot_min)/ (gdouble) (n-1);

/* colour boxes */
glPolygonMode(GL_FRONT, GL_FILL);
glBegin(GL_QUADS);
for (i=0 ; i<n ; i++)
  {
  switch (data->ms_colour_method)
    {
    case MS_SOLVENT:
      ms_dock_colour(colour, z1, data->epot_min, data->epot_max);
      break;

    default:
      ms_epot_colour(colour, z1, data->epot_min, data->epot_max);
    }

  glColor3f(colour[0], colour[1], colour[2]);
  z1 -= dz;

  gl_project(w1, x, y+i*20, canvas);
  gl_project(w2, x, y+19+i*20, canvas);
  glVertex3dv(w1);
  glVertex3dv(w2);

  gl_project(w1, x+19, y+19+i*20, canvas);
  gl_project(w2, x+19, y+i*20, canvas);
  glVertex3dv(w1);
  glVertex3dv(w2);
  }
glEnd();

/* init */
text = g_string_new(NULL);
z1 = data->epot_max;
glColor3f(1.0, 1.0, 1.0);

/* box labels */
for (i=0 ; i<n ; i++)
  {
  g_string_printf(text, "%6.2f", z1);//g_string_sprintf
  pango_print(text->str, x+30, y+i*20+18, canvas,gl_fontsize, 0);
  z1 -= dz;
  }
g_string_free(text, TRUE);
}

/**************************************/
/* draw text we wish to be unobscured */
/**************************************/
void gl_draw_text(struct canvas_pak *canvas, struct model_pak *data)
{
guint i;
gint j, type;
gint n = 0, hidden = 0;
gchar *text;
gdouble q, v1[3], v2[3], v3[3];
GSList *list;
GString *label;
struct vec_pak *v;
struct spatial_pak *spatial;
struct core_pak *core[4];
struct shel_pak *shell;

if (!canvas)
  return;
if (!data)
  return;

/* print mode */
text = get_mode_label(data);
pango_print(text, canvas->x+canvas->width-gl_text_width(text),
	     canvas->height-canvas->y-20, canvas, gl_fontsize, 0);
g_free(text);

/* print some useful info */
if (sysenv.render.show_energy)
  {
  text = property_lookup("Energy", data);
  if (text)
    pango_print(text, canvas->x+canvas->width-gl_text_width(text),
        canvas->y+2*gl_fontsize, canvas, gl_fontsize, 0);
  }

if (data->show_frame_number)
  {
  if (data->animation)
    {
    text = g_strdup_printf("[%d:%d]", data->cur_frame, data->num_frames-1);
    pango_print(text, (canvas->x+canvas->width-gl_text_width(text))/2,
	canvas->y+40, canvas, gl_fontsize, 0);
    g_free(text);
    }
  }

/* hkl labels */
/*
if (data->num_vertices && data->morph_label)
  gl_draw_miller(data);
*/

/* unit cell lengths */
if (data->show_cell_lengths)
  {
  j=2;
  for (i=0 ; i<data->periodic ; i++)
    {
    j += pow(-1, i) * (i+1);
    text = g_strdup_printf("%5.2f \u212B", data->pbc[i]);
    ARR3SET(v1, data->cell[0].rx);
    ARR3ADD(v1, data->cell[j].rx);
    VEC3MUL(v1, 0.5);
    pango_print_world(text, v1, canvas);
    g_free(text);
    }
  }

/* epot scale */
if (data->ms_colour_scale)
  gl_draw_colour_scale(canvas->x+1, sysenv.height - canvas->y - canvas->height + 80, data);

/* NEW - camera waypoint number */
if (data->show_waypoints && !data->animating)
  {
  i=0;
  glColor3f(0.0, 0.0, 1.0);
  for (list=data->waypoint_list ; list ; list=g_slist_next(list))
    {
    struct camera_pak *camera = list->data;
    i++;
    if (camera == data->camera)
      continue;

    text = g_strdup_printf("%d", i);
    pango_print_world(text, camera->x, canvas);
    g_free(text);
    }
  }

/* TODO - incorporate scaling etc. in camera waypoint drawing */
/* the following text is likely to have partial */
/* overlapping, so XOR text fragments for clearer display */
/* due to new text diplay, GL_XOR is no longer recommended --OVHPA */
//glEnable(GL_COLOR_LOGIC_OP);
//glLogicOp(GL_XOR);
//glColor4f(1.0, 1.0, 1.0, 1.0);
/* TODO - all atom related printing -> construct a string */
label = g_string_new(NULL);


if (data->show_selection_labels)
  list = data->selection;
else
  list = data->cores;

/*
for (list=data->selection ; list ; list=g_slist_next(list))
for (list=data->cores ; list ; list=g_slist_next(list))
*/

while (list)
  {
  core[0] = list->data;
  if (core[0]->status & (DELETED | HIDDEN))
    {
    list = g_slist_next(list);
    hidden += 1;
    continue;
    }

  label = g_string_assign(label, "");

/* show the order the atom was read in (start from 1 - helps with zmatrix debugging) */
  if (data->show_atom_index)
    {
    i = g_slist_index(data->cores, core[0]);
    g_string_append_printf(label, "[%d]", i+1);
    }

/* set up atom labels */
  if (data->show_atom_labels)
    {
    g_string_append_printf(label, "(%s)", core[0]->atom_label);
    }
  if (data->show_atom_types)
    {
    if (core[0]->atom_type)
      {
      g_string_append_printf(label, "(%s)", core[0]->atom_type);
      }
    else
      {
      g_string_append_printf(label, "(?)");
      }
    }

/*VZ*/
  if (data->show_nmr_shifts)
    {
    g_string_append_printf(label, "(%4.2f)", core[0]->atom_nmr_shift);
    }
  if (data->show_nmr_csa)
    {
    g_string_append_printf(label, "(%4.2f;", core[0]->atom_nmr_aniso);
    g_string_append_printf(label, "%4.2f)", core[0]->atom_nmr_asym);
    }
  if (data->show_nmr_efg)
    {
    g_string_append_printf(label, "(%g;", core[0]->atom_nmr_cq);
    g_string_append_printf(label, "%4.2f)", core[0]->atom_nmr_efgasym);
    }

/* get atom charge, add shell charge (if any) to get net result */
  if (data->show_atom_charges)
    {
    q = atom_charge(core[0]);
    g_string_append_printf(label, "{%6.4f}", q);
    }

  if (data->show_core_charges)
    {
    q = core[0]->charge;
    g_string_append_printf(label, "{%6.4f}", q);
    }

  if (data->show_shell_charges)
    {
    if (core[0]->shell)
      {
      shell = core[0]->shell;
      q = shell->charge;
      g_string_append_printf(label, "{%6.4f}", q);
      }
    else
      g_string_append_printf(label, "{}");
    }

/* print */
  if (label->str)
    {
    ARR3SET(v1, core[0]->rx);
    pango_print_world(label->str, v1, canvas);
    }

  list = g_slist_next(list); 
  }
g_string_free(label, TRUE);

if( hidden > 0)
  {
  n += 2;
  text = g_strdup_printf("hidden: %d", hidden);
  pango_print(text, canvas->x+20,
        canvas->height-canvas->y-n*gl_fontsize, canvas, gl_fontsize, 0);
  g_free(text);
  }

if( data->selection)
  {
  n += 2;
  text = g_strdup_printf("selected: %d", g_slist_length(data->selection));
  pango_print(text, canvas->x+20,
        canvas->height-canvas->y-n*gl_fontsize, canvas, gl_fontsize, 0);
  g_free(text);
  }

/* print current frame */
/* geom measurement labels */
if (data->show_geom_labels)
  for (list=data->measure_list ; list ; list=g_slist_next(list))
    {
    type = measure_type_get(list->data);
    switch(type)
      {
      case MEASURE_BOND:
      case MEASURE_DISTANCE:
      case MEASURE_INTER:
      case MEASURE_INTRA:
        measure_coord_get(v1, 0, list->data, data);
        measure_coord_get(v2, 1, list->data, data);
        ARR3ADD(v1, v2);
        VEC3MUL(v1, 0.5);
        pango_print_world(measure_value_get(list->data), v1, canvas);
        break;

      case MEASURE_ANGLE:
/* angle is i-j-k */
        measure_coord_get(v1, 0, list->data, data);
        measure_coord_get(v2, 1, list->data, data);
        measure_coord_get(v3, 2, list->data, data);
/* angle label */
/* FIXME - should use a similar process to the draw_arc code to */
/* determine which arm is shorter & use that to determine label position */
        ARR3ADD(v1, v2);
        ARR3ADD(v1, v3);
        VEC3MUL(v1, 0.3333);
        pango_print_world(measure_value_get(list->data), v1, canvas);
        break;
      }
    }

/* spatial object labels */
//glDisable(GL_COLOR_LOGIC_OP);
/* FIXME - need to change the variable name */
if (data->morph_label)
  for (list=data->spatial ; list ; list=g_slist_next(list))
    {
    spatial = list->data;
    if (spatial->show_label)
      {
      glColor4f(spatial->c[0], spatial->c[1], spatial->c[2], 1.0);
      v = g_slist_nth_data(spatial->list, 0);
      if (gl_visible(v->rn, data))
        pango_print_world(spatial->label, spatial->x, canvas);
      }
    }
}

/********************************/
/* draw a ribbon special object */
/********************************/
void gl_draw_ribbon(struct model_pak *data)
{
gdouble len, vec1[3], vec2[3];
GSList *list, *rlist;
struct ribbon_pak *ribbon;
struct object_pak *object;
GLfloat ctrl[8][3];

for (list=data->ribbons ; list ; list=g_slist_next(list))
  {
  object = list->data;

g_assert(object->type == RIBBON);

/* go through the ribbon segment list */
rlist = (GSList *) object->data;
while (rlist != NULL)
  {
  ribbon = rlist->data;

  glColor4f(ribbon->colour[0], ribbon->colour[1], ribbon->colour[2],
                                            sysenv.render.transmit);

/* end points */
  ARR3SET(&ctrl[0][0], ribbon->r1);
  ARR3SET(&ctrl[3][0], ribbon->r2);

/* get distance between ribbon points */
  ARR3SET(vec1, ribbon->r1);
  ARR3SUB(vec1, ribbon->r2);
  len = VEC3MAG(vec1);

/* shape control points */
  ARR3SET(&ctrl[1][0], ribbon->r1);
  ARR3SET(&ctrl[2][0], ribbon->r2);

/* segment length based curvature - controls how flat it is at the cyclic group */
  ARR3SET(vec1, ribbon->o1);
  VEC3MUL(vec1, len*sysenv.render.ribbon_curvature);
  ARR3ADD(&ctrl[1][0], vec1);
  ARR3SET(vec2, ribbon->o2);
  VEC3MUL(vec2, len*sysenv.render.ribbon_curvature);
  ARR3ADD(&ctrl[2][0], vec2);

/* compute offsets for ribbon thickness */
  crossprod(vec1, ribbon->n1, ribbon->o1);
  crossprod(vec2, ribbon->n2, ribbon->o2);
  normalize(vec1, 3);
  normalize(vec2, 3);

/* thickness vectors for the two ribbon endpoints */
  VEC3MUL(vec1, 0.5*sysenv.render.ribbon_thickness);
  VEC3MUL(vec2, 0.5*sysenv.render.ribbon_thickness);

/* ensure these are pointing the same way */
  if (via(vec1, vec2, 3) > PI/2.0)
    {
    VEC3MUL(vec2, -1.0);
    }

/* FIXME - 2D evaluators have mysteriously just stopped working */
/* FIXME - fedora / driver problem? */
/* FIXME - seems to be specific to jago (diff video card) -> update driver */
if (sysenv.render.wire_surface)
  {

/* CURRENT - exp using a 1D workaround (only use half the control points) */
glLineWidth(sysenv.render.ribbon_thickness);

glMap1f(GL_MAP1_VERTEX_3, 0.0, 1.0, 3, 4, &ctrl[0][0]);
glEnable(GL_MAP1_VERTEX_3);
glMapGrid1f(sysenv.render.ribbon_quality, 0.0, 1.0);
glEvalMesh1(GL_LINE, 0, sysenv.render.ribbon_quality);

/*
{
GLfloat f;
glBegin(GL_LINE_STRIP);
for (f=0.0 ; f<1.0 ; f+=0.01)
  {
  glEvalCoord1f(f);
  }
glEnd();
}
*/

  }
else
  {
/* init the bottom edge control points */
  ARR3SET(&ctrl[4][0], &ctrl[0][0]);
  ARR3SET(&ctrl[5][0], &ctrl[1][0]);
  ARR3SET(&ctrl[6][0], &ctrl[2][0]);
  ARR3SET(&ctrl[7][0], &ctrl[3][0]);
/* lift points to make the top edge */
  ARR3ADD(&ctrl[0][0], vec1);
  ARR3ADD(&ctrl[1][0], vec1);
  ARR3ADD(&ctrl[2][0], vec2);
  ARR3ADD(&ctrl[3][0], vec2);
/* lower points to make the bottom edge */
  ARR3SUB(&ctrl[4][0], vec1);
  ARR3SUB(&ctrl[5][0], vec1);
  ARR3SUB(&ctrl[6][0], vec2);
  ARR3SUB(&ctrl[7][0], vec2);
/* drawing */
/* CURRENT - 2D evaluators have mysteriously just stopped working */
  glMap2f(GL_MAP2_VERTEX_3, 
          0.0, 1.0, 3, 4,
          0.0, 1.0, 12, 2,
          &ctrl[0][0]);
  glEnable(GL_MAP2_VERTEX_3);
  glEnable(GL_AUTO_NORMAL);
  glMapGrid2f(sysenv.render.ribbon_quality, 0.0, 1.0, 3, 0.0, 1.0);
  glEvalMesh2(GL_FILL, 0, sysenv.render.ribbon_quality, 0, 3);
  }


  rlist = g_slist_next(rlist);
  }
  }
}

/**************************/
/* spatial object drawing */
/**************************/
/* NB: different setup for vectors/planes - draw in seperate iterations */
void gl_draw_spatial(gint material, struct model_pak *data)
{
gdouble size, v0[3], v1[3], v2[3], vi[3], mp[3], mn[3];
GSList *list, *list1, *list2, *ilist;
struct vec_pak *p1, *p2;
struct spatial_pak *spatial;
struct image_pak *image;
struct camera_pak *camera;

g_assert(data != NULL);

camera = data->camera;

g_assert(camera != NULL);

ARR3SET(v0, camera->v);
quat_rotate(v0, camera->q);

for (list=data->spatial ; list ; list=g_slist_next(list))
  {
  spatial = list->data;

/* NEW - normal check for wire frame and show hidden */
  if (sysenv.render.wire_surface && !sysenv.render.wire_show_hidden)
    {
    p1 = g_slist_nth_data(spatial->list, 0);

    if (via(v0, p1->rn, 3) < 0.5*PI)
      continue;
    }

  if (spatial->material == material)
    {
/* enumerate periodic images */
    ilist=NULL;
    do
      {
      if (ilist)
        {
/* image */
        image = ilist->data;
        ARR3SET(vi, image->rx);
        ilist = g_slist_next(ilist);
        }
      else
        {
/* original */
        VEC3SET(vi, 0.0, 0.0, 0.0);
        if (spatial->periodic)
          ilist = data->images;
        }

      switch (spatial->type)
        {
/* vector spatials (special object) */
        case SPATIAL_VECTOR:
          list1 = spatial->list;
          list2 = g_slist_next(list1);
          while (list1 && list2)
            {
            p1 = list1->data;
            p2 = list2->data;
            ARR3SET(v1, p1->rx);
            ARR3SET(v2, p2->rx);
            draw_vector(v1, v2, 0.04);
            list1 = g_slist_next(list2);
            list2 = g_slist_next(list1);
            }
          break;

/* generic spatials (method + vertices) */
        default:
size = 0.0;
VEC3SET(mp, 0.0, 0.0, 0.0);
VEC3SET(mn, 0.0, 0.0, 0.0);

//p1 = g_slist_nth_data(spatial->list, 0);/*FIX: 7d9c35*/

/* CURRENT */
/*
sysenv.render.wire_show_hidden
*/

          glBegin(spatial->method);
          for (list1=spatial->list ; list1 ; list1=g_slist_next(list1))
            {
            p1 = list1->data;
            ARR3SET(v1, p1->rx);
            ARR3ADD(v1, vi);

ARR3ADD(mp, v1);
ARR3SET(mn, p1->rn);
size++;

            glColor4f(p1->colour[0], p1->colour[1], p1->colour[2], sysenv.render.transmit);
            glNormal3dv(p1->rn);
            glVertex3dv(v1);
            }
          glEnd();

if (size > 0.1)
  {
  VEC3MUL(mp, 1.0/size);
  }
ARR3SET(spatial->x, mp);

          break;
        }
      }
    while (ilist);
    }
  }
}

/***************************************/
/* draw a picture in the OpenGL window */
/***************************************/
#if DRAW_PICTURE
void gl_picture_draw(struct canvas_pak *canvas, struct model_pak *model)
{
GdkPixbuf *raw_pixbuf, *scaled_pixbuf;
GError *error;

g_assert(model->picture_active != NULL);

/* read in the picture */
error = NULL;
raw_pixbuf = gdk_pixbuf_new_from_file(model->picture_active, &error);

/* error checking */
if (!raw_pixbuf)
  {
  if (error)
    {
    printf("%s\n", error->message);
    g_error_free(error);
    }
  else
    printf("Failed to load: %s\n", (gchar *) model->picture_active);
  return;
  }

/* scale and draw the picture */
scaled_pixbuf = gdk_pixbuf_scale_simple(raw_pixbuf,
                  canvas->width, canvas->height, GDK_INTERP_TILES);

gdis_gdk_draw_pixbuf(gtk_widget_get_window(canvas->glarea), scaled_pixbuf,
                     0, 0, 0, 0, canvas->width, canvas->height);
}
#endif

/*************************/
/* draw camera waypoints */
/*************************/
void gl_camera_waypoints_draw(struct model_pak *model)
{
gdouble x[3], o[3], e[3], b1[3], b2[3], b3[3], b4[4];
GSList *list;
struct point_pak sphere;

gl_init_sphere(&sphere, model);

for (list=model->waypoint_list ; list ; list=g_slist_next(list))
  {
  struct camera_pak *camera = list->data;
  if (camera == model->camera)
    continue;

/* offset the sphere so it doesn't interfere with viewing */
  ARR3SET(x, camera->v);
  VEC3MUL(x, -0.1);
  ARR3ADD(x, camera->x);
  glColor3f(0.5, 0.5, 0.5);
  gl_draw_sphere(&sphere, x, 0.1);

#define CONE_SIZE 0.3

/* get vector orthogonal to viewing and orientation vectors */
  ARR3SET(e, camera->e);
  VEC3MUL(e, CONE_SIZE);

/* compute base of the cone */
  ARR3SET(b1, camera->v);
  VEC3MUL(b1, CONE_SIZE);
  ARR3ADD(b1, camera->x);
  ARR3SET(b2, b1);
  ARR3SET(b3, b1);
  ARR3SET(b4, b1);
  ARR3SET(o, camera->o);
  VEC3MUL(o, CONE_SIZE);

/* compute 4 base points of the FOY square cone */
  ARR3ADD(b1, o);
  ARR3ADD(b1, e);
  ARR3ADD(b2, o);
  ARR3SUB(b2, e);
  ARR3SUB(b3, o);
  ARR3SUB(b3, e);
  ARR3SUB(b4, o);
  ARR3ADD(b4, e);

/* square cone = FOV */
  glBegin(GL_TRIANGLE_FAN);
  glVertex3dv(camera->x);
  glVertex3dv(b2);
  glVertex3dv(b3);
  glVertex3dv(b4);
  glVertex3dv(b1);
  glEnd();

/* complete the cone - different colour to indicate this is the UP orientation direction */
  glColor3f(1.0, 0.0, 0.0);
  glBegin(GL_TRIANGLES);
  glVertex3dv(camera->x);
  glVertex3dv(b1);
  glVertex3dv(b2);
  glEnd();
  }

gl_free_points(&sphere);
}

/************************/
/* main drawing routine */
/************************/
void draw_objs(struct canvas_pak *canvas, struct model_pak *data)
{
gint i;
gulong time;
gdouble r;
gfloat specular[4], fog[4];
gdouble fog_mark;
GSList *pipes[4];
GSList *solid=NULL, *ghost=NULL, *wire=NULL;

time = mytimer();

/* NEW - playing around with the idea of zone visibility */
/*
zone_visible_init(data);
*/

/* transformation recording */
if (data->mode == RECORD)
  {
  struct camera_pak *camera;

/* hack to prevent duplicate recording of the 1st frame */
/* FIXME - this will fail for multiple (concatenated) recordings */
  if (data->num_frames > 1)
    {
    camera = camera_dup(data->camera);
    data->transform_list = g_slist_append(data->transform_list, camera);
    }

  data->num_frames++;
  }

/* set up the lighting */
gl_init_lights(data);

/* set up the fontsize --OVHPA */
gl_get_fontsize();

/* scaling affects placement to avoid near/far clipping */
r = sysenv.rsize;

/* main drawing setup */
glFrontFace(GL_CCW);
glEnable(GL_LIGHTING);
glDepthMask(GL_TRUE);
glEnable(GL_DEPTH_TEST);
glDepthFunc(GL_LESS);
glEnable(GL_CULL_FACE);
glShadeModel(GL_SMOOTH);
glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
glLineWidth(sysenv.render.line_thickness);
glPointSize(2.0);

/* NEW - vertex arrarys */
#if VERTEX_ARRAYS
glEnableClientState(GL_VERTEX_ARRAY);
glEnableClientState(GL_NORMAL_ARRAY);
#endif

/* turn off antialiasing, in case it was previously on */
glDisable(GL_LINE_SMOOTH);
glDisable(GL_POINT_SMOOTH);
glDisable(GL_BLEND);
glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

/* depth queuing via fog */
/* FIXME - broken due to new camera code */
if (sysenv.render.fog)
  {
  glEnable(GL_FOG);

  ARR3SET(fog, sysenv.render.bg_colour);
  glFogfv(GL_FOG_COLOR, fog);

  glFogf(GL_FOG_MODE, GL_LINEAR);
  glHint(GL_FOG_HINT, GL_DONT_CARE);

/* NB: only does something if mode is EXP */
/*
  glFogf(GL_FOG_DENSITY, 0.1);
*/

  fog_mark = r;
  glFogf(GL_FOG_START, fog_mark);

  fog_mark += (1.0 - sysenv.render.fog_density) * 4.0*r;
  glFogf(GL_FOG_END, fog_mark);
  }
else
  glDisable(GL_FOG);

/* solid drawing */
glPolygonMode(GL_FRONT, GL_FILL);
glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);

/* material shininess control (atoms) */
VEC3SET(specular, sysenv.render.ahl_strength 
                , sysenv.render.ahl_strength 
                , sysenv.render.ahl_strength);
glMaterialf(GL_FRONT, GL_SHININESS, sysenv.render.ahl_size);
glMaterialfv(GL_FRONT, GL_SPECULAR, specular);

/* colour-only change for spheres */
glEnable(GL_COLOR_MATERIAL);
glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

/* TODO - speedup - if atoms are v small - disable lighting */
/* draw solid cores */
if (data->show_cores)
  gl_build_core_lists(&solid, &ghost, &wire, data);
gl_draw_cores(solid, data);

/* pipe lists should be build AFTER core list (OFF_SCREEN flag tests) */
render_make_pipes(pipes, data);

/* double sided drawing for all spatial objects */
glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
glDisable(GL_CULL_FACE);

/* draw solid bonds */
glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
gl_draw_pipes(FALSE, pipes[0], data);

/* material shininess control (surfaces) */
VEC3SET(specular, sysenv.render.shl_strength 
                , sysenv.render.shl_strength 
                , sysenv.render.shl_strength);
glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, sysenv.render.shl_size);
glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
glColor4f(sysenv.render.fg_colour[0], 
          sysenv.render.fg_colour[1],
          sysenv.render.fg_colour[2], 1.0);

/* draw camera waypoint locations */
if (data->show_waypoints && !data->animating)
  gl_camera_waypoints_draw(data);

gl_draw_spatial(SPATIAL_SOLID, data);

/* wire frame stuff */
glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
if (sysenv.render.antialias)
  {
  glEnable(GL_LINE_SMOOTH);
  glEnable(GL_POINT_SMOOTH);
  glEnable(GL_BLEND);
  }
gl_draw_cores(wire, data);

/* cancel wire frame if spatials are to be solid */
if (!sysenv.render.wire_surface)
  {
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

/* spatials are potentially translucent */
  if (sysenv.render.transmit < 0.99)
    {
/* alpha blending for translucent surfaces */
    glEnable(GL_BLEND);
/* NB: make depth buffer read only - for translucent objects */
/* see the red book p229 */
    glDepthMask(GL_FALSE);
    }
  }

/* NEW - all spatials drawn with this line thickness */
glLineWidth(sysenv.render.frame_thickness);

/* FIXME - translucent spatials are not drawn back to front & can look funny */
gl_draw_spatial(SPATIAL_SURFACE, data);

/* ensure solid surfaces from now on */
glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
if (data->show_axes)
  {
/* always draw axes polygon fragments */
  glDepthFunc(GL_ALWAYS);
  gl_draw_axes(TRUE, canvas, data);
  glDepthFunc(GL_LESS);
  }

/* enforce transparency */
glEnable(GL_BLEND);
glDepthMask(GL_FALSE);

/* translucent ghost atoms */
ghost = g_slist_sort(ghost, (gpointer) gl_depth_sort);
gl_draw_cores(ghost, data);

/* ghost bonds */
/* NB: back to front ordering */
pipes[1] = render_sort_pipes(pipes[1]);
gl_draw_pipes(FALSE, pipes[1], data);

/* translucent shells */
if (data->show_shells)
  gl_draw_shells(data);

/* CURRENT - ribbon drawing is broken */
gl_draw_ribbon(data);

/* halos */
glDisable(GL_LIGHTING);
glShadeModel(GL_FLAT);
gl_draw_halo_list(data->selection, data);

/* stop translucent drawing/depth buffering */
glDepthMask(GL_TRUE);

/* at this point, should have completed all colour_material routines */
/* ie only simple line/text drawing stuff after this point */
glEnable(GL_CULL_FACE);
glDisable(GL_COLOR_MATERIAL);

gl_draw_spatial(SPATIAL_LINE, data);

/* always visible foreground colour */
glColor4f(sysenv.render.fg_colour[0], 
          sysenv.render.fg_colour[1],
          sysenv.render.fg_colour[2], 1.0);

/* unit cell drawing */
if (data->show_cell && data->periodic)
  {
  glLineWidth(sysenv.render.frame_thickness);
  gl_draw_cell(data);
  }

/* draw wire/line bond types */
glLineWidth(sysenv.render.stick_thickness);
glColor4f(1.0, 0.85, 0.5, 1.0);
glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

/* wire frame cylinders bonds */
glDisable(GL_CULL_FACE);
gl_draw_pipes(FALSE, pipes[2], data);
glEnable(GL_CULL_FACE);

/* stick (line) bonds */
gl_draw_pipes(TRUE, pipes[3], data);

/* set up stippling */
glEnable(GL_LINE_STIPPLE);
glLineStipple(1, 0x0F0F);
glLineWidth(1.0);

/* periodic cell images */
glColor3f(0.8, 0.7, 0.6);
if (data->show_cell_images)
  gl_draw_cell_images(data);

/* enforce no depth queuing from now on */
glDisable(GL_FOG);

/* active/measurements colour (yellow) */
glColor4f(sysenv.render.label_colour[0],
          sysenv.render.label_colour[1],
          sysenv.render.label_colour[2], 1.0);

/* selection box */
glLineWidth(1.5);
if (data->box_on)
  gl_draw_box(data->select_box[0], data->select_box[1],
              data->select_box[2], data->select_box[3], canvas);

/* measurements */
glLineWidth(sysenv.render.geom_line_width);
gl_draw_measurements(data);

/* CURRENT - core/shell links */
if (data->show_links)
  gl_draw_links(data);

glDisable(GL_LINE_STIPPLE);

/* text drawing - accept all fragments */
glDepthFunc(GL_ALWAYS);
glColor4f(sysenv.render.fg_colour[0], 
          sysenv.render.fg_colour[1],
          sysenv.render.fg_colour[2], 1.0);
gl_draw_text(canvas, data);

if (data->show_axes)
  gl_draw_axes(FALSE, canvas, data);

/* free all core lists */
g_slist_free(solid);
g_slist_free(ghost);
g_slist_free(wire);

/* free all pipes */
for (i=4 ; i-- ; )
  free_slist(pipes[i]);

/* save timing info */
data->redraw_current = mytimer() - time;
data->redraw_cumulative += data->redraw_current;
data->redraw_count++;
}

/************************/
/* total canvas refresh */
/************************/
/*
void gl_draw(struct canvas_pak *canvas, struct model_pak *data)
{
gint flag;
gdouble sq, cq;
GdkGLContext *glcontext;
GdkGLDrawable *gldrawable;
PangoFontDescription *pfd;

if (!data)
  {
  gui_atom_widget_update(NULL, NULL);
  return;
  }

sq = sysenv.render.sphere_quality;
cq = sysenv.render.cylinder_quality;
if (sysenv.moving && sysenv.render.fast_rotation)
  {
  sysenv.render.sphere_quality = 1;
  sysenv.render.cylinder_quality = 5;
  }

sysenv.render.sphere_quality = sq;
sysenv.render.cylinder_quality = cq;
}
*/

/****************************************************************************/
/* timing analysis, returns true if redraw timeout was adjusted, else false */
/****************************************************************************/
#define DEBUG_CANVAS_TIMING 0
gint canvas_timing_adjust(struct model_pak *model)
{
static gint n=1;
gint m;
gdouble time;

/* timing analysis */
if (model->redraw_count >= 10)
  {
  time = model->redraw_cumulative;
  time /= model->redraw_count;
  time /= 1000.0;

#if DEBUG_CANVAS_TIMING
printf("[redraw] cumulative = %d us : average = %.1f ms : freq = %d x 25ms\n",
   model->redraw_cumulative, time, n);
#endif

  model->redraw_count = 0;
  model->redraw_cumulative = 0;

/* adjust redraw frequency? */
  m = 1 + time/25;
/* NEW - only increase if the difference is at least */
/* two greater to prevent flicking back and forth */
  if (m > n+1)
    {
    n = m;
#if DEBUG_CANVAS_TIMING
printf("increasing delay between redraw: %d\n", n);
#endif
    g_timeout_add(n*25, gui_canvas_handler, NULL);
    return(TRUE);
    }
  }
else
  {
/* redraw frequency test */
  if (n > 1 && model->redraw_count)
    {
    time = model->redraw_current;

    time *= 0.001;
    m = 1 + time/25;
/* adjust redraw frequency? */
    if (m < n)
      {
      n = m;
#if DEBUG_CANVAS_TIMING
printf("decreasing delay between redraw: %d : %dus)\n", n, model->redraw_current);
#endif
      g_timeout_add(n*25, gui_canvas_handler, NULL);
      return(TRUE);
      }
    }
  }
return(FALSE);
}

/************************************************************/
/* perform a snapshot of sysenv.glarea onto eps_file --OVHPA*/
/************************************************************/
void do_eps_snapshot(struct model_pak *model,gint w,gint h){
GdkWindow *drawable;
GdkPixbuf *pixbuf;
#ifdef CAIRO_HAS_PS_SURFACE
gchar *text;
cairo_t *cr;
cairo_surface_t *eps_surface;
#else
GError *error=NULL;
#endif //CAIRO_HAS_PS_SURFACE
/**/
if(model==NULL) return;
if(model->eps_file==NULL) return;
/*init pixbuf*/
drawable = gdis_gl_get_window(sysenv.glarea);
if (!drawable)
  return;
pixbuf = gdis_gdk_pixbuf_get_from_window(drawable, 0, 0, w, h);
if (!pixbuf)
  return;
#ifdef CAIRO_HAS_PS_SURFACE
eps_surface=cairo_ps_surface_create(model->eps_file,w,h);/*FIXME: w and h are in points*/
cairo_ps_surface_set_eps (eps_surface,TRUE);
/*comments*/
text=g_strdup_printf("%%%%Title: %s",model->basename);
cairo_ps_surface_dsc_comment (eps_surface,text);g_free(text);
text=g_strdup_printf("%%%%Software: GDIS %4.2f.%d (C) %d",VERSION,PATCH,YEAR);
cairo_ps_surface_dsc_comment (eps_surface,text);g_free(text);
/*end comments*/
cr = cairo_create (eps_surface);
gdk_cairo_set_source_pixbuf(cr,pixbuf,0,0);
cairo_paint (cr);
cairo_surface_flush (eps_surface);/*useful?*/
cairo_surface_destroy (eps_surface);
cairo_destroy (cr);
#else
if(sysenv.have_eps){
	gdk_pixbuf_save (pixbuf,model->eps_file,"eps",&error,NULL);/*probably not available*/
}else{
	gdk_pixbuf_save (pixbuf,model->eps_file,"png",&error,NULL);/*png is always possible*/
}
#endif //CAIRO_HAS_PS_SURFACE
g_object_unref (pixbuf);
model->snapshot_eps=FALSE;
}

#if GTK_MAJOR_VERSION >= 3
static void gl_core_mat4_identity(GLfloat *mat)
{
gint i;

for (i=0 ; i<16 ; i++)
  mat[i] = 0.0f;

mat[0] = 1.0f;
mat[5] = 1.0f;
mat[10] = 1.0f;
mat[15] = 1.0f;
}

static void gl_core_mat4_copy(GLdouble *dest, const GLfloat *src)
{
gint i;

for (i=0 ; i<16 ; i++)
  dest[i] = src[i];
}

static void gl_core_mat4_multiply(GLfloat *result,
                                  const GLfloat *lhs,
                                  const GLfloat *rhs)
{
gint row, col, k;
GLfloat tmp[16];

for (col=0 ; col<4 ; col++)
  {
  for (row=0 ; row<4 ; row++)
    {
    GLfloat sum = 0.0f;

    for (k=0 ; k<4 ; k++)
      sum += lhs[4*k + row] * rhs[4*col + k];

    tmp[4*col + row] = sum;
    }
  }

memcpy(result, tmp, sizeof(tmp));
}

static GLfloat gl_core_vec3_dot(const GLfloat *a, const GLfloat *b)
{
return(a[0]*b[0] + a[1]*b[1] + a[2]*b[2]);
}

static void gl_core_vec3_cross(GLfloat *result,
                               const GLfloat *a,
                               const GLfloat *b)
{
result[0] = a[1]*b[2] - a[2]*b[1];
result[1] = a[2]*b[0] - a[0]*b[2];
result[2] = a[0]*b[1] - a[1]*b[0];
}

static gboolean gl_core_vec3_normalize(GLfloat *vec)
{
gdouble len;

len = sqrt(gl_core_vec3_dot(vec, vec));
if (len < FRACTION_TOLERANCE)
  return(FALSE);

vec[0] /= len;
vec[1] /= len;
vec[2] /= len;

return(TRUE);
}

static gboolean gl_core_debug_enabled(void)
{
return(g_getenv("GDIS_DEBUG_GL_VERBOSE") != NULL);
}

static void gl_core_transform_point(const GLfloat *mat,
                                    const gdouble *point,
                                    GLfloat *clip)
{
gint row;

for (row=0 ; row<4 ; row++)
  {
  clip[row] = mat[row] * point[0]
            + mat[4 + row] * point[1]
            + mat[8 + row] * point[2]
            + mat[12 + row];
  }
}

static void gl_core_log_gl_error(const gchar *stage)
{
GLenum err;

if (!gl_core_debug_enabled())
  return;

err = glGetError();
if (err != GL_NO_ERROR)
  g_printerr("GDIS core debug: GL error 0x%x at %s\n", err, stage);
}

static void gl_core_clear_gl_errors(void)
{
if (!gl_core_debug_enabled())
  return;

while (glGetError() != GL_NO_ERROR)
  {
  }
}

static void gl_core_mat4_look_at(const gdouble *eye,
                                 const gdouble *center,
                                 const gdouble *up_hint,
                                 GLfloat *mat)
{
GLfloat forward[3], up[3], side[3], up_real[3];
GLfloat fallback[3];

forward[0] = center[0] - eye[0];
forward[1] = center[1] - eye[1];
forward[2] = center[2] - eye[2];
if (!gl_core_vec3_normalize(forward))
  {
  gl_core_mat4_identity(mat);
  return;
  }

up[0] = up_hint[0];
up[1] = up_hint[1];
up[2] = up_hint[2];
if (!gl_core_vec3_normalize(up))
  {
  up[0] = 0.0f;
  up[1] = 1.0f;
  up[2] = 0.0f;
  }

gl_core_vec3_cross(side, forward, up);
if (!gl_core_vec3_normalize(side))
  {
  fallback[0] = 0.0f;
  fallback[1] = 1.0f;
  fallback[2] = 0.0f;
  gl_core_vec3_cross(side, forward, fallback);
  if (!gl_core_vec3_normalize(side))
    {
    fallback[0] = 0.0f;
    fallback[1] = 0.0f;
    fallback[2] = 1.0f;
    gl_core_vec3_cross(side, forward, fallback);
    if (!gl_core_vec3_normalize(side))
      {
      gl_core_mat4_identity(mat);
      return;
      }
    }
  }

gl_core_vec3_cross(up_real, side, forward);
gl_core_mat4_identity(mat);

mat[0] = side[0];
mat[1] = up_real[0];
mat[2] = -forward[0];
mat[4] = side[1];
mat[5] = up_real[1];
mat[6] = -forward[1];
mat[8] = side[2];
mat[9] = up_real[2];
mat[10] = -forward[2];
mat[12] = -side[0]*eye[0] - side[1]*eye[1] - side[2]*eye[2];
mat[13] = -up_real[0]*eye[0] - up_real[1]*eye[1] - up_real[2]*eye[2];
mat[14] = forward[0]*eye[0] + forward[1]*eye[1] + forward[2]*eye[2];
}

static void gl_core_mat4_perspective(gdouble fov,
                                     gdouble aspect,
                                     gdouble near_clip,
                                     gdouble far_clip,
                                     GLfloat *mat)
{
gdouble scale;

gl_core_mat4_identity(mat);

scale = 1.0 / tan(0.5*D2R*fov);
mat[0] = scale / aspect;
mat[5] = scale;
mat[10] = (far_clip + near_clip) / (near_clip - far_clip);
mat[11] = -1.0f;
mat[14] = (2.0 * far_clip * near_clip) / (near_clip - far_clip);
mat[15] = 0.0f;
}

static void gl_core_mat4_ortho(gdouble left,
                               gdouble right,
                               gdouble bottom,
                               gdouble top,
                               gdouble near_clip,
                               gdouble far_clip,
                               GLfloat *mat)
{
gl_core_mat4_identity(mat);

mat[0] = 2.0 / (right - left);
mat[5] = 2.0 / (top - bottom);
mat[10] = -2.0 / (far_clip - near_clip);
mat[12] = -(right + left) / (right - left);
mat[13] = -(top + bottom) / (top - bottom);
mat[14] = -(far_clip + near_clip) / (far_clip - near_clip);
}

static void gl_core_prepare_matrices(struct canvas_pak *canvas,
                                     struct model_pak *model,
                                     GLfloat *modelview,
                                     GLfloat *projection,
                                     GLfloat *mvp)
{
gdouble a, r;
gdouble x[3], o[3], v[3];
struct camera_pak *camera;

g_assert(canvas != NULL);

glViewport(canvas->x, canvas->y, canvas->width, canvas->height);
canvas->viewport[0] = canvas->x;
canvas->viewport[1] = canvas->y;
canvas->viewport[2] = canvas->width;
canvas->viewport[3] = canvas->height;

gl_core_mat4_identity(modelview);
gl_core_mat4_identity(projection);
gl_core_mat4_identity(mvp);

if (!model)
  {
  gl_core_mat4_copy(canvas->modelview, modelview);
  gl_core_mat4_copy(canvas->projection, projection);
  return;
  }

g_assert(model->camera != NULL);
camera = model->camera;

r = 1.0 + model->rmax;
sysenv.rsize = r;

ARR3SET(x, camera->x);
ARR3SET(o, camera->o);
ARR3SET(v, camera->v);

switch (camera->mode)
  {
  case FREE:
    break;

  default:
  case LOCKED:
    quat_rotate(x, camera->q);
    quat_rotate(o, camera->q);
    quat_rotate(v, camera->q);
    break;
  }

ARR3ADD(v, x);
gl_core_mat4_look_at(x, v, o, modelview);

if (camera->zoom < 0.05)
  camera->zoom = 0.05;

if (canvas->height > 0)
  {
  a = canvas->width;
  a /= canvas->height;
  }
else
  {
  a = 1.0;
  }

sysenv.aspect = a;

if (camera->perspective)
  {
  gl_core_mat4_perspective(camera->fov, a, 0.1, 4.0*sysenv.rsize,
                           projection);
  }
else
  {
  r *= camera->zoom;
  if (a > 1.0)
    gl_core_mat4_ortho(-r*a, r*a, -r, r, 0.0, 4.0*sysenv.rsize,
                       projection);
  else
    gl_core_mat4_ortho(-r, r, -r/a, r/a, 0.0, 4.0*sysenv.rsize,
                       projection);
  }

gl_core_mat4_multiply(mvp, projection, modelview);
gl_core_mat4_copy(canvas->modelview, modelview);
gl_core_mat4_copy(canvas->projection, projection);

VEC3SET(&gl_acm[0], -1.0,  0.0,  0.0);
VEC3SET(&gl_acm[3],  0.0, -1.0,  0.0);
VEC3SET(&gl_acm[6],  0.0,  0.0, -1.0);
}

static void gl_core_release_geometry(void)
{
if (gl_core_renderer.vao && glIsVertexArray(gl_core_renderer.vao))
  glDeleteVertexArrays(1, &gl_core_renderer.vao);
if (gl_core_renderer.vbo && glIsBuffer(gl_core_renderer.vbo))
  glDeleteBuffers(1, &gl_core_renderer.vbo);
if (gl_core_renderer.ebo && glIsBuffer(gl_core_renderer.ebo))
  glDeleteBuffers(1, &gl_core_renderer.ebo);
if (gl_core_renderer.cylinder_vao &&
    glIsVertexArray(gl_core_renderer.cylinder_vao))
  glDeleteVertexArrays(1, &gl_core_renderer.cylinder_vao);
if (gl_core_renderer.cylinder_vbo &&
    glIsBuffer(gl_core_renderer.cylinder_vbo))
  glDeleteBuffers(1, &gl_core_renderer.cylinder_vbo);
if (gl_core_renderer.cylinder_ebo &&
    glIsBuffer(gl_core_renderer.cylinder_ebo))
  glDeleteBuffers(1, &gl_core_renderer.cylinder_ebo);
if (gl_core_renderer.line_vao && glIsVertexArray(gl_core_renderer.line_vao))
  glDeleteVertexArrays(1, &gl_core_renderer.line_vao);
if (gl_core_renderer.line_vbo && glIsBuffer(gl_core_renderer.line_vbo))
  glDeleteBuffers(1, &gl_core_renderer.line_vbo);
if (gl_core_renderer.surface_vao &&
    glIsVertexArray(gl_core_renderer.surface_vao))
  glDeleteVertexArrays(1, &gl_core_renderer.surface_vao);
if (gl_core_renderer.surface_vbo && glIsBuffer(gl_core_renderer.surface_vbo))
  glDeleteBuffers(1, &gl_core_renderer.surface_vbo);
if (gl_core_renderer.overlay_vao &&
    glIsVertexArray(gl_core_renderer.overlay_vao))
  glDeleteVertexArrays(1, &gl_core_renderer.overlay_vao);
if (gl_core_renderer.overlay_vbo &&
    glIsBuffer(gl_core_renderer.overlay_vbo))
  glDeleteBuffers(1, &gl_core_renderer.overlay_vbo);
if (gl_core_renderer.overlay_texture &&
    glIsTexture(gl_core_renderer.overlay_texture))
  glDeleteTextures(1, &gl_core_renderer.overlay_texture);

gl_core_renderer.vao = 0;
gl_core_renderer.vbo = 0;
gl_core_renderer.ebo = 0;
gl_core_renderer.cylinder_vao = 0;
gl_core_renderer.cylinder_vbo = 0;
gl_core_renderer.cylinder_ebo = 0;
gl_core_renderer.line_vao = 0;
gl_core_renderer.line_vbo = 0;
gl_core_renderer.surface_vao = 0;
gl_core_renderer.surface_vbo = 0;
gl_core_renderer.overlay_vao = 0;
gl_core_renderer.overlay_vbo = 0;
gl_core_renderer.overlay_texture = 0;
gl_core_renderer.overlay_width = 0;
gl_core_renderer.overlay_height = 0;

if (gl_core_renderer.sphere_mesh)
  {
  va_free(gl_core_renderer.sphere_mesh);
  gl_core_renderer.sphere_mesh = NULL;
  }
if (gl_core_renderer.cylinder_mesh)
  {
  va_free(gl_core_renderer.cylinder_mesh);
  gl_core_renderer.cylinder_mesh = NULL;
  }
}

static void gl_core_release_program(void)
{
if (gl_core_renderer.program && glIsProgram(gl_core_renderer.program))
  glDeleteProgram(gl_core_renderer.program);
if (gl_core_renderer.line_program && glIsProgram(gl_core_renderer.line_program))
  glDeleteProgram(gl_core_renderer.line_program);
if (gl_core_renderer.surface_program &&
    glIsProgram(gl_core_renderer.surface_program))
  glDeleteProgram(gl_core_renderer.surface_program);
if (gl_core_renderer.overlay_program &&
    glIsProgram(gl_core_renderer.overlay_program))
  glDeleteProgram(gl_core_renderer.overlay_program);

gl_core_renderer.program = 0;
gl_core_renderer.line_program = 0;
gl_core_renderer.surface_program = 0;
gl_core_renderer.overlay_program = 0;
gl_core_renderer.u_mvp = -1;
gl_core_renderer.u_colour = -1;
gl_core_renderer.u_line_mvp = -1;
gl_core_renderer.u_line_colour = -1;
gl_core_renderer.u_surface_mvp = -1;
gl_core_renderer.u_surface_alpha = -1;
gl_core_renderer.u_overlay_texture = -1;
}

static gboolean gl_core_shader_status(GLuint handle,
                                      gboolean is_program,
                                      const gchar *label)
{
GLint ok = GL_FALSE;
GLint len = 0;
gchar *log = NULL;

if (is_program)
  {
  glGetProgramiv(handle, GL_LINK_STATUS, &ok);
  if (ok == GL_TRUE)
    return(TRUE);
  glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &len);
  }
else
  {
  glGetShaderiv(handle, GL_COMPILE_STATUS, &ok);
  if (ok == GL_TRUE)
    return(TRUE);
  glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &len);
  }

if (len > 1)
  {
  log = g_malloc(len);
  if (is_program)
    glGetProgramInfoLog(handle, len, NULL, log);
  else
    glGetShaderInfoLog(handle, len, NULL, log);
  g_printerr("%s failed: %s\n", label, log);
  g_free(log);
  }
else
  g_printerr("%s failed.\n", label);

return(FALSE);
}

static gboolean gl_core_prepare_program(void)
{
GLuint program, vertex_shader, fragment_shader;
const gchar *vertex_source =
  "#version 150\n"
  "in vec3 a_position;\n"
  "in vec3 a_normal;\n"
  "uniform mat4 u_mvp;\n"
  "out vec3 v_normal;\n"
  "void main(void)\n"
  "{\n"
  "  v_normal = normalize(a_normal);\n"
  "  gl_Position = u_mvp * vec4(a_position, 1.0);\n"
  "}\n";
const gchar *fragment_source =
  "#version 150\n"
  "in vec3 v_normal;\n"
  "uniform vec4 u_colour;\n"
  "out vec4 fragColor;\n"
  "void main(void)\n"
  "{\n"
  "  vec3 light_dir = normalize(vec3(0.35, 0.55, 0.75));\n"
  "  float diffuse = max(dot(normalize(v_normal), light_dir), 0.0);\n"
  "  float lighting = 0.28 + 0.72 * diffuse;\n"
  "  fragColor = vec4(u_colour.rgb * lighting, u_colour.a);\n"
  "}\n";

if (gl_core_renderer.program && glIsProgram(gl_core_renderer.program))
  return(TRUE);

gl_core_release_program();

vertex_shader = glCreateShader(GL_VERTEX_SHADER);
glShaderSource(vertex_shader, 1, &vertex_source, NULL);
glCompileShader(vertex_shader);
if (!gl_core_shader_status(vertex_shader, FALSE, "Core vertex shader"))
  {
  glDeleteShader(vertex_shader);
  return(FALSE);
  }

fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
glShaderSource(fragment_shader, 1, &fragment_source, NULL);
glCompileShader(fragment_shader);
if (!gl_core_shader_status(fragment_shader, FALSE, "Core fragment shader"))
  {
  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);
  return(FALSE);
  }

program = glCreateProgram();
glAttachShader(program, vertex_shader);
glAttachShader(program, fragment_shader);
glBindAttribLocation(program, 0, "a_position");
glBindAttribLocation(program, 1, "a_normal");
glBindFragDataLocation(program, 0, "fragColor");
glLinkProgram(program);

glDeleteShader(vertex_shader);
glDeleteShader(fragment_shader);

if (!gl_core_shader_status(program, TRUE, "Core shader program"))
  {
  glDeleteProgram(program);
  return(FALSE);
  }

gl_core_renderer.program = program;
gl_core_renderer.u_mvp = glGetUniformLocation(program, "u_mvp");
gl_core_renderer.u_colour = glGetUniformLocation(program, "u_colour");

return(TRUE);
}

static gboolean gl_core_prepare_line_program(void)
{
GLuint program, vertex_shader, fragment_shader;
const gchar *vertex_source =
  "#version 150\n"
  "in vec3 a_position;\n"
  "uniform mat4 u_mvp;\n"
  "void main(void)\n"
  "{\n"
  "  gl_Position = u_mvp * vec4(a_position, 1.0);\n"
  "}\n";
const gchar *fragment_source =
  "#version 150\n"
  "uniform vec4 u_colour;\n"
  "out vec4 fragColor;\n"
  "void main(void)\n"
  "{\n"
  "  fragColor = u_colour;\n"
  "}\n";

if (gl_core_renderer.line_program && glIsProgram(gl_core_renderer.line_program))
  return(TRUE);

if (gl_core_renderer.line_program)
  glDeleteProgram(gl_core_renderer.line_program);

vertex_shader = glCreateShader(GL_VERTEX_SHADER);
glShaderSource(vertex_shader, 1, &vertex_source, NULL);
glCompileShader(vertex_shader);
if (!gl_core_shader_status(vertex_shader, FALSE, "Core line vertex shader"))
  {
  glDeleteShader(vertex_shader);
  return(FALSE);
  }

fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
glShaderSource(fragment_shader, 1, &fragment_source, NULL);
glCompileShader(fragment_shader);
if (!gl_core_shader_status(fragment_shader, FALSE, "Core line fragment shader"))
  {
  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);
  return(FALSE);
  }

program = glCreateProgram();
glAttachShader(program, vertex_shader);
glAttachShader(program, fragment_shader);
glBindAttribLocation(program, 0, "a_position");
glBindFragDataLocation(program, 0, "fragColor");
glLinkProgram(program);

glDeleteShader(vertex_shader);
glDeleteShader(fragment_shader);

if (!gl_core_shader_status(program, TRUE, "Core line shader program"))
  {
  glDeleteProgram(program);
  return(FALSE);
  }

gl_core_renderer.line_program = program;
gl_core_renderer.u_line_mvp = glGetUniformLocation(program, "u_mvp");
gl_core_renderer.u_line_colour = glGetUniformLocation(program, "u_colour");

return(TRUE);
}

static gboolean gl_core_prepare_surface_program(void)
{
GLuint program, vertex_shader, fragment_shader;
const gchar *vertex_source =
  "#version 150\n"
  "in vec3 a_position;\n"
  "in vec3 a_normal;\n"
  "in vec3 a_colour;\n"
  "uniform mat4 u_mvp;\n"
  "out vec3 v_normal;\n"
  "out vec3 v_colour;\n"
  "void main(void)\n"
  "{\n"
  "  v_normal = normalize(a_normal);\n"
  "  v_colour = a_colour;\n"
  "  gl_Position = u_mvp * vec4(a_position, 1.0);\n"
  "}\n";
const gchar *fragment_source =
  "#version 150\n"
  "in vec3 v_normal;\n"
  "in vec3 v_colour;\n"
  "uniform float u_alpha;\n"
  "out vec4 fragColor;\n"
  "void main(void)\n"
  "{\n"
  "  vec3 light_dir = normalize(vec3(0.35, 0.55, 0.75));\n"
  "  vec3 normal = normalize(v_normal);\n"
  "  if (!gl_FrontFacing)\n"
  "    normal = -normal;\n"
  "  float diffuse = max(dot(normal, light_dir), 0.0);\n"
  "  float lighting = 0.28 + 0.72 * diffuse;\n"
  "  fragColor = vec4(v_colour * lighting, u_alpha);\n"
  "}\n";

if (gl_core_renderer.surface_program &&
    glIsProgram(gl_core_renderer.surface_program))
  return(TRUE);

vertex_shader = glCreateShader(GL_VERTEX_SHADER);
glShaderSource(vertex_shader, 1, &vertex_source, NULL);
glCompileShader(vertex_shader);
if (!gl_core_shader_status(vertex_shader, FALSE, "Core surface vertex shader"))
  {
  glDeleteShader(vertex_shader);
  return(FALSE);
  }

fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
glShaderSource(fragment_shader, 1, &fragment_source, NULL);
glCompileShader(fragment_shader);
if (!gl_core_shader_status(fragment_shader, FALSE, "Core surface fragment shader"))
  {
  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);
  return(FALSE);
  }

program = glCreateProgram();
glAttachShader(program, vertex_shader);
glAttachShader(program, fragment_shader);
glBindAttribLocation(program, 0, "a_position");
glBindAttribLocation(program, 1, "a_normal");
glBindAttribLocation(program, 2, "a_colour");
glBindFragDataLocation(program, 0, "fragColor");
glLinkProgram(program);

glDeleteShader(vertex_shader);
glDeleteShader(fragment_shader);

if (!gl_core_shader_status(program, TRUE, "Core surface shader program"))
  {
  glDeleteProgram(program);
  return(FALSE);
  }

gl_core_renderer.surface_program = program;
gl_core_renderer.u_surface_mvp = glGetUniformLocation(program, "u_mvp");
gl_core_renderer.u_surface_alpha = glGetUniformLocation(program, "u_alpha");

return(TRUE);
}

static gboolean gl_core_prepare_overlay_program(void)
{
GLuint program, vertex_shader, fragment_shader;
const gchar *vertex_source =
  "#version 150\n"
  "in vec2 a_position;\n"
  "in vec2 a_texcoord;\n"
  "out vec2 v_texcoord;\n"
  "void main(void)\n"
  "{\n"
  "  v_texcoord = a_texcoord;\n"
  "  gl_Position = vec4(a_position, 0.0, 1.0);\n"
  "}\n";
const gchar *fragment_source =
  "#version 150\n"
  "in vec2 v_texcoord;\n"
  "uniform sampler2D u_texture;\n"
  "out vec4 fragColor;\n"
  "void main(void)\n"
  "{\n"
  "  fragColor = texture(u_texture, v_texcoord);\n"
  "}\n";

if (gl_core_renderer.overlay_program &&
    glIsProgram(gl_core_renderer.overlay_program))
  return(TRUE);

vertex_shader = glCreateShader(GL_VERTEX_SHADER);
glShaderSource(vertex_shader, 1, &vertex_source, NULL);
glCompileShader(vertex_shader);
if (!gl_core_shader_status(vertex_shader, FALSE, "Core overlay vertex shader"))
  {
  glDeleteShader(vertex_shader);
  return(FALSE);
  }

fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
glShaderSource(fragment_shader, 1, &fragment_source, NULL);
glCompileShader(fragment_shader);
if (!gl_core_shader_status(fragment_shader, FALSE,
                           "Core overlay fragment shader"))
  {
  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);
  return(FALSE);
  }

program = glCreateProgram();
glAttachShader(program, vertex_shader);
glAttachShader(program, fragment_shader);
glBindAttribLocation(program, 0, "a_position");
glBindAttribLocation(program, 1, "a_texcoord");
glBindFragDataLocation(program, 0, "fragColor");
glLinkProgram(program);

glDeleteShader(vertex_shader);
glDeleteShader(fragment_shader);

if (!gl_core_shader_status(program, TRUE, "Core overlay shader program"))
  {
  glDeleteProgram(program);
  return(FALSE);
  }

gl_core_renderer.overlay_program = program;
gl_core_renderer.u_overlay_texture = glGetUniformLocation(program,
                                                          "u_texture");

return(TRUE);
}

static gint gl_core_cylinder_quality(struct model_pak *model)
{
gint quality;

quality = nearest_int(sysenv.render.cylinder_quality);
if (quality < 3)
  quality = 3;

if (sysenv.render.auto_quality && model && model->rmax > FRACTION_TOLERANCE)
  {
  gint max;

  max = sysenv.size / (10.0 * model->rmax);
  max++;
  if (max < 3)
    max = 3;
  if (quality > max)
    quality = max;
  }

return(quality);
}

static gboolean gl_core_prepare_geometry(struct model_pak *model)
{
gint quality;
gint cylinder_quality;
gint vertices, indices;
gconstpointer vertex_data;
gconstpointer index_data;

quality = sysenv.render.sphere_quality;
cylinder_quality = gl_core_cylinder_quality(model);

if (gl_core_renderer.sphere_mesh &&
    (gl_core_renderer.sphere_quality != quality ||
     gl_core_renderer.cylinder_quality != cylinder_quality))
  gl_core_release_geometry();

if (!gl_core_renderer.sphere_mesh)
  {
  gl_core_renderer.sphere_mesh = va_init();
  va_make_sphere(gl_core_renderer.sphere_mesh);
  gl_core_renderer.sphere_quality = quality;
  }
if (!gl_core_renderer.cylinder_mesh)
  {
  gdouble saved_quality;

  gl_core_renderer.cylinder_mesh = va_init();
  saved_quality = sysenv.render.cylinder_quality;
  sysenv.render.cylinder_quality = cylinder_quality;
  va_make_cylinder(gl_core_renderer.cylinder_mesh);
  sysenv.render.cylinder_quality = saved_quality;
  gl_core_renderer.cylinder_quality = cylinder_quality;
  }

if (gl_core_renderer.vao && !glIsVertexArray(gl_core_renderer.vao))
  {
  gl_core_renderer.vao = 0;
  gl_core_renderer.vbo = 0;
  gl_core_renderer.ebo = 0;
  }

if (gl_core_renderer.vao)
  return(TRUE);

vertices = va_get_num_vertices(gl_core_renderer.sphere_mesh);
indices = va_get_num_indices(gl_core_renderer.sphere_mesh);
vertex_data = va_get_interleaved(gl_core_renderer.sphere_mesh);
index_data = va_get_indices(gl_core_renderer.sphere_mesh);

glGenVertexArrays(1, &gl_core_renderer.vao);
glBindVertexArray(gl_core_renderer.vao);

glGenBuffers(1, &gl_core_renderer.vbo);
glBindBuffer(GL_ARRAY_BUFFER, gl_core_renderer.vbo);
glBufferData(GL_ARRAY_BUFFER,
             6 * vertices * sizeof(GLfloat),
             vertex_data,
             GL_DYNAMIC_DRAW);

glGenBuffers(1, &gl_core_renderer.ebo);
glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl_core_renderer.ebo);
glBufferData(GL_ELEMENT_ARRAY_BUFFER,
             indices * sizeof(GLuint),
             index_data,
             GL_STATIC_DRAW);

glEnableVertexAttribArray(0);
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                      6*sizeof(GLfloat), (void *) 0);
glEnableVertexAttribArray(1);
glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                      6*sizeof(GLfloat), (void *) (3*sizeof(GLfloat)));

glBindVertexArray(0);
glBindBuffer(GL_ARRAY_BUFFER, 0);

return(TRUE);
}

static gboolean gl_core_prepare_cylinder_geometry(void)
{
gint vertices, indices;
gconstpointer vertex_data;
gconstpointer index_data;

if (gl_core_renderer.cylinder_vao &&
    !glIsVertexArray(gl_core_renderer.cylinder_vao))
  {
  gl_core_renderer.cylinder_vao = 0;
  gl_core_renderer.cylinder_vbo = 0;
  gl_core_renderer.cylinder_ebo = 0;
  }

if (gl_core_renderer.cylinder_vao)
  return(TRUE);

vertices = va_get_num_vertices(gl_core_renderer.cylinder_mesh);
indices = va_get_num_indices(gl_core_renderer.cylinder_mesh);
vertex_data = va_get_interleaved(gl_core_renderer.cylinder_mesh);
index_data = va_get_indices(gl_core_renderer.cylinder_mesh);

glGenVertexArrays(1, &gl_core_renderer.cylinder_vao);
glBindVertexArray(gl_core_renderer.cylinder_vao);

glGenBuffers(1, &gl_core_renderer.cylinder_vbo);
glBindBuffer(GL_ARRAY_BUFFER, gl_core_renderer.cylinder_vbo);
glBufferData(GL_ARRAY_BUFFER,
             6 * vertices * sizeof(GLfloat),
             vertex_data,
             GL_DYNAMIC_DRAW);

glGenBuffers(1, &gl_core_renderer.cylinder_ebo);
glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl_core_renderer.cylinder_ebo);
glBufferData(GL_ELEMENT_ARRAY_BUFFER,
             indices * sizeof(GLuint),
             index_data,
             GL_STATIC_DRAW);

glEnableVertexAttribArray(0);
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                      6*sizeof(GLfloat), (void *) 0);
glEnableVertexAttribArray(1);
glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                      6*sizeof(GLfloat), (void *) (3*sizeof(GLfloat)));

glBindVertexArray(0);
glBindBuffer(GL_ARRAY_BUFFER, 0);

return(TRUE);
}

static gboolean gl_core_prepare_line_geometry(void)
{
if (gl_core_renderer.line_vao && !glIsVertexArray(gl_core_renderer.line_vao))
  {
  gl_core_renderer.line_vao = 0;
  gl_core_renderer.line_vbo = 0;
  }

if (gl_core_renderer.line_vao)
  return(TRUE);

glGenVertexArrays(1, &gl_core_renderer.line_vao);
glBindVertexArray(gl_core_renderer.line_vao);

glGenBuffers(1, &gl_core_renderer.line_vbo);
glBindBuffer(GL_ARRAY_BUFFER, gl_core_renderer.line_vbo);
glBufferData(GL_ARRAY_BUFFER, 2 * 3 * sizeof(GLfloat), NULL, GL_DYNAMIC_DRAW);

glEnableVertexAttribArray(0);
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(GLfloat), (void *) 0);

glBindVertexArray(0);
glBindBuffer(GL_ARRAY_BUFFER, 0);

return(TRUE);
}

static gboolean gl_core_prepare_surface_geometry(void)
{
if (gl_core_renderer.surface_vao &&
    !glIsVertexArray(gl_core_renderer.surface_vao))
  {
  gl_core_renderer.surface_vao = 0;
  gl_core_renderer.surface_vbo = 0;
  }

if (gl_core_renderer.surface_vao)
  return(TRUE);

glGenVertexArrays(1, &gl_core_renderer.surface_vao);
glBindVertexArray(gl_core_renderer.surface_vao);

glGenBuffers(1, &gl_core_renderer.surface_vbo);
glBindBuffer(GL_ARRAY_BUFFER, gl_core_renderer.surface_vbo);
glBufferData(GL_ARRAY_BUFFER, 9 * sizeof(GLfloat), NULL, GL_DYNAMIC_DRAW);

glEnableVertexAttribArray(0);
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                      9*sizeof(GLfloat), (void *) 0);
glEnableVertexAttribArray(1);
glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                      9*sizeof(GLfloat), (void *) (3*sizeof(GLfloat)));
glEnableVertexAttribArray(2);
glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE,
                      9*sizeof(GLfloat), (void *) (6*sizeof(GLfloat)));

glBindVertexArray(0);
glBindBuffer(GL_ARRAY_BUFFER, 0);

return(TRUE);
}

static gboolean gl_core_prepare_overlay_geometry(void)
{
static const GLfloat quad[16] =
  {
  -1.0f,  1.0f, 0.0f, 0.0f,
  -1.0f, -1.0f, 0.0f, 1.0f,
   1.0f,  1.0f, 1.0f, 0.0f,
   1.0f, -1.0f, 1.0f, 1.0f
  };

if (gl_core_renderer.overlay_vao &&
    !glIsVertexArray(gl_core_renderer.overlay_vao))
  {
  gl_core_renderer.overlay_vao = 0;
  gl_core_renderer.overlay_vbo = 0;
  }

if (gl_core_renderer.overlay_vao)
  return(TRUE);

glGenVertexArrays(1, &gl_core_renderer.overlay_vao);
glBindVertexArray(gl_core_renderer.overlay_vao);

glGenBuffers(1, &gl_core_renderer.overlay_vbo);
glBindBuffer(GL_ARRAY_BUFFER, gl_core_renderer.overlay_vbo);
glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);

glEnableVertexAttribArray(0);
glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
                      4*sizeof(GLfloat), (void *) 0);
glEnableVertexAttribArray(1);
glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
                      4*sizeof(GLfloat), (void *) (2*sizeof(GLfloat)));

glBindVertexArray(0);
glBindBuffer(GL_ARRAY_BUFFER, 0);

return(TRUE);
}

static gboolean gl_core_prepare_overlay_texture(gint width,
                                                gint height)
{
if (width < 1 || height < 1)
  return(FALSE);

if (gl_core_renderer.overlay_texture &&
    !glIsTexture(gl_core_renderer.overlay_texture))
  {
  gl_core_renderer.overlay_texture = 0;
  gl_core_renderer.overlay_width = 0;
  gl_core_renderer.overlay_height = 0;
  }

if (!gl_core_renderer.overlay_texture)
  glGenTextures(1, &gl_core_renderer.overlay_texture);

glBindTexture(GL_TEXTURE_2D, gl_core_renderer.overlay_texture);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

if (gl_core_renderer.overlay_width != width ||
    gl_core_renderer.overlay_height != height)
  {
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0,
               GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  gl_core_renderer.overlay_width = width;
  gl_core_renderer.overlay_height = height;
  }

glBindTexture(GL_TEXTURE_2D, 0);

return(TRUE);
}

static gboolean gl_core_prepare(struct model_pak *model)
{
if (gl_core_renderer.program && !glIsProgram(gl_core_renderer.program))
  {
  gl_core_renderer.program = 0;
  gl_core_renderer.line_program = 0;
  gl_core_renderer.surface_program = 0;
  gl_core_renderer.overlay_program = 0;
  gl_core_renderer.u_mvp = -1;
  gl_core_renderer.u_colour = -1;
  gl_core_renderer.u_line_mvp = -1;
  gl_core_renderer.u_line_colour = -1;
  gl_core_renderer.u_surface_mvp = -1;
  gl_core_renderer.u_surface_alpha = -1;
  gl_core_renderer.u_overlay_texture = -1;
  gl_core_renderer.vao = 0;
  gl_core_renderer.vbo = 0;
  gl_core_renderer.ebo = 0;
  gl_core_renderer.cylinder_vao = 0;
  gl_core_renderer.cylinder_vbo = 0;
  gl_core_renderer.cylinder_ebo = 0;
  gl_core_renderer.line_vao = 0;
  gl_core_renderer.line_vbo = 0;
  gl_core_renderer.surface_vao = 0;
  gl_core_renderer.surface_vbo = 0;
  gl_core_renderer.overlay_vao = 0;
  gl_core_renderer.overlay_vbo = 0;
  gl_core_renderer.overlay_texture = 0;
  gl_core_renderer.overlay_width = 0;
  gl_core_renderer.overlay_height = 0;
  }

if (!gl_core_prepare_program())
  return(FALSE);
if (!gl_core_prepare_line_program())
  return(FALSE);
if (!gl_core_prepare_surface_program())
  return(FALSE);
if (!gl_core_prepare_overlay_program())
  return(FALSE);

if (!gl_core_prepare_geometry(model))
  return(FALSE);
if (!gl_core_prepare_cylinder_geometry())
  return(FALSE);
if (!gl_core_prepare_line_geometry())
  return(FALSE);
if (!gl_core_prepare_surface_geometry())
  return(FALSE);
if (!gl_core_prepare_overlay_geometry())
  return(FALSE);

gl_core_log_gl_error("core prepare");

return(TRUE);
}

static void gl_core_screen_vertex(struct canvas_pak *canvas,
                                  gdouble x,
                                  gdouble y,
                                  GLfloat *vertex)
{
gdouble local_x, local_y;

local_x = x - canvas->x;
local_y = y - canvas->y;

if (canvas->width > 0)
  vertex[0] = (GLfloat) (2.0*local_x / canvas->width - 1.0);
else
  vertex[0] = 0.0f;

if (canvas->height > 0)
  vertex[1] = (GLfloat) (1.0 - 2.0*local_y / canvas->height);
else
  vertex[1] = 0.0f;

vertex[2] = 0.0f;
}

static void gl_core_append_world_vertex(GArray *vertices, const gdouble *point)
{
GLfloat vertex[3];

vertex[0] = point[0];
vertex[1] = point[1];
vertex[2] = point[2];
g_array_append_vals(vertices, vertex, 3);
}

static void gl_core_append_world_segment(GArray *vertices,
                                         const gdouble *v1,
                                         const gdouble *v2)
{
gl_core_append_world_vertex(vertices, v1);
gl_core_append_world_vertex(vertices, v2);
}

static void gl_core_append_screen_segment(GArray *vertices,
                                          struct canvas_pak *canvas,
                                          gdouble x1,
                                          gdouble y1,
                                          gdouble x2,
                                          gdouble y2)
{
GLfloat vertex[3];

gl_core_screen_vertex(canvas, x1, y1, vertex);
g_array_append_vals(vertices, vertex, 3);
gl_core_screen_vertex(canvas, x2, y2, vertex);
g_array_append_vals(vertices, vertex, 3);
}

static void gl_core_overlay_fill_rect(gint x,
                                      gint y,
                                      gint width,
                                      gint height,
                                      const gdouble *colour)
{
cairo_t *cr;

if (!sysenv.cairo_surface || width <= 0 || height <= 0)
  return;

cairo_surface_flush(sysenv.cairo_surface);
cr = cairo_create(sysenv.cairo_surface);
if (!cr)
  return;

cairo_rectangle(cr, x, y, width, height);
cairo_set_source_rgb(cr, colour[0], colour[1], colour[2]);
cairo_fill(cr);
cairo_destroy(cr);
cairo_surface_mark_dirty(sysenv.cairo_surface);
}

static void gl_core_draw_line_vertices(const GLfloat *mvp,
                                       const GLfloat *vertices,
                                       guint num_vertices,
                                       const GLfloat *colour,
                                       gdouble line_width)
{
if (!vertices || !num_vertices)
  return;

glUseProgram(gl_core_renderer.line_program);
glUniformMatrix4fv(gl_core_renderer.u_line_mvp, 1, GL_FALSE, mvp);
glUniform4f(gl_core_renderer.u_line_colour,
            colour[0], colour[1], colour[2], colour[3]);
glBindVertexArray(gl_core_renderer.line_vao);
glBindBuffer(GL_ARRAY_BUFFER, gl_core_renderer.line_vbo);
glBufferData(GL_ARRAY_BUFFER,
             num_vertices * 3 * sizeof(GLfloat),
             vertices,
             GL_DYNAMIC_DRAW);

if (line_width > 0.0)
  glLineWidth(line_width);

glDrawArrays(GL_LINES, 0, num_vertices);

glBindVertexArray(0);
glUseProgram(0);
}

static void gl_core_append_surface_vertex(GArray *vertices,
                                          struct vec_pak *point,
                                          const gdouble *offset)
{
GLfloat vertex[9];

vertex[0] = point->rx[0] + offset[0];
vertex[1] = point->rx[1] + offset[1];
vertex[2] = point->rx[2] + offset[2];
vertex[3] = point->rn[0];
vertex[4] = point->rn[1];
vertex[5] = point->rn[2];
vertex[6] = point->colour[0];
vertex[7] = point->colour[1];
vertex[8] = point->colour[2];
g_array_append_vals(vertices, vertex, 9);
}

static void gl_core_append_surface_triangle(GArray *vertices,
                                            struct vec_pak *p1,
                                            struct vec_pak *p2,
                                            struct vec_pak *p3,
                                            const gdouble *offset)
{
gl_core_append_surface_vertex(vertices, p1, offset);
gl_core_append_surface_vertex(vertices, p2, offset);
gl_core_append_surface_vertex(vertices, p3, offset);
}

static void gl_core_draw_surface_vertices(const GLfloat *mvp,
                                          const GLfloat *vertices,
                                          guint num_vertices,
                                          gdouble alpha)
{
if (!vertices || !num_vertices)
  return;

glUseProgram(gl_core_renderer.surface_program);
glUniformMatrix4fv(gl_core_renderer.u_surface_mvp, 1, GL_FALSE, mvp);
glUniform1f(gl_core_renderer.u_surface_alpha, alpha);
glBindVertexArray(gl_core_renderer.surface_vao);
glBindBuffer(GL_ARRAY_BUFFER, gl_core_renderer.surface_vbo);
glBufferData(GL_ARRAY_BUFFER,
             num_vertices * 9 * sizeof(GLfloat),
             vertices,
             GL_DYNAMIC_DRAW);
glDrawArrays(GL_TRIANGLES, 0, num_vertices);
glBindVertexArray(0);
glUseProgram(0);
}

static void gl_core_graph_rgba(GLfloat *rgba, graph_color colour)
{
rgba[3] = 1.0f;

switch (colour)
  {
  case GRAPH_COLOR_BLACK:
    rgba[0] = 0.0f;
    rgba[1] = 0.0f;
    rgba[2] = 0.0f;
    break;
  case GRAPH_COLOR_WHITE:
    rgba[0] = 1.0f;
    rgba[1] = 1.0f;
    rgba[2] = 1.0f;
    break;
  case GRAPH_COLOR_BLUE:
    rgba[0] = 0.0f;
    rgba[1] = 0.0f;
    rgba[2] = 1.0f;
    break;
  case GRAPH_COLOR_GREEN:
    rgba[0] = 0.0f;
    rgba[1] = 0.5f;
    rgba[2] = 0.0f;
    break;
  case GRAPH_COLOR_RED:
    rgba[0] = 1.0f;
    rgba[1] = 0.0f;
    rgba[2] = 0.0f;
    break;
  case GRAPH_COLOR_YELLOW:
    rgba[0] = 1.0f;
    rgba[1] = 1.0f;
    rgba[2] = 0.0f;
    break;
  case GRAPH_COLOR_GRAY:
    rgba[0] = 0.5f;
    rgba[1] = 0.5f;
    rgba[2] = 0.5f;
    break;
  case GRAPH_COLOR_NAVY:
    rgba[0] = 0.0f;
    rgba[1] = 0.0f;
    rgba[2] = 0.5f;
    break;
  case GRAPH_COLOR_LIME:
    rgba[0] = 0.0f;
    rgba[1] = 1.0f;
    rgba[2] = 0.0f;
    break;
  case GRAPH_COLOR_TEAL:
    rgba[0] = 0.0f;
    rgba[1] = 0.5f;
    rgba[2] = 0.5f;
    break;
  case GRAPH_COLOR_AQUA:
    rgba[0] = 0.0f;
    rgba[1] = 1.0f;
    rgba[2] = 1.0f;
    break;
  case GRAPH_COLOR_MAROON:
    rgba[0] = 0.5f;
    rgba[1] = 0.0f;
    rgba[2] = 0.0f;
    break;
  case GRAPH_COLOR_PURPLE:
    rgba[0] = 0.5f;
    rgba[1] = 0.0f;
    rgba[2] = 0.5f;
    break;
  case GRAPH_COLOR_OLIVE:
    rgba[0] = 0.5f;
    rgba[1] = 0.5f;
    rgba[2] = 0.0f;
    break;
  case GRAPH_COLOR_SILVER:
    rgba[0] = 0.75f;
    rgba[1] = 0.75f;
    rgba[2] = 0.75f;
    break;
  case GRAPH_COLOR_FUSHIA:
    rgba[0] = 1.0f;
    rgba[1] = 0.0f;
    rgba[2] = 1.0f;
    break;
  case GRAPH_COLOR_DEFAULT:
  default:
    rgba[0] = sysenv.render.title_colour[0];
    rgba[1] = sysenv.render.title_colour[1];
    rgba[2] = sysenv.render.title_colour[2];
    break;
  }
}

static void gl_core_graph_layout(struct canvas_pak *canvas,
                                 struct graph_pak *graph,
                                 gdouble *ox,
                                 gdouble *oy,
                                 gdouble *dx,
                                 gdouble *dy)
{
*ox = canvas->x + 4*gl_fontsize;
if (graph->ylabel)
  *ox += 2*gl_fontsize;

*oy = canvas->y + canvas->height - 2*gl_fontsize;
if (graph->xlabel)
  *oy -= 2*gl_fontsize;

*dy = canvas->height - 8.0*gl_fontsize;
*dx = canvas->width - 2.0*(*ox);

if (graph->title)
  *dy = canvas->height - 10.0*gl_fontsize;
if (graph->x_title)
  *oy = canvas->y + canvas->height - 5*gl_fontsize;

if (*dx < 1.0)
  *dx = 1.0;
if (*dy < 1.0)
  *dy = 1.0;
}

static gdouble gl_core_graph_map_x(struct graph_pak *graph,
                                   gdouble ox,
                                   gdouble dx,
                                   gdouble x)
{
gdouble range;

range = graph->xmax - graph->xmin;
if (fabs(range) < FRACTION_TOLERANCE)
  range = 1.0;

x -= graph->xmin;
x /= range;

return(ox + x*dx);
}

static gdouble gl_core_graph_map_y(struct graph_pak *graph,
                                   gdouble oy,
                                   gdouble dy,
                                   gdouble y)
{
gdouble range;

range = graph->ymax - graph->ymin;
if (fabs(range) < FRACTION_TOLERANCE)
  range = 1.0;

y -= graph->ymin;
y /= range;

return(oy - y*dy);
}

static gboolean gl_core_graph_selected_point(struct graph_pak *graph,
                                             gdouble *x,
                                             gdouble *y)
{
g_data_x *gx;

if (!graph || graph->select < 0)
  return(FALSE);

gx = graph->set_list ? graph->set_list->data : NULL;

switch (graph->type)
  {
  case GRAPH_IY_TYPE:
  case GRAPH_XY_TYPE:
  case GRAPH_IX_TYPE:
  case GRAPH_XX_TYPE:
    if (!gx || graph->select >= gx->x_size)
      return(FALSE);
    *x = gx->x[graph->select];
    *y = graph->select_2;
    return(TRUE);

  case GRAPH_YX_TYPE:
    if (!gx || graph->select >= gx->x_size)
      return(FALSE);
    *x = graph->select_2;
    *y = gx->x[graph->select];
    return(TRUE);

  default:
    if (graph->size <= 1)
      return(FALSE);
    *x = (gdouble) graph->select / (graph->size-1);
    *x *= (graph->xmax - graph->xmin);
    *x += graph->xmin;
    *y = graph->select_2;
    return(TRUE);
  }
}

static void gl_core_draw_colour_scale_overlay(struct canvas_pak *canvas,
                                              gint x,
                                              gint y,
                                              struct model_pak *data)
{
gint i, n;
gdouble z1, dz;
gdouble colour[3];
GString *text;

if (!canvas || !data || !sysenv.cairo_surface)
  return;

n = data->epot_div;
if (n < 2)
  return;

z1 = data->epot_max;
dz = (data->epot_max - data->epot_min) / (gdouble) (n-1);

for (i=0 ; i<n ; i++)
  {
  switch (data->ms_colour_method)
    {
    case MS_SOLVENT:
      ms_dock_colour(colour, z1, data->epot_min, data->epot_max);
      break;

    default:
      ms_epot_colour(colour, z1, data->epot_min, data->epot_max);
      break;
    }

  gl_core_overlay_fill_rect(x, y+i*20, 19, 19, colour);
  z1 -= dz;
  }

text = g_string_new(NULL);
z1 = data->epot_max;
for (i=0 ; i<n ; i++)
  {
  g_string_printf(text, "%6.2f", z1);
  pango_print(text->str, x+30, y+i*20+18, canvas, gl_fontsize, 0);
  z1 -= dz;
  }
g_string_free(text, TRUE);
}

static void gl_core_draw_graph_text(struct canvas_pak *canvas,
                                    struct graph_pak *graph)
{
gint i, x, y, xoff;
gchar *text;
gdouble xf, yf, ox, oy, sx, dx, dy;
gdouble sel_x, sel_y;

if (!canvas || !graph || !sysenv.cairo_surface)
  return;

gl_core_graph_layout(canvas, graph, &ox, &oy, &dx, &dy);

for (i=0 ; i<graph->xticks ; i++)
  {
  xf = (graph->xticks > 1) ? (gdouble) i / (graph->xticks-1) : 0.0;
  x = ox + xf*dx;

  if (!graph->xlabel)
    continue;

  xf *= (graph->xmax - graph->xmin);
  xf += graph->xmin;
  if (graph->type == GRAPH_IY_TYPE || graph->type == GRAPH_IX_TYPE)
    {
    text = g_strdup_printf("%i", (gint) xf);
    pango_print(text, x, oy+gl_fontsize, canvas, gl_fontsize-2, 0);
    }
  else
    {
    text = g_strdup_printf("%.2f", xf);
    pango_print(text, x-2*gl_fontsize, oy+gl_fontsize, canvas,
                gl_fontsize-2, 0);
    }
  g_free(text);
  }

for (i=0 ; i<graph->yticks ; i++)
  {
  yf = (graph->yticks > 1) ? (gdouble) i / (graph->yticks-1) : 0.0;
  y = oy - yf*dy;

  if (!graph->ylabel)
    continue;

  yf *= (graph->ymax - graph->ymin);
  yf += graph->ymin;
  if (graph->ymax > 999.999999)
    text = g_strdup_printf("%.2e", yf);
  else
    text = g_strdup_printf("%7.2f", yf);
  pango_print(text, 2*gl_fontsize, y-1, canvas, gl_fontsize-2, 0);
  g_free(text);
  }

if (graph->select >= 0 &&
    graph->select_label &&
    gl_core_graph_selected_point(graph, &sel_x, &sel_y))
  {
  sx = gl_core_graph_map_x(graph, ox, dx, sel_x);
  xoff = strlen(graph->select_label);
  xoff *= gl_fontsize;
  xoff /= 4;
  pango_print(graph->select_label, sx-xoff, 0, canvas, gl_fontsize-2, 0);
  }

if (graph->title)
  pango_print(graph->title, ox+2, oy-(gint)dy-4*gl_fontsize, canvas,
              gl_fontsize, 0);
if (graph->sub_title)
  pango_print(graph->sub_title, ox+0.5*dx+2, oy-dy-2*gl_fontsize, canvas,
              gl_fontsize, 0);
if (graph->x_title)
  {
  if (graph->type == GRAPH_YX_TYPE)
    {
    gchar *ptr, *ptr2;

    ptr = g_strdup(graph->x_title);
    ptr2 = ptr;
    while ((*ptr2 != '\t') && (*ptr2 != '\0'))
      ptr2++;
    if (*ptr2 != '\0')
      {
      *ptr2 = '\0';
      ptr2++;
      pango_print(ptr, ox+2+gl_fontsize, oy+2*gl_fontsize, canvas,
                  gl_fontsize, 0);
      pango_print(ptr2, ox+0.5*dx+2, oy+2*gl_fontsize, canvas,
                  gl_fontsize, 0);
      }
    else
      {
      pango_print(ptr, ox+2, oy+2*gl_fontsize, canvas, gl_fontsize, 0);
      }
    g_free(ptr);
    }
  else
    {
    pango_print(graph->x_title, ox+0.33*dx+2, oy+2*gl_fontsize, canvas,
                gl_fontsize, 0);
    }
  }
if (graph->y_title)
  pango_print(graph->y_title, 0+0.5*gl_fontsize, oy-0.33*dy, canvas,
              gl_fontsize, 90);
}

static void gl_core_draw_model_overlay(struct canvas_pak *canvas,
                                       struct model_pak *data)
{
if (!canvas || !data || !sysenv.cairo_surface)
  return;

gl_draw_text(canvas, data);
if (data->show_axes)
  {
  gdouble origin[3], axis[3], scale, guide[3];
  gchar label[3];
  gint ry, i;

  if (data->axes_type == CARTESIAN)
    strcpy(label, " x");
  else
    strcpy(label, " a");

  ry = sysenv.height - canvas->y - canvas->height + 40;
  gl_project(origin, canvas->x+40, ry, canvas);
  gl_project(guide, canvas->x+20, ry, canvas);
  ARR3SUB(guide, origin);

  if (data->rmax > FRACTION_TOLERANCE)
    {
    scale = 20.0 * VEC3MAG(guide) / data->rmax;
    for (i=0 ; i<3 ; i++)
      {
      ARR3SET(axis, data->axes[i].rx);
      VEC3MUL(axis, scale);
      ARR3ADD(axis, origin);
      pango_print_world(label, axis, canvas);
      label[1]++;
      }
    }
  }
}

static void gl_core_draw_overlay_texture(const guchar *surface_data,
                                         gint width,
                                         gint height)
{
gsize pixels;
guchar *rgba_data;

if (!surface_data || width < 1 || height < 1)
  return;

pixels = width * height;
rgba_data = g_malloc(4 * pixels * sizeof(guchar));
if (!rgba_data)
  return;

for (gsize i=0 ; i<pixels ; i++)
  {
  rgba_data[4*i] = surface_data[4*i + 2];
  rgba_data[4*i + 1] = surface_data[4*i + 1];
  rgba_data[4*i + 2] = surface_data[4*i];
  rgba_data[4*i + 3] = surface_data[4*i + 3];
  }

gl_core_clear_gl_errors();

glDisable(GL_DEPTH_TEST);
glDepthMask(GL_FALSE);
glDisable(GL_CULL_FACE);
glEnable(GL_BLEND);
glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
glViewport(0, 0, width, height);

if (!gl_core_prepare_overlay_texture(width, height))
  {
  g_free(rgba_data);
  return;
  }

glUseProgram(gl_core_renderer.overlay_program);
gl_core_log_gl_error("overlay use program");
glBindVertexArray(gl_core_renderer.overlay_vao);
gl_core_log_gl_error("overlay bind vao");
glActiveTexture(GL_TEXTURE0);
glBindTexture(GL_TEXTURE_2D, gl_core_renderer.overlay_texture);
glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0,
             GL_RGBA, GL_UNSIGNED_BYTE, rgba_data);
gl_core_log_gl_error("overlay tex upload");
glUniform1i(gl_core_renderer.u_overlay_texture, 0);
glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
gl_core_log_gl_error("overlay quad draw");
glBindTexture(GL_TEXTURE_2D, 0);
glBindVertexArray(0);
glUseProgram(0);

glDepthMask(GL_TRUE);
glDisable(GL_BLEND);
g_free(rgba_data);
}

static void gl_core_graph_append_symbol(GArray *segments,
                                        struct canvas_pak *canvas,
                                        graph_symbol symbol,
                                        gdouble x,
                                        gdouble y)
{
switch (symbol)
  {
  case GRAPH_SYMB_SQUARE:
    gl_core_append_screen_segment(segments, canvas, x-2, y-2, x+2, y-2);
    gl_core_append_screen_segment(segments, canvas, x+2, y-2, x+2, y+2);
    gl_core_append_screen_segment(segments, canvas, x+2, y+2, x-2, y+2);
    gl_core_append_screen_segment(segments, canvas, x-2, y+2, x-2, y-2);
    break;
  case GRAPH_SYMB_CROSS:
    gl_core_append_screen_segment(segments, canvas, x-2, y-2, x+2, y+2);
    gl_core_append_screen_segment(segments, canvas, x+2, y-2, x-2, y+2);
    break;
  case GRAPH_SYMB_TRI_DN:
    gl_core_append_screen_segment(segments, canvas, x, y-3, x-3, y+3);
    gl_core_append_screen_segment(segments, canvas, x-3, y+3, x+3, y+3);
    gl_core_append_screen_segment(segments, canvas, x+3, y+3, x, y-3);
    break;
  case GRAPH_SYMB_TRI_UP:
    gl_core_append_screen_segment(segments, canvas, x, y+3, x-3, y-3);
    gl_core_append_screen_segment(segments, canvas, x-3, y-3, x+3, y-3);
    gl_core_append_screen_segment(segments, canvas, x+3, y-3, x, y+3);
    break;
  case GRAPH_SYMB_DIAM:
    gl_core_append_screen_segment(segments, canvas, x, y+5, x-5, y);
    gl_core_append_screen_segment(segments, canvas, x-5, y, x, y-5);
    gl_core_append_screen_segment(segments, canvas, x, y-5, x+5, y);
    gl_core_append_screen_segment(segments, canvas, x+5, y, x, y+5);
    break;
  case GRAPH_SYMB_NONE:
  default:
    break;
  }
}

static void gl_core_draw_spatial_surfaces(struct model_pak *model,
                                          gint material,
                                          const GLfloat *mvp)
{
GSList *list, *ilist, *vlist;
struct spatial_pak *spatial;
struct image_pak *image;
gdouble offset[3], centroid[3];
gdouble alpha;
gint centroid_count;

if (!model->spatial)
  return;

alpha = (material == SPATIAL_SURFACE) ? sysenv.render.transmit : 1.0;

if (alpha < 0.99)
  {
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDepthMask(GL_FALSE);
  }

glDisable(GL_CULL_FACE);

for (list=model->spatial ; list ; list=g_slist_next(list))
  {
  GArray *vertices;

  spatial = list->data;

  if (spatial->material != material)
    continue;
  if (spatial->type == SPATIAL_VECTOR)
    continue;
  if (spatial->method != GL_TRIANGLES &&
      spatial->method != GL_QUADS &&
      spatial->method != GL_POLYGON)
    continue;

  ilist = NULL;
  do
    {
    struct vec_pak *p1, *p2, *p3, *p4;
    gboolean original;

    if (ilist)
      {
      image = ilist->data;
      ARR3SET(offset, image->rx);
      ilist = g_slist_next(ilist);
      original = FALSE;
      }
    else
      {
      VEC3SET(offset, 0.0, 0.0, 0.0);
      if (spatial->periodic)
        ilist = model->images;
      original = TRUE;
      }

    vertices = g_array_new(FALSE, FALSE, sizeof(GLfloat));
    VEC3SET(centroid, 0.0, 0.0, 0.0);
    centroid_count = 0;

    switch (spatial->method)
      {
      case GL_TRIANGLES:
        vlist = spatial->list;
        while (vlist && g_slist_next(vlist) && g_slist_next(g_slist_next(vlist)))
          {
          p1 = vlist->data;
          p2 = g_slist_next(vlist)->data;
          p3 = g_slist_next(g_slist_next(vlist))->data;
          gl_core_append_surface_triangle(vertices, p1, p2, p3, offset);
          if (original)
            {
            ARR3ADD(centroid, p1->rx);
            ARR3ADD(centroid, p2->rx);
            ARR3ADD(centroid, p3->rx);
            centroid_count += 3;
            }
          vlist = g_slist_next(g_slist_next(g_slist_next(vlist)));
          }
        break;

      case GL_QUADS:
        vlist = spatial->list;
        while (vlist &&
               g_slist_next(vlist) &&
               g_slist_next(g_slist_next(vlist)) &&
               g_slist_next(g_slist_next(g_slist_next(vlist))))
          {
          p1 = vlist->data;
          p2 = g_slist_next(vlist)->data;
          p3 = g_slist_next(g_slist_next(vlist))->data;
          p4 = g_slist_next(g_slist_next(g_slist_next(vlist)))->data;
          gl_core_append_surface_triangle(vertices, p1, p2, p3, offset);
          gl_core_append_surface_triangle(vertices, p1, p3, p4, offset);
          if (original)
            {
            ARR3ADD(centroid, p1->rx);
            ARR3ADD(centroid, p2->rx);
            ARR3ADD(centroid, p3->rx);
            ARR3ADD(centroid, p4->rx);
            centroid_count += 4;
            }
          vlist = g_slist_next(g_slist_next(g_slist_next(g_slist_next(vlist))));
          }
        break;

      case GL_POLYGON:
        p1 = g_slist_nth_data(spatial->list, 0);
        p2 = g_slist_nth_data(spatial->list, 1);
        vlist = g_slist_nth(spatial->list, 2);
        while (p1 && p2 && vlist)
          {
          p3 = vlist->data;
          gl_core_append_surface_triangle(vertices, p1, p2, p3, offset);
          if (original)
            {
            ARR3ADD(centroid, p3->rx);
            centroid_count++;
            }
          p2 = p3;
          vlist = g_slist_next(vlist);
          }
        if (original && p1 && p2)
          {
          ARR3ADD(centroid, p1->rx);
          ARR3ADD(centroid, p2->rx);
          centroid_count += 2;
          }
        break;

      default:
        break;
      }

    if (vertices->len)
      gl_core_draw_surface_vertices(mvp,
                                    (const GLfloat *) vertices->data,
                                    vertices->len / 9,
                                    alpha);
    g_array_free(vertices, TRUE);
    }
  while (ilist);

  if (centroid_count > 0)
    {
    VEC3MUL(centroid, 1.0/centroid_count);
    ARR3SET(spatial->x, centroid);
    }
  }

if (alpha < 0.99)
  {
  glDepthMask(GL_TRUE);
  glDisable(GL_BLEND);
  }
}

static void gl_core_draw_spatial_lines(struct model_pak *model,
                                       const GLfloat *mvp)
{
GSList *list, *ilist, *vlist;
struct spatial_pak *spatial;
struct image_pak *image;

if (!model->spatial)
  return;

for (list=model->spatial ; list ; list=g_slist_next(list))
  {
  GArray *segments;
  GLfloat colour[4];
  gdouble offset[3], v1[3], v2[3];
  struct vec_pak *first;
  gboolean draw_as_lines;

  spatial = list->data;
  draw_as_lines = (spatial->type == SPATIAL_VECTOR ||
                   spatial->material == SPATIAL_LINE ||
                   spatial->method == GL_LINE_LOOP ||
                   spatial->method == GL_LINE_STRIP);
  if (!draw_as_lines)
    continue;

  first = g_slist_nth_data(spatial->list, 0);
  if (first)
    {
    colour[0] = first->colour[0];
    colour[1] = first->colour[1];
    colour[2] = first->colour[2];
    }
  else
    {
    colour[0] = sysenv.render.fg_colour[0];
    colour[1] = sysenv.render.fg_colour[1];
    colour[2] = sysenv.render.fg_colour[2];
    }
  colour[3] = 1.0f;

  ilist = NULL;
  do
    {
    if (ilist)
      {
      image = ilist->data;
      ARR3SET(offset, image->rx);
      ilist = g_slist_next(ilist);
      }
    else
      {
      VEC3SET(offset, 0.0, 0.0, 0.0);
      if (spatial->periodic)
        ilist = model->images;
      }

    segments = g_array_new(FALSE, FALSE, sizeof(GLfloat));

    if (spatial->type == SPATIAL_VECTOR)
      {
      vlist = spatial->list;
      while (vlist && g_slist_next(vlist))
        {
        struct vec_pak *p1, *p2;

        p1 = vlist->data;
        p2 = g_slist_next(vlist)->data;
        ARR3SET(v1, p1->rx);
        ARR3SET(v2, p2->rx);
        ARR3ADD(v1, offset);
        ARR3ADD(v2, offset);
        gl_core_append_world_segment(segments, v1, v2);
        vlist = g_slist_next(g_slist_next(vlist));
        }
      }
    else if (spatial->method == GL_LINES)
      {
      vlist = spatial->list;
      while (vlist && g_slist_next(vlist))
        {
        struct vec_pak *p1, *p2;

        p1 = vlist->data;
        p2 = g_slist_next(vlist)->data;
        ARR3SET(v1, p1->rx);
        ARR3SET(v2, p2->rx);
        ARR3ADD(v1, offset);
        ARR3ADD(v2, offset);
        gl_core_append_world_segment(segments, v1, v2);
        vlist = g_slist_next(g_slist_next(vlist));
        }
      }
    else
      {
      struct vec_pak *start, *prev, *current;

      start = g_slist_nth_data(spatial->list, 0);
      prev = start;
      vlist = g_slist_nth(spatial->list, 1);
      while (prev && vlist)
        {
        current = vlist->data;
        ARR3SET(v1, prev->rx);
        ARR3SET(v2, current->rx);
        ARR3ADD(v1, offset);
        ARR3ADD(v2, offset);
        gl_core_append_world_segment(segments, v1, v2);
        prev = current;
        vlist = g_slist_next(vlist);
        }
      if (start && prev && spatial->method == GL_LINE_LOOP && prev != start)
        {
        ARR3SET(v1, prev->rx);
        ARR3SET(v2, start->rx);
        ARR3ADD(v1, offset);
        ARR3ADD(v2, offset);
        gl_core_append_world_segment(segments, v1, v2);
        }
      }

    if (segments->len)
      gl_core_draw_line_vertices(mvp,
                                 (const GLfloat *) segments->data,
                                 segments->len / 3,
                                 colour,
                                 sysenv.render.frame_thickness);
    g_array_free(segments, TRUE);
    }
  while (ilist);
  }
}

static void gl_core_draw_graph(struct canvas_pak *canvas,
                               struct model_pak *model)
{
GLfloat identity[16];
GLfloat axis_colour[4] = {0.0f, 0.0f, 0.0f, 1.0f};
GLfloat select_colour[4] = {0.9f, 0.7f, 0.4f, 1.0f};
gdouble ox, oy, dx, dy;
GSList *list;
struct graph_pak *graph;
GArray *segments;
gint i;

graph = model->graph_active;
if (!graph || !g_slist_find(model->graph_list, graph))
  return;

gl_core_mat4_identity(identity);
gl_core_graph_layout(canvas, graph, &ox, &oy, &dx, &dy);
axis_colour[0] = sysenv.render.fg_colour[0];
axis_colour[1] = sysenv.render.fg_colour[1];
axis_colour[2] = sysenv.render.fg_colour[2];

glDisable(GL_DEPTH_TEST);
glDepthMask(GL_FALSE);
glDisable(GL_CULL_FACE);
glEnable(GL_BLEND);
glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

segments = g_array_new(FALSE, FALSE, sizeof(GLfloat));

for (i=0 ; i<graph->xticks ; i++)
  {
  gdouble tick;
  gdouble x;

  tick = (graph->xticks > 1) ? (gdouble) i / (graph->xticks - 1) : 0.0;
  x = ox + tick*dx;
  if (i > 0)
    gl_core_append_screen_segment(segments, canvas,
                                  ox + ((gdouble) (i-1) / MAX(graph->xticks-1, 1))*dx,
                                  oy,
                                  x,
                                  oy);
  gl_core_append_screen_segment(segments, canvas, x, oy, x, oy+5);
  if (graph->type != GRAPH_REGULAR)
    gl_core_append_screen_segment(segments, canvas, x, oy-dy-1, x, oy-dy-5);
  }

for (i=0 ; i<graph->yticks ; i++)
  {
  gdouble tick;
  gdouble y;

  tick = (graph->yticks > 1) ? (gdouble) i / (graph->yticks - 1) : 0.0;
  y = oy - tick*dy;
  if (i > 0)
    gl_core_append_screen_segment(segments, canvas,
                                  ox,
                                  oy - ((gdouble) (i-1) / MAX(graph->yticks-1, 1))*dy,
                                  ox,
                                  y);
  gl_core_append_screen_segment(segments, canvas, ox, y, ox-5, y);
  if (graph->type != GRAPH_REGULAR)
    gl_core_append_screen_segment(segments, canvas, ox+dx, y, ox+dx+4, y);
  }

if (segments->len)
  gl_core_draw_line_vertices(identity,
                             (const GLfloat *) segments->data,
                             segments->len / 3,
                             axis_colour,
                             1.5);
g_array_free(segments, TRUE);

list = graph->set_list;
if (graph->type == GRAPH_IY_TYPE ||
    graph->type == GRAPH_XY_TYPE ||
    graph->type == GRAPH_YX_TYPE ||
    graph->type == GRAPH_IX_TYPE ||
    graph->type == GRAPH_XX_TYPE)
  {
  g_data_x *gx;
  gint set_index;

  gx = list ? list->data : NULL;
  list = list ? g_slist_next(list) : NULL;

  for (set_index=0 ; list ; list=g_slist_next(list), set_index++)
    {
    g_data_y *gy;
    GArray *data_segments;
    GLfloat data_colour[4];
    gint old_valid;
    gdouble old_x, old_y;

    gy = list->data;
    if (!gy || !gy->y || gy->y_size <= 0)
      continue;

    gl_core_graph_rgba(data_colour, gy->color);
    data_segments = g_array_new(FALSE, FALSE, sizeof(GLfloat));
    old_valid = FALSE;
    old_x = old_y = 0.0;

    for (i=0 ; i<gy->y_size ; )
      {
      graph_type type;
      graph_symbol symbol;
      gdouble xv, yv, sx, sy;
      gboolean in_range;

      type = (gy->type == GRAPH_REGULAR) ? graph->type : gy->type;
      symbol = GRAPH_SYMB_NONE;
      if (gy->symbol)
        symbol = gy->symbol[i];

      switch (type)
        {
        case GRAPH_IY_TYPE:
        case GRAPH_XY_TYPE:
          xv = (gx && i < gx->x_size) ? gx->x[i] : i;
          yv = gy->y[i];
          i++;
          break;
        case GRAPH_YX_TYPE:
          if (i+1 >= gy->y_size)
            {
            i = gy->y_size;
            continue;
            }
          xv = gy->y[i+1];
          yv = gy->y[i];
          i += 2;
          break;
        case GRAPH_IX_TYPE:
        case GRAPH_XX_TYPE:
          xv = (gx && set_index < gx->x_size) ? gx->x[set_index] : set_index;
          yv = gy->y[i];
          i++;
          break;
        case GRAPH_REGULAR:
        default:
          xv = i;
          yv = gy->y[i];
          i++;
          break;
        }

      if (isnan(yv))
        {
        old_valid = FALSE;
        continue;
        }

      in_range = (xv >= graph->xmin && xv <= graph->xmax &&
                  yv >= graph->ymin && yv <= graph->ymax);
      if (!in_range)
        {
        old_valid = FALSE;
        continue;
        }

      sx = gl_core_graph_map_x(graph, ox, dx, xv);
      sy = gl_core_graph_map_y(graph, oy, dy, yv);

      if (symbol != GRAPH_SYMB_NONE)
        gl_core_graph_append_symbol(data_segments, canvas, symbol, sx, sy);

      if (old_valid &&
          gy->line != GRAPH_LINE_NONE &&
          type != GRAPH_IX_TYPE &&
          type != GRAPH_XX_TYPE)
        gl_core_append_screen_segment(data_segments, canvas, old_x, old_y, sx, sy);

      old_valid = TRUE;
      old_x = sx;
      old_y = sy;
      }

    if (data_segments->len)
      gl_core_draw_line_vertices(identity,
                                 (const GLfloat *) data_segments->data,
                                 data_segments->len / 3,
                                 data_colour,
                                 (gy->line == GRAPH_LINE_THICK) ? 2.0 : 1.0);
    g_array_free(data_segments, TRUE);
    }
  }
else
  {
  for ( ; list ; list=g_slist_next(list))
    {
    gdouble *ptr;
    gint old_valid;
    gdouble old_x, old_y;

    ptr = list->data;
    if (!ptr || graph->size <= 0)
      continue;

    segments = g_array_new(FALSE, FALSE, sizeof(GLfloat));
    old_valid = FALSE;
    old_x = old_y = 0.0;

    for (i=0 ; i<graph->size ; i++)
      {
      gdouble xv, yv, sx, sy;

      yv = ptr[i];
      if (isnan(yv))
        {
        old_valid = FALSE;
        continue;
        }

      xv = (graph->size > 1) ? (gdouble) i / (graph->size-1) : 0.0;
      xv *= (graph->xmax - graph->xmin);
      xv += graph->xmin;

      if (xv < graph->xmin || xv > graph->xmax ||
          yv < graph->ymin || yv > graph->ymax)
        {
        old_valid = FALSE;
        continue;
        }

      sx = gl_core_graph_map_x(graph, ox, dx, xv);
      sy = gl_core_graph_map_y(graph, oy, dy, yv);
      if (old_valid)
        gl_core_append_screen_segment(segments, canvas, old_x, old_y, sx, sy);
      old_valid = TRUE;
      old_x = sx;
      old_y = sy;
      }

    if (segments->len)
      gl_core_draw_line_vertices(identity,
                                 (const GLfloat *) segments->data,
                                 segments->len / 3,
                                 axis_colour,
                                 1.0);
    g_array_free(segments, TRUE);
    }
  }

if (graph->select >= 0)
  {
  gdouble sx, sy;
  gboolean have_select;

  have_select = FALSE;
  if (graph->type == GRAPH_IY_TYPE ||
      graph->type == GRAPH_XY_TYPE ||
      graph->type == GRAPH_IX_TYPE ||
      graph->type == GRAPH_XX_TYPE ||
      graph->type == GRAPH_YX_TYPE)
    {
    g_data_x *gx;

    gx = graph->set_list ? graph->set_list->data : NULL;
    if (gx && graph->select < gx->x_size)
      {
      gdouble xv, yv;

      if (graph->type == GRAPH_YX_TYPE)
        {
        xv = graph->select_2;
        yv = gx->x[graph->select];
        }
      else
        {
        xv = gx->x[graph->select];
        yv = graph->select_2;
        }
      sx = gl_core_graph_map_x(graph, ox, dx, xv);
      sy = gl_core_graph_map_y(graph, oy, dy, yv);
      have_select = TRUE;
      }
    }

  if (have_select)
    {
    segments = g_array_new(FALSE, FALSE, sizeof(GLfloat));
    gl_core_append_screen_segment(segments, canvas, ox, sy, sx, sy);
    gl_core_append_screen_segment(segments, canvas, sx, sy, sx, oy-dy);
    gl_core_draw_line_vertices(identity,
                               (const GLfloat *) segments->data,
                               segments->len / 3,
                               select_colour,
                               1.5);
    g_array_free(segments, TRUE);
    }
  }

if (graph->require_xaxis)
  {
  gdouble y;

  y = gl_core_graph_map_y(graph, oy, dy, 0.0);
  segments = g_array_new(FALSE, FALSE, sizeof(GLfloat));
  gl_core_append_screen_segment(segments, canvas, ox, y, ox+dx, y);
  gl_core_draw_line_vertices(identity,
                             (const GLfloat *) segments->data,
                             segments->len / 3,
                             select_colour,
                             1.5);
  g_array_free(segments, TRUE);
  }

if (graph->require_yaxis)
  {
  gdouble x;

  x = gl_core_graph_map_x(graph, ox, dx, 0.0);
  segments = g_array_new(FALSE, FALSE, sizeof(GLfloat));
  gl_core_append_screen_segment(segments, canvas, x, oy-dy, x, oy);
  gl_core_draw_line_vertices(identity,
                             (const GLfloat *) segments->data,
                             segments->len / 3,
                             select_colour,
                             1.5);
  g_array_free(segments, TRUE);
  }

glDepthMask(GL_TRUE);
glDisable(GL_BLEND);
}

static void gl_core_append_cell_edges(GArray *segments,
                                      struct vec_pak *cell,
                                      const gdouble *offset)
{
static const gint loops[2][4] = {{0, 1, 2, 3}, {4, 5, 6, 7}};
gdouble v1[3], v2[3];
gint i, j;

for (i=0 ; i<2 ; i++)
  {
  for (j=0 ; j<4 ; j++)
    {
    ARR3SET(v1, cell[loops[i][j]].rx);
    ARR3SET(v2, cell[loops[i][(j+1)%4]].rx);
    ARR3ADD(v1, offset);
    ARR3ADD(v2, offset);
    gl_core_append_world_segment(segments, v1, v2);
    }
  }

for (i=0 ; i<4 ; i++)
  {
  ARR3SET(v1, cell[i].rx);
  ARR3SET(v2, cell[i+4].rx);
  ARR3ADD(v1, offset);
  ARR3ADD(v2, offset);
  gl_core_append_world_segment(segments, v1, v2);
  }
}

static void gl_core_append_arc_segments(GArray *segments,
                                        const gdouble *p0,
                                        const gdouble *p1,
                                        const gdouble *p2)
{
const gint num_points = 32;
gdouble theta, st, ct;
gdouble v1[3], v2[3], prev[3], current[3];
gdouble len1, len2;
gint i;

ARR3SET(v1, p1);
ARR3SUB(v1, p0);
ARR3SET(v2, p2);
ARR3SUB(v2, p0);

VEC3MUL(v1, 0.5);
VEC3MUL(v2, 0.5);

len1 = VEC3MAG(v1);
len2 = VEC3MAG(v2);
if (len1 < FRACTION_TOLERANCE || len2 < FRACTION_TOLERANCE)
  return;

if (len2 > len1)
  {
  VEC3MUL(v2, len1/len2);
  }
else
  {
  VEC3MUL(v1, len2/len1);
  }

ARR3SET(prev, p0);
ARR3ADD(prev, v1);

for (i=1 ; i<=num_points ; i++)
  {
  theta = (gdouble) i * 0.5 * G_PI / (gdouble) num_points;
  st = tbl_sin(theta);
  ct = tbl_cos(theta);

  ARR3SET(current, p0);
  current[0] += ct * v1[0] + st * v2[0];
  current[1] += ct * v1[1] + st * v2[1];
  current[2] += ct * v1[2] + st * v2[2];
  gl_core_append_world_segment(segments, prev, current);
  ARR3SET(prev, current);
  }
}

static void gl_core_draw_cell_frame(struct model_pak *model,
                                    const GLfloat *mvp)
{
GArray *segments;
GLfloat colour[4];
gdouble offset[3];

if (!model || !model->show_cell || !model->periodic)
  return;

segments = g_array_new(FALSE, FALSE, sizeof(GLfloat));
VEC3SET(offset, 0.0, 0.0, 0.0);
gl_core_append_cell_edges(segments, model->cell, offset);

if (!segments->len)
  {
  g_array_free(segments, TRUE);
  return;
  }

colour[0] = sysenv.render.fg_colour[0];
colour[1] = sysenv.render.fg_colour[1];
colour[2] = sysenv.render.fg_colour[2];
colour[3] = 1.0f;
gl_core_draw_line_vertices(mvp,
                           (const GLfloat *) segments->data,
                           segments->len / 3,
                           colour,
                           sysenv.render.frame_thickness);
g_array_free(segments, TRUE);
}

static void gl_core_draw_cell_images(struct model_pak *model,
                                     const GLfloat *mvp)
{
GArray *segments;
GLfloat colour[4] = {0.8f, 0.7f, 0.6f, 1.0f};
GSList *list;

if (!model || !model->show_cell_images || !model->images)
  return;

segments = g_array_new(FALSE, FALSE, sizeof(GLfloat));
for (list=model->images ; list ; list=g_slist_next(list))
  {
  struct image_pak *image;

  image = list->data;
  if (!image)
    continue;
  gl_core_append_cell_edges(segments, model->cell, image->rx);
  }

if (segments->len)
  gl_core_draw_line_vertices(mvp,
                             (const GLfloat *) segments->data,
                             segments->len / 3,
                             colour,
                             1.0);
g_array_free(segments, TRUE);
}

static void gl_core_draw_links_overlay(struct model_pak *model,
                                       const GLfloat *mvp)
{
GArray *segments;
GLfloat colour[4];
GSList *list;

if (!model || !model->show_links)
  return;

segments = g_array_new(FALSE, FALSE, sizeof(GLfloat));
for (list=model->cores ; list ; list=g_slist_next(list))
  {
  struct core_pak *core;
  struct shel_pak *shell;

  core = list->data;
  if (!core || !core->shell)
    continue;
  if (core->status & (HIDDEN | DELETED))
    continue;

  shell = core->shell;
  gl_core_append_world_segment(segments, core->rx, shell->rx);
  }

if (segments->len)
  {
  colour[0] = sysenv.render.label_colour[0];
  colour[1] = sysenv.render.label_colour[1];
  colour[2] = sysenv.render.label_colour[2];
  colour[3] = 1.0f;
  gl_core_draw_line_vertices(mvp,
                             (const GLfloat *) segments->data,
                             segments->len / 3,
                             colour,
                             1.0);
  }
g_array_free(segments, TRUE);
}

static void gl_core_draw_measurements_overlay(struct model_pak *model,
                                              const GLfloat *mvp)
{
gdouble colour_rgb[3], a1[3], a2[3], a3[3], a4[3], v1[3], v2[3], v3[3], n[3];
gdouble line_width;
GSList *list;

if (!model || !model->measure_list)
  return;

line_width = sysenv.render.geom_line_width;
if (line_width < 1.0)
  line_width = 1.0;

for (list=model->measure_list ; list ; list=g_slist_next(list))
  {
  GArray *segments;
  GLfloat colour[4];
  gint type;

  type = measure_type_get(list->data);
  measure_colour_get(colour_rgb, list->data);
  colour[0] = colour_rgb[0];
  colour[1] = colour_rgb[1];
  colour[2] = colour_rgb[2];
  colour[3] = 1.0f;

  segments = g_array_new(FALSE, FALSE, sizeof(GLfloat));
  switch (type)
    {
    case MEASURE_BOND:
    case MEASURE_DISTANCE:
    case MEASURE_INTER:
    case MEASURE_INTRA:
      measure_coord_get(v1, 0, list->data, model);
      measure_coord_get(v2, 1, list->data, model);
      gl_core_append_world_segment(segments, v1, v2);
      break;

    case MEASURE_ANGLE:
      measure_coord_get(v1, 0, list->data, model);
      measure_coord_get(v2, 1, list->data, model);
      measure_coord_get(v3, 2, list->data, model);
      gl_core_append_arc_segments(segments, v2, v1, v3);
      break;

    case MEASURE_TORSION:
      measure_coord_get(a1, 0, list->data, model);
      measure_coord_get(a2, 1, list->data, model);
      measure_coord_get(a3, 2, list->data, model);
      measure_coord_get(a4, 3, list->data, model);
      ARR3SET(n, a3);
      ARR3SUB(n, a2);
      normalize(n, 3);
      ARR3SET(v3, a1);
      ARR3SUB(v3, a2);
      proj_vop(v1, v3, n);
      normalize(v1, 3);
      ARR3SET(v3, a4);
      ARR3SUB(v3, a3);
      proj_vop(v2, v3, n);
      normalize(v2, 3);
      ARR3SET(v3, a2);
      ARR3ADD(v3, a3);
      VEC3MUL(v3, 0.5);
      ARR3ADD(v1, v3);
      ARR3ADD(v2, v3);
      gl_core_append_arc_segments(segments, v3, v1, v2);
      gl_core_append_world_segment(segments, v1, v3);
      gl_core_append_world_segment(segments, v3, v2);
      break;

    default:
      break;
    }

  if (segments->len)
    gl_core_draw_line_vertices(mvp,
                               (const GLfloat *) segments->data,
                               segments->len / 3,
                               colour,
                               line_width);
  g_array_free(segments, TRUE);
  }
}

static void gl_core_draw_axes_geometry(struct canvas_pak *canvas,
                                       struct model_pak *model,
                                       const GLfloat *mvp)
{
GArray *segments;
GLfloat colour[4];
gdouble origin[3], axis[3], guide[3], scale;
gint ry, i;

if (!canvas || !model || !model->show_axes)
  return;
if (model->rmax <= FRACTION_TOLERANCE)
  return;

ry = sysenv.height - canvas->y - canvas->height + 40;
gl_project(origin, canvas->x+40, ry, canvas);
gl_project(guide, canvas->x+20, ry, canvas);
ARR3SUB(guide, origin);
scale = 20.0 * VEC3MAG(guide) / model->rmax;
if (scale <= FRACTION_TOLERANCE)
  return;

segments = g_array_new(FALSE, FALSE, sizeof(GLfloat));
for (i=0 ; i<3 ; i++)
  {
  ARR3SET(axis, model->axes[i].rx);
  VEC3MUL(axis, scale);
  ARR3ADD(axis, origin);
  gl_core_append_world_segment(segments, origin, axis);
  }

if (!segments->len)
  {
  g_array_free(segments, TRUE);
  return;
  }

colour[0] = sysenv.render.fg_colour[0];
colour[1] = sysenv.render.fg_colour[1];
colour[2] = sysenv.render.fg_colour[2];
colour[3] = 1.0f;

glDisable(GL_DEPTH_TEST);
glDepthMask(GL_FALSE);
gl_core_draw_line_vertices(mvp,
                           (const GLfloat *) segments->data,
                           segments->len / 3,
                           colour,
                           MAX(sysenv.render.frame_thickness, 1.0));
glDepthMask(GL_TRUE);
glEnable(GL_DEPTH_TEST);
g_array_free(segments, TRUE);
}

static void gl_core_draw_selection_box_overlay(struct canvas_pak *canvas,
                                               struct model_pak *model)
{
GArray *segments;
GLfloat colour[4];
GLfloat identity[16];

if (!canvas || !model || !model->box_on)
  return;

segments = g_array_new(FALSE, FALSE, sizeof(GLfloat));
gl_core_append_screen_segment(segments, canvas,
                              model->select_box[0], model->select_box[1],
                              model->select_box[2], model->select_box[1]);
gl_core_append_screen_segment(segments, canvas,
                              model->select_box[2], model->select_box[1],
                              model->select_box[2], model->select_box[3]);
gl_core_append_screen_segment(segments, canvas,
                              model->select_box[2], model->select_box[3],
                              model->select_box[0], model->select_box[3]);
gl_core_append_screen_segment(segments, canvas,
                              model->select_box[0], model->select_box[3],
                              model->select_box[0], model->select_box[1]);

if (!segments->len)
  {
  g_array_free(segments, TRUE);
  return;
  }

colour[0] = sysenv.render.label_colour[0];
colour[1] = sysenv.render.label_colour[1];
colour[2] = sysenv.render.label_colour[2];
colour[3] = 1.0f;
gl_core_mat4_identity(identity);

glDisable(GL_DEPTH_TEST);
glDepthMask(GL_FALSE);
gl_core_draw_line_vertices(identity,
                           (const GLfloat *) segments->data,
                           segments->len / 3,
                           colour,
                           1.5);
glDepthMask(GL_TRUE);
glEnable(GL_DEPTH_TEST);
g_array_free(segments, TRUE);
}

static void gl_core_draw_core_list(GSList *cores,
                                   struct model_pak *model,
                                   const GLfloat *mvp)
{
gint draw_mode;
gint num_indices;
gint num_vertices;
gsize vertex_bytes;
gdouble radius, x[3], colour[4];
GSList *list, *ilist;
struct core_pak *core;
struct image_pak *image;

draw_mode = va_get_gl_method(gl_core_renderer.sphere_mesh);
num_indices = va_get_num_indices(gl_core_renderer.sphere_mesh);
num_vertices = va_get_num_vertices(gl_core_renderer.sphere_mesh);
vertex_bytes = 6 * num_vertices * sizeof(GLfloat);

glUseProgram(gl_core_renderer.program);
glUniformMatrix4fv(gl_core_renderer.u_mvp, 1, GL_FALSE, mvp);
glBindVertexArray(gl_core_renderer.vao);
glBindBuffer(GL_ARRAY_BUFFER, gl_core_renderer.vbo);

for (list=cores ; list ; list=g_slist_next(list))
  {
  core = list->data;

  ARR3SET(colour, core->colour);
  VEC3MUL(colour, INV_COLOUR_SCALE);
  colour[3] = core->colour[3];

  radius = gl_get_radius(core, model);
  if (radius <= 0.0)
    continue;

  glUniform4f(gl_core_renderer.u_colour,
              colour[0], colour[1], colour[2], colour[3]);

  ilist = NULL;
  do
    {
    ARR3SET(x, core->rx);
    if (ilist)
      {
      image = ilist->data;
      ARR3ADD(x, image->rx);
      ilist = g_slist_next(ilist);
      }
    else
      ilist = model->images;

    va_prepare_sphere(gl_core_renderer.sphere_mesh, x, radius);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertex_bytes,
                    va_get_interleaved(gl_core_renderer.sphere_mesh));
    glDrawElements(draw_mode, num_indices, GL_UNSIGNED_INT, 0);
    }
  while (ilist);
  }

glBindVertexArray(0);
glUseProgram(0);
}

static void gl_core_draw_cylinder_pipe_list(GSList *pipe_list,
                                            struct model_pak *model,
                                            const GLfloat *mvp)
{
gint draw_mode;
gint num_indices;
gint num_vertices;
gsize vertex_bytes;
gdouble v1[3], v2[3];
gdouble axis[3], len, trim;
GSList *list, *ilist;
struct pipe_pak *pipe;
struct image_pak *image;

if (!pipe_list)
  return;

draw_mode = va_get_gl_method(gl_core_renderer.cylinder_mesh);
num_indices = va_get_num_indices(gl_core_renderer.cylinder_mesh);
num_vertices = va_get_num_vertices(gl_core_renderer.cylinder_mesh);
vertex_bytes = 6 * num_vertices * sizeof(GLfloat);

glUseProgram(gl_core_renderer.program);
glUniformMatrix4fv(gl_core_renderer.u_mvp, 1, GL_FALSE, mvp);
glBindVertexArray(gl_core_renderer.cylinder_vao);
glBindBuffer(GL_ARRAY_BUFFER, gl_core_renderer.cylinder_vbo);

for (list=pipe_list ; list ; list=g_slist_next(list))
  {
  pipe = list->data;

  ilist = NULL;
  do
    {
    ARR3SET(v1, pipe->v1);
    ARR3SET(v2, pipe->v2);
    if (ilist)
      {
      image = ilist->data;
      ARR3ADD(v1, image->rx);
      ARR3ADD(v2, image->rx);
      ilist = g_slist_next(ilist);
      }
    else
      ilist = model->images;

    ARR3SET(axis, v2);
    ARR3SUB(axis, v1);
    len = VEC3MAG(axis);
    if (len > FRACTION_TOLERANCE)
      {
      trim = gl_get_radius(pipe->core, model);
      if (trim > 0.0 && trim < len)
        {
        normalize(axis, 3);
        v1[0] += trim * axis[0];
        v1[1] += trim * axis[1];
        v1[2] += trim * axis[2];
        }
      }

    glUniform4f(gl_core_renderer.u_colour,
                pipe->colour[0],
                pipe->colour[1],
                pipe->colour[2],
                pipe->colour[3] > 0.0 ? pipe->colour[3] : 1.0);
    va_prepare_cylinder(gl_core_renderer.cylinder_mesh, v1, v2, pipe->radius);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertex_bytes,
                    va_get_interleaved(gl_core_renderer.cylinder_mesh));
    glDrawElements(draw_mode, num_indices, GL_UNSIGNED_INT, 0);
    }
  while (ilist);
  }

glBindVertexArray(0);
glUseProgram(0);
}

void gl_core_draw_line_pipe_list(GSList *pipe_list,
                                 struct model_pak *model,
                                 const GLfloat *mvp)
{
GLfloat vertices[6];
gdouble v1[3], v2[3];
GSList *list, *ilist;
struct pipe_pak *pipe;
struct image_pak *image;

if (!pipe_list)
  return;

glUseProgram(gl_core_renderer.line_program);
glUniformMatrix4fv(gl_core_renderer.u_line_mvp, 1, GL_FALSE, mvp);
glBindVertexArray(gl_core_renderer.line_vao);
glBindBuffer(GL_ARRAY_BUFFER, gl_core_renderer.line_vbo);

for (list=pipe_list ; list ; list=g_slist_next(list))
  {
  pipe = list->data;

  ilist = NULL;
  do
    {
    ARR3SET(v1, pipe->v1);
    ARR3SET(v2, pipe->v2);
    if (ilist)
      {
      image = ilist->data;
      ARR3ADD(v1, image->rx);
      ARR3ADD(v2, image->rx);
      ilist = g_slist_next(ilist);
      }
    else
      ilist = model->images;

    vertices[0] = v1[0];
    vertices[1] = v1[1];
    vertices[2] = v1[2];
    vertices[3] = v2[0];
    vertices[4] = v2[1];
    vertices[5] = v2[2];

    glUniform4f(gl_core_renderer.u_line_colour,
                pipe->colour[0],
                pipe->colour[1],
                pipe->colour[2],
                pipe->colour[3] > 0.0 ? pipe->colour[3] : 1.0);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glDrawArrays(GL_LINES, 0, 2);
    }
  while (ilist);
  }

glBindVertexArray(0);
glUseProgram(0);
}

static void gl_core_draw_selection_overlay(struct model_pak *model,
                                           const GLfloat *mvp)
{
gint draw_mode;
gint num_indices;
gint num_vertices;
gsize vertex_bytes;
gdouble radius, x[3], line_width;
GSList *list, *ilist;
struct core_pak *core;
struct image_pak *image;

if (!model || !model->selection)
  return;

draw_mode = va_get_gl_method(gl_core_renderer.sphere_mesh);
num_indices = va_get_num_indices(gl_core_renderer.sphere_mesh);
num_vertices = va_get_num_vertices(gl_core_renderer.sphere_mesh);
vertex_bytes = 6 * num_vertices * sizeof(GLfloat);

line_width = sysenv.render.geom_line_width;
if (line_width < 1.5)
  line_width = 1.5;

glUseProgram(gl_core_renderer.program);
glUniformMatrix4fv(gl_core_renderer.u_mvp, 1, GL_FALSE, mvp);
glBindVertexArray(gl_core_renderer.vao);
glBindBuffer(GL_ARRAY_BUFFER, gl_core_renderer.vbo);
glUniform4f(gl_core_renderer.u_colour, 1.0f, 0.95f, 0.25f, 1.0f);
glEnable(GL_BLEND);
glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
glDepthMask(GL_FALSE);
glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
glLineWidth(line_width);

for (list=model->selection ; list ; list=g_slist_next(list))
  {
  core = list->data;

  if (!core)
    continue;
  if (core->status & (HIDDEN | DELETED))
    continue;
  if (core->render_mode == ZONE)
    continue;

  radius = fabs(gl_get_radius(core, model));
  if (radius < sysenv.render.ball_radius)
    radius = sysenv.render.ball_radius;
  radius *= 1.15;

  ilist = NULL;
  do
    {
    ARR3SET(x, core->rx);
    if (ilist)
      {
      image = ilist->data;
      ARR3ADD(x, image->rx);
      ilist = g_slist_next(ilist);
      }
    else
      ilist = model->images;

    va_prepare_sphere(gl_core_renderer.sphere_mesh, x, radius);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertex_bytes,
                    va_get_interleaved(gl_core_renderer.sphere_mesh));
    glDrawElements(draw_mode, num_indices, GL_UNSIGNED_INT, 0);
    }
  while (ilist);
  }

glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
glDepthMask(GL_TRUE);
glDisable(GL_BLEND);
glBindVertexArray(0);
glUseProgram(0);
}

static void gl_core_draw_model(struct canvas_pak *canvas,
                               struct model_pak *model)
{
GSList *solid, *ghost, *wire;
GSList *pipes[4];
GLfloat modelview[16], projection[16], mvp[16];
static gboolean debug_reported = FALSE;
static gpointer last_model = NULL;
static gpointer last_graph = NULL;
static gboolean first_graph_state = TRUE;
gint i;

solid = NULL;
ghost = NULL;
wire = NULL;
for (i=0 ; i<4 ; i++)
  pipes[i] = NULL;

gl_core_prepare_matrices(canvas, model, modelview, projection, mvp);

if (!gl_core_prepare(model))
  return;

if (g_getenv("GDIS_DEBUG_GRAPH_STATE") &&
    (first_graph_state ||
     model != last_model ||
     model->graph_active != last_graph))
  {
  g_printerr("gl_core_draw_model: model=%p active_model=%p graph_active=%p graphs=%d picture=%p\n",
             model, sysenv.active_model, model->graph_active,
             g_slist_length(model->graph_list), model->picture_active);
  last_model = model;
  last_graph = model->graph_active;
  first_graph_state = FALSE;
  }

if (model->graph_active)
  {
  gl_core_draw_graph(canvas, model);
  return;
  }

if (!gl_core_renderer.warned_limited &&
    !g_getenv("GDIS_SUPPRESS_LIMITED_MODE_NOTICE"))
  {
  g_printerr("GDIS core-profile renderer is active in limited mode: atom "
             "spheres, bond/stick geometry, cell frames, graph plots/text, "
             "spatial surfaces, colour scales, and several overlays render, "
             "while some legacy overlays and interaction paths still need "
             "porting.\n");
  gl_core_renderer.warned_limited = TRUE;
  }

glEnable(GL_DEPTH_TEST);
glDepthFunc(GL_LESS);
glDepthMask(GL_TRUE);
glDisable(GL_CULL_FACE);
glDisable(GL_STENCIL_TEST);
glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
glDisable(GL_BLEND);

if (model->show_cores)
  gl_build_core_lists(&solid, &ghost, &wire, model);
render_make_pipes(pipes, model);

if (gl_core_debug_enabled() && !debug_reported)
  {
  struct camera_pak *camera;
  struct core_pak *core;
  gdouble radius;

  camera = model->camera;
  g_printerr("GDIS core debug: canvas=(%d,%d %dx%d) show_cores=%d graph=%d "
             "picture=%d\n",
             canvas->x, canvas->y, canvas->width, canvas->height,
             model->show_cores, model->graph_active != NULL,
             model->picture_active != NULL);
  g_printerr("GDIS core debug: solids=%d ghosts=%d wires=%d sphere_quality=%d "
             "vertices=%d indices=%d\n",
             g_slist_length(solid), g_slist_length(ghost), g_slist_length(wire),
             nearest_int(sysenv.render.sphere_quality),
             gl_core_renderer.sphere_mesh ?
               va_get_num_vertices(gl_core_renderer.sphere_mesh) : 0,
             gl_core_renderer.sphere_mesh ?
               va_get_num_indices(gl_core_renderer.sphere_mesh) : 0);

  if (camera)
    {
    g_printerr("GDIS core debug: camera x=(%.3f %.3f %.3f) "
               "v=(%.3f %.3f %.3f) o=(%.3f %.3f %.3f) zoom=%.3f "
               "perspective=%d\n",
               camera->x[0], camera->x[1], camera->x[2],
               camera->v[0], camera->v[1], camera->v[2],
               camera->o[0], camera->o[1], camera->o[2],
               camera->zoom, camera->perspective);
    }

  core = solid ? solid->data : NULL;
  if (!core && wire)
    core = wire->data;
  if (!core && ghost)
    core = ghost->data;

  if (core)
    {
    GLfloat clip[4];

    radius = gl_get_radius(core, model);
    gl_core_transform_point(mvp, core->rx, clip);
    g_printerr("GDIS core debug: first core=%s pos=(%.3f %.3f %.3f) "
               "radius=%.3f clip=(%.3f %.3f %.3f %.3f)",
               core->atom_label,
               core->rx[0], core->rx[1], core->rx[2],
               radius,
               clip[0], clip[1], clip[2], clip[3]);
    if (fabs(clip[3]) > FRACTION_TOLERANCE)
      {
      g_printerr(" ndc=(%.3f %.3f %.3f)",
                 clip[0]/clip[3], clip[1]/clip[3], clip[2]/clip[3]);
      }
    g_printerr("\n");
    }

  debug_reported = TRUE;
  }

glDisable(GL_BLEND);
gl_core_draw_core_list(solid, model, mvp);
gl_core_log_gl_error("solid draw");
gl_core_draw_cylinder_pipe_list(pipes[0], model, mvp);
gl_core_log_gl_error("solid cylinder draw");
gl_core_draw_cylinder_pipe_list(pipes[3], model, mvp);
gl_core_log_gl_error("stick cylinder draw");
gl_core_draw_spatial_surfaces(model, SPATIAL_SOLID, mvp);
gl_core_log_gl_error("solid spatial draw");

glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
glLineWidth(sysenv.render.line_thickness);
gl_core_draw_core_list(wire, model, mvp);
gl_core_log_gl_error("wire draw");
gl_core_draw_cylinder_pipe_list(pipes[2], model, mvp);
gl_core_log_gl_error("wire cylinder draw");

glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
gl_core_draw_spatial_surfaces(model, SPATIAL_SURFACE, mvp);
gl_core_log_gl_error("surface spatial draw");
if (ghost)
  {
  ghost = g_slist_sort(ghost, (gpointer) gl_depth_sort);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDepthMask(GL_FALSE);
  gl_core_draw_core_list(ghost, model, mvp);
  gl_core_log_gl_error("ghost draw");
  pipes[1] = render_sort_pipes(pipes[1]);
  gl_core_draw_cylinder_pipe_list(pipes[1], model, mvp);
  gl_core_log_gl_error("ghost cylinder draw");
  glDepthMask(GL_TRUE);
  glDisable(GL_BLEND);
  }

gl_core_draw_spatial_lines(model, mvp);
gl_core_log_gl_error("line spatial draw");
gl_core_draw_cell_frame(model, mvp);
gl_core_log_gl_error("cell draw");
gl_core_draw_cell_images(model, mvp);
gl_core_log_gl_error("cell image draw");
gl_core_draw_measurements_overlay(model, mvp);
gl_core_log_gl_error("measurement draw");
gl_core_draw_links_overlay(model, mvp);
gl_core_log_gl_error("link draw");
gl_core_draw_selection_overlay(model, mvp);
gl_core_draw_axes_geometry(canvas, model, mvp);
gl_core_log_gl_error("axes draw");
gl_core_draw_selection_box_overlay(canvas, model);
gl_core_log_gl_error("selection box draw");

for (i=0 ; i<4 ; i++)
  g_slist_free_full(pipes[i], g_free);
g_slist_free(solid);
g_slist_free(ghost);
g_slist_free(wire);
}

static gboolean gl_core_refresh(void)
{
GSList *list;
struct model_pak *model;
struct canvas_pak *canvas;
gboolean do_snap;
struct canvas_pak *snap_canvas;
guchar *surface_data;
gint w, h;

do_snap = FALSE;
snap_canvas = NULL;
surface_data = NULL;
gdis_gl_get_size(sysenv.glarea, &w, &h);

glClearColor(sysenv.render.bg_colour[0], sysenv.render.bg_colour[1],
             sysenv.render.bg_colour[2], 1.0);
glClearStencil(0x0);
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
make_fg_visible();

if (sysenv.cairo_surface)
  {
  cairo_surface_destroy(sysenv.cairo_surface);
  sysenv.cairo_surface = NULL;
  }
if (w > 0 && h > 0)
  {
  surface_data = g_malloc0(4*w*h*sizeof(guchar));
  sysenv.cairo_surface = cairo_image_surface_create_for_data(surface_data,
                                                             CAIRO_FORMAT_ARGB32,
                                                             w, h, 4*w);
  }

for (list=sysenv.canvas_list ; list ; list=g_slist_next(list))
  {
  canvas = list->data;
  model = canvas->model;

  if (!model)
    continue;

  gl_core_draw_model(canvas, model);
  if (sysenv.cairo_surface)
    {
    if (model->graph_active)
      gl_core_draw_graph_text(canvas, model->graph_active);
    else
      gl_core_draw_model_overlay(canvas, model);
    }
  model->redraw = FALSE;

  if (model->snapshot_eps)
    {
    snap_canvas = canvas;
    do_snap = TRUE;
    }

  if (canvas_timing_adjust(model))
    {
    if (sysenv.cairo_surface)
      {
      cairo_surface_destroy(sysenv.cairo_surface);
      sysenv.cairo_surface = NULL;
      }
    g_free(surface_data);
    return(FALSE);
    }
  }

if (sysenv.cairo_surface)
  {
  cairo_surface_flush(sysenv.cairo_surface);
  if (g_getenv("GDIS_DEBUG_OVERLAY_DUMP"))
    cairo_surface_write_to_png(sysenv.cairo_surface,
                               g_getenv("GDIS_DEBUG_OVERLAY_DUMP"));
  gl_core_draw_overlay_texture(surface_data, w, h);
  cairo_surface_destroy(sysenv.cairo_surface);
  sysenv.cairo_surface = NULL;
  }
g_free(surface_data);

if (do_snap)
  do_eps_snapshot(snap_canvas->model, snap_canvas->width, snap_canvas->height);

return(TRUE);
}
#else
static gboolean gl_core_refresh(void)
{
return(FALSE);
}
#endif

/**************************/
/* handle redraw requests */
/**************************/
#define DEBUG_CANVAS_REFRESH 0
#define GDK_GL_DEBUG 0
gint gl_canvas_refresh(void)
{
gint nc;
GSList *list;
struct model_pak *model;
struct canvas_pak *canvas;
gboolean do_snap=FALSE;
struct canvas_pak *snap_canvas;
/*CAIRO write --OHPA*/
unsigned char* surface_data = NULL;
gint w,h;

/* divert to stereo update? */
if (sysenv.stereo)
  {
/* FIXME - this hack means windowed stereo will only display the 1st canvas */
stereo_init_window((sysenv.canvas_list)->data);

  stereo_draw();
  return(TRUE);
  }

/* is there anything to draw on? */
if (!gdis_gl_begin(sysenv.glarea))
  return(FALSE);

if (!gdis_gl_context_is_legacy(sysenv.glarea))
  {
  gboolean ok;

  ok = gl_core_refresh();
  gdis_gl_swap_buffers(sysenv.glarea);
  gdis_gl_end(sysenv.glarea);
  return(ok);
  }

glClearColor(sysenv.render.bg_colour[0], sysenv.render.bg_colour[1], sysenv.render.bg_colour[2], 0.0);
glClearStencil(0x0);
#if GTK_MAJOR_VERSION < 3
glDrawBuffer(GL_BACK);
#endif
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
make_fg_visible();

/* pango fonts for OpenGL are now taken 'on the fly' --OVHPA */
/*cairo_surface init - we now use an explicit buffer! --OVHPA*/
if(sysenv.cairo_surface!=NULL) {
	/*we have another refresh before the first one ends?*/
	cairo_surface_destroy(sysenv.cairo_surface);
	sysenv.cairo_surface=NULL;
	/*we are going to overwrite it anyway.*/
}
gdis_gl_get_size(sysenv.glarea, &w, &h);
surface_data=g_malloc0(4*w*h*sizeof(unsigned char));
sysenv.cairo_surface=cairo_image_surface_create_for_data(surface_data,CAIRO_FORMAT_ARGB32,w,h,4*w);

nc = g_slist_length(sysenv.canvas_list);

for (list=sysenv.canvas_list ; list ; list=g_slist_next(list))
  {
  canvas = list->data;
  model = canvas->model;

#if DEBUG_CANVAS
gchar *str = g_strdup_printf("FPS: %i", sysenv.fps);
pango_print(str, 5, 5, canvas, 10, 0);
g_free(str);
printf("canvas: %p\nactive: %d\nresize: %d\nmodel: %p\n",
        canvas, canvas->active, canvas->resize, canvas->model);
#endif

/* set up viewing transformations (even if no model - border) */
  gl_init_projection(canvas, model);

/* drawing (model only) */
  if (model)
    {
/* clear the atom info box  */
/* CURRENT - always have 2 redraw each canvas as glClear() blanks the entire window */
/* TODO - can we clear/redraw only when redraw requested? */
/*
    if (model->redraw)
*/
      {

#if DEBUG_CANVAS_REFRESH
printf("gl_draw(): %d,%d - %d x %d\n", canvas->x, canvas->y, canvas->width, canvas->height);
#endif

      if (model->graph_active)
        graph_draw(canvas, model);
      else
        draw_objs(canvas, model);

      model->redraw = FALSE;

      if(model->snapshot_eps) {
		snap_canvas=canvas;
		do_snap=TRUE;/*<- do actual snapshot AFTER drawing*/
      }

/*draw the text buffer*/
	cairo_surface_flush(sysenv.cairo_surface);
	/* This would normally be unnecessary, but setting the
	 * raster position to the screen edge will FAIL due to
	 * projection rounding making the raster off-screen...
	 *                                             --OVHPA */
	glRasterPos2i(canvas->x,canvas->y);
	/* And, because position of glBitmap is never invalid! */
	glBitmap(0, 0, 0, 0, -0.5*canvas->width,0.5*canvas->height, NULL);
	/*usual bliting*/
	glPixelZoom( 1.0, -1.0 );
	glDrawPixels(w,h,GL_BGRA,GL_UNSIGNED_BYTE,surface_data);
/* --OVHPA*/
      if (canvas_timing_adjust(model))
        {
		gdis_gl_swap_buffers(sysenv.glarea);
	        gdis_gl_end(sysenv.glarea);
		/*cleanup text buffer*/
		cairo_surface_destroy(sysenv.cairo_surface);
		g_free(surface_data);
	sysenv.cairo_surface=NULL;
        return(FALSE);
        }
      }
    }

/* draw viewport frames if more than one canvas */
  if (nc > 1)
    {
    glColor3f(1.0, 1.0, 1.0);
    if (sysenv.active_model)
      {
      if (canvas->model == sysenv.active_model)
        glColor3f(1.0, 1.0, 0.0);
      }
    glLineWidth(1.0);
    gl_draw_box(canvas->x+1, sysenv.height-canvas->y-1,
                canvas->x+canvas->width-1, sysenv.height-canvas->y-canvas->height, canvas);
    }
  }
gdis_gl_swap_buffers(sysenv.glarea);
gdis_gl_end(sysenv.glarea);
/*cleanup text buffer*/
cairo_surface_destroy(sysenv.cairo_surface);
g_free(surface_data);
sysenv.cairo_surface=NULL;

if(do_snap) do_eps_snapshot(snap_canvas->model,snap_canvas->width,snap_canvas->height);/*snapshot here*/

return(TRUE);
}

/**************************/
/* handle redraw requests */
/**************************/
gboolean gui_canvas_handler(gpointer dummy)
{
static gulong start=0, time=0, frames=0;

(void) dummy;

/* first time init */
if (!start)
  start = mytimer();

frames++;

/* update FPS every nth frame */
if (frames > 10)
  {
  gdouble fps = frames * 1000000.0;

  time = mytimer() - start;

  sysenv.fps = nearest_int(fps / (gdouble) time);
  start = mytimer();
  time = frames = 0;
  }

if (sysenv.refresh_canvas)
  {
#if GTK_MAJOR_VERSION >= 3
  if (GTK_IS_GL_AREA(sysenv.glarea))
    gtk_gl_area_queue_render(GTK_GL_AREA(sysenv.glarea));
  else
    gl_canvas_refresh();
#else
  gl_canvas_refresh();
#endif
  sysenv.refresh_canvas = FALSE;
  }

if (sysenv.refresh_dialog)
  {
/* dialog redraw */
  dialog_refresh_all();

/* model pane redraw */
  tree_model_refresh(sysenv.active_model);

/* selection redraw */
  gui_active_refresh();

  sysenv.refresh_dialog = FALSE;
  }

if (sysenv.snapshot)
  image_write(sysenv.glarea);

return(TRUE);
}
