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
#if GTK_MAJOR_VERSION < 3
#include <gtk/gtkgl.h>
#endif

#include "gui_gl.h"

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
gtk_gl_area_set_has_depth_buffer(GTK_GL_AREA(widget), TRUE);
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
g_return_val_if_fail(GTK_IS_GL_AREA(widget), FALSE);

gtk_gl_area_make_current(GTK_GL_AREA(widget));
if (gtk_gl_area_get_error(GTK_GL_AREA(widget)))
  return(FALSE);

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
if (GTK_IS_GL_AREA(widget))
  gtk_gl_area_queue_render(GTK_GL_AREA(widget));
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
#if GTK_MAJOR_VERSION >= 3
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
