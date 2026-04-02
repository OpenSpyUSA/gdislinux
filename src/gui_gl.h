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

#ifndef GDIS_GUI_GL_H
#define GDIS_GUI_GL_H

#include <gtk/gtk.h>

gint gdis_gl_backend_init(gint *argc, gchar ***argv);
gpointer gdis_gl_config_new(gboolean stereo);
gpointer gdis_gl_config_new_basic(void);
GtkWidget *gdis_gl_widget_new(gpointer glconfig);
gboolean gdis_gl_widget_enable(GtkWidget *widget, gpointer glconfig);
gboolean gdis_gl_begin(GtkWidget *widget);
void gdis_gl_end(GtkWidget *widget);
void gdis_gl_swap_buffers(GtkWidget *widget);
void gdis_gl_get_size(GtkWidget *widget, gint *w, gint *h);
GdkWindow *gdis_gl_get_window(GtkWidget *widget);

#endif
