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

#define G_DISABLE_DEPRECATED
#define GDK_DISABLE_DEPRECATED
#define GDK_PIXBUF_DISABLE_DEPRECATED
#define GLIB_DISABLE_DEPRECATION_WARNINGS

#include "gui_gl.h"

#if GTK_MAJOR_VERSION < 3
#include <gtk/gtkgl.h>
#endif

#if GTK_MAJOR_VERSION >= 3
static void gdis_gl_report_context(GdkGLContext *context)
{
static gboolean reported = FALSE;
gint major = 0, minor = 0;

if (reported || !context || !g_getenv("GDIS_DEBUG_GL"))
  return;

gdk_gl_context_get_version(context, &major, &minor);
g_printerr("GDIS GL context: %d.%d legacy=%d es=%d\n",
           major, minor,
           gdk_gl_context_is_legacy(context),
           gdk_gl_context_get_use_es(context));
reported = TRUE;
}

#if GTK_MAJOR_VERSION < 4
static GdkGLContext *gdis_glarea_create_context(GtkGLArea *area,
                                                gpointer data)
{
GdkWindow *window;
GdkGLContext *context;
GError *error = NULL;

(void) data;

window = gtk_widget_get_window(GTK_WIDGET(area));
if (!window)
  return(NULL);

context = gdk_window_create_gl_context(window, &error);
if (!context)
  {
  if (error)
    {
    gtk_gl_area_set_error(area, error);
    g_error_free(error);
    }
  return(NULL);
  }

gdk_gl_context_set_use_es(context, FALSE);
gdk_gl_context_set_required_version(context, 3, 2);
gdk_gl_context_set_forward_compatible(context, FALSE);

return(context);
}
#endif
#endif

gint gdis_gl_backend_init(gint *argc, gchar ***argv)
{
g_return_val_if_fail(argc != NULL, 1);
g_return_val_if_fail(argv != NULL, 1);

#if GTK_MAJOR_VERSION < 3
gdk_gl_init(argc, argv);
gtk_gl_init(argc, argv);
#endif

return(0);
}

gpointer gdis_gl_config_new(gboolean stereo)
{
#if GTK_MAJOR_VERSION >= 3
if (stereo)
  return(NULL);
return(GINT_TO_POINTER(1));
#else
gint mode;

mode = GDK_GL_MODE_RGB | GDK_GL_MODE_DEPTH | GDK_GL_MODE_DOUBLE;
if (stereo)
  mode |= GDK_GL_STEREO;

return(gdk_gl_config_new_by_mode(mode));
#endif
}

gpointer gdis_gl_config_new_basic(void)
{
#if GTK_MAJOR_VERSION >= 3
return(GINT_TO_POINTER(1));
#else
return(gdk_gl_config_new_by_mode(GDK_GL_MODE_RGB | GDK_GL_MODE_DEPTH));
#endif
}

GtkWidget *gdis_gl_widget_new(gpointer glconfig)
{
#if GTK_MAJOR_VERSION >= 3
GtkWidget *widget;

widget = gtk_gl_area_new();
#if GTK_MAJOR_VERSION >= 4
#if GTK_CHECK_VERSION(4, 14, 0)
gtk_gl_area_set_allowed_apis(GTK_GL_AREA(widget), GDK_GL_API_GL);
#endif
#else
g_signal_connect(widget, "create-context",
                 G_CALLBACK(gdis_glarea_create_context), NULL);
#endif
gtk_gl_area_set_use_es(GTK_GL_AREA(widget), FALSE);
#if GTK_MAJOR_VERSION >= 4
gtk_gl_area_set_required_version(GTK_GL_AREA(widget), 2, 1);
#else
gtk_gl_area_set_required_version(GTK_GL_AREA(widget), 3, 2);
#endif
gtk_gl_area_set_has_depth_buffer(GTK_GL_AREA(widget), TRUE);
gtk_gl_area_set_has_stencil_buffer(GTK_GL_AREA(widget), TRUE);
gtk_gl_area_set_auto_render(GTK_GL_AREA(widget), FALSE);

return(widget);
#else
GtkWidget *widget;

widget = gtk_drawing_area_new();
gdis_gl_widget_enable(widget, glconfig);

return(widget);
#endif
}

gboolean gdis_gl_widget_enable(GtkWidget *widget, gpointer glconfig)
{
g_return_val_if_fail(GTK_IS_WIDGET(widget), FALSE);

#if GTK_MAJOR_VERSION >= 3
(void) glconfig;
return(GTK_IS_GL_AREA(widget));
#else
g_return_val_if_fail(glconfig != NULL, FALSE);

return(gtk_widget_set_gl_capability(widget, GDK_GL_CONFIG(glconfig),
                                    NULL, TRUE, GDK_GL_RGBA_TYPE));
#endif
}

gboolean gdis_gl_begin(GtkWidget *widget)
{
#if GTK_MAJOR_VERSION >= 3
GError *error;
GdkGLContext *context;

g_return_val_if_fail(GTK_IS_GL_AREA(widget), FALSE);

gtk_gl_area_make_current(GTK_GL_AREA(widget));
error = gtk_gl_area_get_error(GTK_GL_AREA(widget));
if (error)
  {
  if (g_getenv("GDIS_DEBUG_GL"))
    g_printerr("GtkGLArea error: %s\n", error->message);
  return(FALSE);
  }

context = gtk_gl_area_get_context(GTK_GL_AREA(widget));
gdis_gl_report_context(context);

return(TRUE);
#else
GdkGLContext *glcontext;
GdkGLDrawable *gldrawable;

g_return_val_if_fail(GTK_IS_WIDGET(widget), FALSE);

glcontext = gtk_widget_get_gl_context(widget);
gldrawable = gtk_widget_get_gl_drawable(widget);
if (!glcontext || !gldrawable)
  return(FALSE);

return(gdk_gl_drawable_gl_begin(gldrawable, glcontext));
#endif
}

gboolean gdis_gl_context_is_legacy(GtkWidget *widget)
{
#if GTK_MAJOR_VERSION >= 3
GdkGLContext *context;

g_return_val_if_fail(GTK_IS_GL_AREA(widget), TRUE);

context = gtk_gl_area_get_context(GTK_GL_AREA(widget));
if (!context)
  return(FALSE);

return(gdk_gl_context_is_legacy(context));
#else
(void) widget;
return(TRUE);
#endif
}

void gdis_gl_end(GtkWidget *widget)
{
#if GTK_MAJOR_VERSION >= 3
(void) widget;
#else
GdkGLDrawable *gldrawable;

g_return_if_fail(GTK_IS_WIDGET(widget));

gldrawable = gtk_widget_get_gl_drawable(widget);
if (gldrawable)
  gdk_gl_drawable_gl_end(gldrawable);
#endif
}

void gdis_gl_swap_buffers(GtkWidget *widget)
{
#if GTK_MAJOR_VERSION >= 3
(void) widget;
#else
GdkGLDrawable *gldrawable;

g_return_if_fail(GTK_IS_WIDGET(widget));

gldrawable = gtk_widget_get_gl_drawable(widget);
if (gldrawable)
  gdk_gl_drawable_swap_buffers(gldrawable);
#endif
}

void gdis_gl_get_size(GtkWidget *widget, gint *w, gint *h)
{
#if GTK_MAJOR_VERSION >= 3
if (w)
  *w = 0;
if (h)
  *h = 0;

g_return_if_fail(GTK_IS_WIDGET(widget));

if (w)
  *w = gtk_widget_get_allocated_width(widget);
if (h)
  *h = gtk_widget_get_allocated_height(widget);
#else
GdkGLDrawable *gldrawable;

if (w)
  *w = 0;
if (h)
  *h = 0;

g_return_if_fail(GTK_IS_WIDGET(widget));

gldrawable = gtk_widget_get_gl_drawable(widget);
if (gldrawable)
  gdk_gl_drawable_get_size(gldrawable, w, h);
#endif
}

GdkWindow *gdis_gl_get_window(GtkWidget *widget)
{
#if GTK_MAJOR_VERSION >= 4
GtkNative *native;

g_return_val_if_fail(GTK_IS_WIDGET(widget), NULL);

native = gtk_widget_get_native(widget);
if (!native)
  return(NULL);

return((GdkWindow *) gtk_native_get_surface(native));
#elif GTK_MAJOR_VERSION >= 3
g_return_val_if_fail(GTK_IS_WIDGET(widget), NULL);

return(gtk_widget_get_window(widget));
#else
GdkGLWindow *glwindow;

g_return_val_if_fail(GTK_IS_WIDGET(widget), NULL);

glwindow = gtk_widget_get_gl_window(widget);
if (!glwindow)
  return(NULL);

return(gdk_gl_window_get_window(glwindow));
#endif
}
