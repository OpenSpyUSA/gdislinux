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
#define G_DISABLE_DEPRECATED
#define GDK_DISABLE_DEPRECATED
#define GDK_PIXBUF_DISABLE_DEPRECATED
//#define GTK_DISABLE_DEPRECATED
#define GLIB_DISABLE_DEPRECATION_WARNINGS
#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include "gdis.h"
#include "gui_gl.h"
#include "opengl.h"
#include "interface.h"

extern struct sysenv_pak sysenv;

/***************************/
/* free a canvas structure */
/***************************/
void canvas_free(gpointer data)
{
struct canvas_pak *canvas = data;
g_free(canvas);
}

/****************************/
/* schedule redraw requests */ 
/****************************/
void redraw_canvas(gint action)
{
GSList *list;
struct model_pak *model;

switch (action)
  {
  case SINGLE:
/*
    data = sysenv.active_model;
    if (data)
      data->redraw = TRUE;
    break;
*/
  case ALL:
    for (list=sysenv.mal ; list ; list=g_slist_next(list))
      {
      model = list->data;
      model->redraw = TRUE;
      }
    break;
  }
sysenv.refresh_canvas = TRUE;
}

static void canvas_configure_size(GtkWidget *w, gint width, gint height)
{
gint size;
GSList *list;
struct model_pak *model;

g_assert(w != NULL);

if (width > height)
  size = height;
else
  size = width;

sysenv.x = 0;
sysenv.y = 0;
sysenv.width = width;
sysenv.height = height;
sysenv.size = size;

canvas_resize();

for (list=sysenv.mal ; list ; list=g_slist_next(list))
  {
  model = list->data;
  model->redraw = TRUE;
  }

sysenv.write_gdisrc = TRUE;
}

/*******************/
/* configure event */
/*******************/
#define DEBUG_GL_CONFIG_EVENT 0
gint canvas_configure(GtkWidget *w, GdkEventConfigure *event, gpointer data)
{
(void) event;
(void) data;

canvas_configure_size(w,
                      gdis_gtk_widget_get_width(w),
                      gdis_gtk_widget_get_height(w));

#if DEBUG_GL_CONFIG_EVENT
printf("Relative canvas origin: (%d,%d)\n",sysenv.x,sysenv.y);
printf("     Canvas dimensions:  %dx%d\n",sysenv.width,sysenv.height);
#endif

return(TRUE);
}

/*****************/
/* expose event */
/****************/
#define DEBUG_GL_EXPOSE 0
gint canvas_expose(GtkWidget *w, GdkEventExpose *event, gpointer data)
{
/*
gl_clear_canvas();
for (list=sysenv.mal ; list ; list=g_slist_next(list))
  {
  model = list->data;
  model->redraw = TRUE;
  canvas = model->canvas;
  if (canvas->active)
    gl_draw(canvas, model);
  }
*/

redraw_canvas(ALL);

return(TRUE);
}

#if GTK_MAJOR_VERSION >= 3
static void canvas_glarea_resize(GtkGLArea *area,
                                 gint width,
                                 gint height,
                                 gpointer data)
{
(void) width;
(void) height;
(void) data;

canvas_configure_size(GTK_WIDGET(area),
                      gdis_gtk_widget_get_width(GTK_WIDGET(area)),
                      gdis_gtk_widget_get_height(GTK_WIDGET(area)));
}

static gboolean canvas_glarea_render(GtkGLArea *area,
                                     GdkGLContext *context,
                                     gpointer data)
{
(void) area;
(void) context;
(void) data;

#if GTK_MAJOR_VERSION >= 4
gtk_gl_area_attach_buffers(area);
#endif

if (g_getenv("GDIS_DEBUG_GL"))
  {
  static gboolean reported = FALSE;

  if (!reported)
    {
    g_printerr("GtkGLArea render callback reached.\n");
    reported = TRUE;
    }
  }

gl_canvas_refresh();

return(TRUE);
}
#endif

#if GTK_MAJOR_VERSION >= 4
static GdkModifierType canvas_gtk4_button_state = 0;

static gboolean canvas_debug_input_enabled(void)
{
return(g_getenv("GDIS_DEBUG_INPUT") != NULL);
}

static GdkModifierType canvas_button_mask(guint button)
{
switch (button)
  {
  case 1:
    return(GDK_BUTTON1_MASK);
  case 2:
    return(GDK_BUTTON2_MASK);
  case 3:
    return(GDK_BUTTON3_MASK);
  case 4:
    return(GDK_BUTTON4_MASK);
  case 5:
    return(GDK_BUTTON5_MASK);
  default:
    return(0);
  }
}

static void canvas_click_pressed(GtkGestureClick *gesture,
                                 gint n_press,
                                 gdouble x,
                                 gdouble y,
                                 gpointer data)
{
GtkWidget *widget;
GdkModifierType state;
guint button;

(void) n_press;
(void) data;

widget = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(gesture));
if (!widget)
  return;

state = gtk_event_controller_get_current_event_state(GTK_EVENT_CONTROLLER(gesture));
button = gtk_gesture_single_get_current_button(GTK_GESTURE_SINGLE(gesture));
state |= canvas_button_mask(button);
canvas_gtk4_button_state |= canvas_button_mask(button);

if (canvas_debug_input_enabled())
  {
  g_printerr("GDIS input: GTK4 click press button=%u at %.1f,%.1f state=0x%x\n",
             button, x, y, state);
  }

gui_press_input(widget, button, x, y, state);
}

static void canvas_click_released(GtkGestureClick *gesture,
                                  gint n_press,
                                  gdouble x,
                                  gdouble y,
                                  gpointer data)
{
GtkWidget *widget;
GdkModifierType state;
guint button;

(void) n_press;
(void) data;

widget = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(gesture));
if (!widget)
  return;

state = gtk_event_controller_get_current_event_state(GTK_EVENT_CONTROLLER(gesture));
button = gtk_gesture_single_get_current_button(GTK_GESTURE_SINGLE(gesture));
state |= canvas_gtk4_button_state;
canvas_gtk4_button_state &= ~canvas_button_mask(button);

if (canvas_debug_input_enabled())
  {
  g_printerr("GDIS input: GTK4 click release button=%u at %.1f,%.1f state=0x%x\n",
             button, x, y, state);
  }

gui_release_input(widget, button, x, y, state);
}

static void canvas_motion_changed(GtkEventControllerMotion *controller,
                                  gdouble x,
                                  gdouble y,
                                  gpointer data)
{
GtkWidget *widget;
GdkModifierType state;

(void) data;

widget = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(controller));
if (!widget)
  return;

state = gtk_event_controller_get_current_event_state(GTK_EVENT_CONTROLLER(controller));
state |= canvas_gtk4_button_state;

if (canvas_debug_input_enabled() && canvas_gtk4_button_state)
  {
  g_printerr("GDIS input: GTK4 motion at %.1f,%.1f state=0x%x tracked=0x%x\n",
             x, y, state, canvas_gtk4_button_state);
  }

gui_motion_input(widget, x, y, state);
}

static gboolean canvas_scroll_changed(GtkEventControllerScroll *controller,
                                      gdouble dx,
                                      gdouble dy,
                                      gpointer data)
{
GtkWidget *widget;
gdouble delta;

(void) data;

widget = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(controller));
if (!widget)
  return(FALSE);

delta = (dy != 0.0) ? dy : dx;

if (canvas_debug_input_enabled())
  {
  g_printerr("GDIS input: GTK4 scroll dx=%.3f dy=%.3f\n", dx, dy);
  }

return(gui_scroll_delta(widget, delta));
}
#endif

/*****************************************************/
/* create a new canvas and place in the canvas table */
/*****************************************************/
void canvas_new(gint x, gint y, gint w, gint h)
{
struct canvas_pak *canvas;

/* create an OpenGL capable drawing area */
canvas = g_malloc(sizeof(struct canvas_pak));
/*
printf("creating canvas: %p (%d,%d) [%d x %d] \n", canvas, x, y, w, h);
*/
canvas->x = x;
canvas->y = y;
canvas->width = w;
canvas->height = h;
if (w > h)
  canvas->size = h;
else
  canvas->size = w;
canvas->active = FALSE;
canvas->resize = TRUE;

/*
canvas->model = sysenv.active_model;
*/
canvas->model = NULL;

sysenv.canvas_list = g_slist_prepend(sysenv.canvas_list, canvas);
}

	/**************************************/
	/* initialize the OpenGL drawing area */
	/**************************************/
void canvas_init(GtkWidget *box)
{
#if GTK_MAJOR_VERSION >= 4
GtkEventController *motion;
GtkEventController *scroll;
GtkGesture *click;
#endif

/* create the drawing area */
sysenv.glarea = gdis_gl_widget_new(sysenv.glconfig);
gtk_widget_set_size_request(sysenv.glarea, sysenv.width, sysenv.height);
gtk_box_pack_start(GTK_BOX(box), sysenv.glarea, TRUE, TRUE, 0);

/* init signals */
#if GTK_MAJOR_VERSION >= 3
g_signal_connect(GTK_OBJECT(sysenv.glarea), "render",
                 G_CALLBACK(canvas_glarea_render), NULL);
g_signal_connect(GTK_OBJECT(sysenv.glarea), "resize",
                 G_CALLBACK(canvas_glarea_resize), NULL);
#else
g_signal_connect(GTK_OBJECT(sysenv.glarea), "expose_event",
                 GTK_SIGNAL_FUNC(canvas_expose), NULL);
g_signal_connect(GTK_OBJECT(sysenv.glarea), "configure_event",
                 GTK_SIGNAL_FUNC(canvas_configure), NULL);
#endif

/* TODO - what about the "realize" event??? */

#if GTK_MAJOR_VERSION >= 4
click = gtk_gesture_click_new();
gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(click), 0);
gtk_event_controller_set_propagation_phase(GTK_EVENT_CONTROLLER(click),
                                           GTK_PHASE_CAPTURE);
g_signal_connect(click, "pressed",
                 G_CALLBACK(canvas_click_pressed), NULL);
g_signal_connect(click, "released",
                 G_CALLBACK(canvas_click_released), NULL);
gtk_widget_add_controller(sysenv.glarea, GTK_EVENT_CONTROLLER(click));

motion = gtk_event_controller_motion_new();
gtk_event_controller_set_propagation_phase(motion, GTK_PHASE_CAPTURE);
g_signal_connect(motion, "motion",
                 G_CALLBACK(canvas_motion_changed), NULL);
gtk_widget_add_controller(sysenv.glarea, motion);

scroll = gtk_event_controller_scroll_new(GTK_EVENT_CONTROLLER_SCROLL_BOTH_AXES
                                         | GTK_EVENT_CONTROLLER_SCROLL_DISCRETE);
gtk_event_controller_set_propagation_phase(scroll, GTK_PHASE_CAPTURE);
g_signal_connect(scroll, "scroll",
                 G_CALLBACK(canvas_scroll_changed), NULL);
gtk_widget_add_controller(sysenv.glarea, scroll);

gtk_widget_set_focusable(sysenv.glarea, TRUE);
#else
g_signal_connect(GTK_OBJECT(sysenv.glarea), "motion_notify_event",
                 GTK_SIGNAL_FUNC(gui_motion_event), NULL);
g_signal_connect(GTK_OBJECT(sysenv.glarea), "button_press_event",
                 GTK_SIGNAL_FUNC(gui_press_event), NULL);
g_signal_connect(GTK_OBJECT(sysenv.glarea), "button_release_event",
                 GTK_SIGNAL_FUNC(gui_release_event), NULL);
g_signal_connect(GTK_OBJECT(sysenv.glarea), "scroll_event",
                 GTK_SIGNAL_FUNC(gui_scroll_event), NULL);

gtk_widget_set_events(GTK_WIDGET(sysenv.glarea), GDK_EXPOSURE_MASK
                                               | GDK_LEAVE_NOTIFY_MASK
                                               | GDK_BUTTON_PRESS_MASK
                                               | GDK_BUTTON_RELEASE_MASK
                                               | GDK_SCROLL_MASK
                                               | GDK_POINTER_MOTION_MASK
                                               | GDK_POINTER_MOTION_HINT_MASK);
#endif

gtk_widget_show(sysenv.glarea);
#if GTK_MAJOR_VERSION >= 4
gtk_widget_grab_focus(sysenv.glarea);
#endif
}

/**************************/
/* table resize primitive */
/**************************/
#define DEBUG_CANVAS_RESIZE 0
void canvas_resize(void)
{
gint i, j, n, rows, cols, width, height;
GSList *list;
struct canvas_pak *canvas;

n = g_slist_length(sysenv.canvas_list);

rows = cols = 1;
switch (n)
  {
  case 2:
    rows = 1;
    cols = 2;
    break;
  case 3:
  case 4:
    rows = 2;
    cols = 2;
    break;
  }

width = sysenv.width / cols;
height = sysenv.height / rows;

#if DEBUG_CANVAS_RESIZE
printf("Splitting (%d, %d) : %d x %d\n", rows, cols, width, height);
#endif

list = sysenv.canvas_list;
for (i=rows ; i-- ; )
  {
  for (j=0 ; j<cols ; j++)
    {
    if (list)
      {
      canvas = list->data;
      canvas->x = j*width;
      canvas->y = i*height;
      canvas->width = width;
      canvas->height = height;

canvas->resize = TRUE;

#if DEBUG_CANVAS_RESIZE
printf(" - canvas (%d, %d) : [%d, %d]\n", i, j, canvas->x, canvas->y);
#endif

      list = g_slist_next(list);    
      }
    }
  }
canvas_shuffle();
}

/*******************************/
/* revert to a single viewport */
/*******************************/
void canvas_single(void)
{
gint i, n;
struct canvas_pak *canvas;

n = g_slist_length(sysenv.canvas_list);

if (n > 1)
  {
  for (i=n-1 ; i-- ; )
    {
    canvas = g_slist_nth_data(sysenv.canvas_list, i);
    sysenv.canvas_list = g_slist_remove(sysenv.canvas_list, canvas);
    }
  }
canvas_resize();
redraw_canvas(SINGLE);
}

/************************************/
/* increase the number of viewports */
/************************************/
void canvas_create(void)
{
gint n;

n = g_slist_length(sysenv.canvas_list);
switch (n)
  {
  case 2:
    canvas_new(0, 0, 0, 0);
  case 1:
  case 0:
    canvas_new(0, 0, 0, 0);
    canvas_resize();
    redraw_canvas(ALL);
    break;
  }
}

/************************************/
/* decrease the number of viewports */
/************************************/
void canvas_delete(void)
{
gint n;
gpointer canvas;

n = g_slist_length(sysenv.canvas_list);

switch (n)
  {
  case 4:
    canvas = sysenv.canvas_list->data;
    sysenv.canvas_list = g_slist_remove(sysenv.canvas_list, canvas);
    canvas_free(canvas);

  case 2:
    canvas = sysenv.canvas_list->data;
    sysenv.canvas_list = g_slist_remove(sysenv.canvas_list, canvas);
    canvas_free(canvas);

/* resize & redraw */
    canvas_resize();
    redraw_canvas(ALL);
    break;
  }
}

/*********************************************/
/* select model at the given canvas position */
/*********************************************/
void canvas_select(gint x, gint y)
{
gint top;
GSList *list;
struct canvas_pak *canvas;

for (list=sysenv.canvas_list ; list ; list=g_slist_next(list))
  {
  canvas = list->data;
  top = sysenv.height - canvas->y - canvas->height;

  if (x >= canvas->x && x < canvas->x+canvas->width)
    {
    if (y >= top && y < top+canvas->height)
      {
      if (canvas->model)
        {
/* only select if not already active -avoid's deselecting graphs */
        if (canvas->model != sysenv.active_model)
          tree_select_model(canvas->model);
        }
      }
    }
  }
}

/***********************************************/
/* get the canvas a model is drawn in (if any) */
/***********************************************/
gpointer canvas_find(struct model_pak *model)
{
GSList *list;
struct canvas_pak *canvas;

for (list=sysenv.canvas_list ; list ; list=g_slist_next(list))
  {
  canvas = list->data;
  if (canvas->model == model)
    return(canvas);
  }
return(NULL);
}
