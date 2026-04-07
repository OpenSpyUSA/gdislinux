#ifndef GDIS_GUI_GTK_COMPAT_H
#define GDIS_GUI_GTK_COMPAT_H

#include <gtk/gtk.h>
#include <string.h>
#include <stdlib.h>

#ifndef GTK_OBJECT
#define GTK_OBJECT(object) (object)
#endif

#ifndef GTK_SIGNAL_FUNC
#define GTK_SIGNAL_FUNC(function) G_CALLBACK(function)
#endif

#if GTK_MAJOR_VERSION >= 3
typedef GCallback GtkSignalFunc;

#ifndef GDK_Insert
#define GDK_Insert GDK_KEY_Insert
#endif
#ifndef GDK_Delete
#define GDK_Delete GDK_KEY_Delete
#endif
#ifndef GDK_Escape
#define GDK_Escape GDK_KEY_Escape
#endif
#ifndef GDK_F1
#define GDK_F1 GDK_KEY_F1
#endif
#ifndef GDK_F2
#define GDK_F2 GDK_KEY_F2
#endif
#ifndef GDK_F9
#define GDK_F9 GDK_KEY_F9
#endif
#ifndef GDK_F12
#define GDK_F12 GDK_KEY_F12
#endif
#endif

static inline GtkWidget *gdis_gtk_box_new(GtkOrientation orientation,
                                          gboolean homogeneous,
                                          gint spacing)
{
GtkWidget *box;

#if GTK_MAJOR_VERSION >= 3
box = gtk_box_new(orientation, spacing);
gtk_box_set_homogeneous(GTK_BOX(box), homogeneous);
#else
if (orientation == GTK_ORIENTATION_HORIZONTAL)
  box = gtk_hbox_new(homogeneous, spacing);
else
  box = gtk_vbox_new(homogeneous, spacing);
#endif

return(box);
}

static inline GtkWidget *gdis_gtk_combo_box_text_new(gboolean with_entry)
{
#if GTK_MAJOR_VERSION >= 3
return(with_entry ? gtk_combo_box_text_new_with_entry()
                  : gtk_combo_box_text_new());
#else
GtkListStore *store;
GtkWidget *combo;

store = gtk_list_store_new(1, G_TYPE_STRING);
if (with_entry)
  {
  combo = gtk_combo_box_new_with_model_and_entry(GTK_TREE_MODEL(store));
  gtk_combo_box_set_entry_text_column(GTK_COMBO_BOX(combo), 0);
  }
else
  {
  GtkCellRenderer *renderer;

  combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));
  renderer = gtk_cell_renderer_text_new();
  gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), renderer, TRUE);
  gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(combo), renderer, "text", 0);
  }
g_object_unref(store);

return(combo);
#endif
}

static inline GtkWidget *gdis_gtk_dialog_get_content_widget(GtkWidget *dialog)
{
return(GTK_WIDGET(gtk_dialog_get_content_area(GTK_DIALOG(dialog))));
}

static inline GtkWidget *gdis_gtk_dialog_get_action_widget(GtkWidget *dialog)
{
#if GTK_MAJOR_VERSION >= 4
GtkWidget *action_area;
GtkWidget *content_area;

action_area = g_object_get_data(G_OBJECT(dialog), "gdis-dialog-action-area");
if (action_area)
  return(action_area);

content_area = gdis_gtk_dialog_get_content_widget(dialog);
action_area = gdis_gtk_box_new(GTK_ORIENTATION_HORIZONTAL, FALSE, 6);
gtk_widget_set_halign(action_area, GTK_ALIGN_END);
gtk_widget_set_margin_top(action_area, 6);
gtk_widget_set_margin_bottom(action_area, 6);
gtk_widget_set_margin_start(action_area, 6);
gtk_widget_set_margin_end(action_area, 6);
gtk_box_append(GTK_BOX(content_area), action_area);
g_object_set_data(G_OBJECT(dialog), "gdis-dialog-action-area", action_area);

return(action_area);
#else
return(GTK_WIDGET(gtk_dialog_get_action_area(GTK_DIALOG(dialog))));
#endif
}

#define GDIS_DIALOG_CONTENTS(dialog) \
        gdis_gtk_dialog_get_content_widget(GTK_WIDGET(dialog))
#define GDIS_DIALOG_ACTIONS(dialog) \
        gdis_gtk_dialog_get_action_widget(GTK_WIDGET(dialog))

#if GTK_MAJOR_VERSION < 3
static inline void gdis_gtk_combo_box_set_popup_fixed_width(GtkComboBox *combo,
                                                            gboolean fixed_width)
{
(void) combo;
(void) fixed_width;
}

#define gtk_combo_box_set_popup_fixed_width(combo, fixed_width) \
        gdis_gtk_combo_box_set_popup_fixed_width(GTK_COMBO_BOX(combo), fixed_width)
#endif

#if GTK_MAJOR_VERSION < 4
static inline GdkWindow *gdis_gdk_get_default_root_window(void)
{
return(gdk_get_default_root_window());
}

static inline void gdis_gdk_window_get_geometry(GdkWindow *window,
                                                gint *x,
                                                gint *y,
                                                gint *width,
                                                gint *height,
                                                gint *depth)
{
gdk_window_get_geometry(window, x, y, width, height, depth);
}
#endif

#if GTK_MAJOR_VERSION >= 4
#ifndef GTK_CONTAINER
#define GTK_CONTAINER(widget) (widget)
#endif

typedef gpointer GtkAccelGroup;
typedef gpointer GtkStyle;
typedef gpointer GtkTooltips;
typedef gpointer GdkVisual;
typedef gpointer GdkColormap;
typedef gpointer GdkWindow;
typedef gpointer GdkFont;
typedef gpointer GdkGC;
typedef gpointer GdkPixmap;
typedef gpointer GdkDrawable;
typedef gint GdkVisualType;
typedef GdkButtonEvent GdkEventButton;
typedef GdkEvent GdkEventConfigure;
typedef GdkEvent GdkEventExpose;
typedef GdkKeyEvent GdkEventKey;
typedef GdkMotionEvent GdkEventMotion;
typedef GdkScrollEvent GdkEventScroll;

typedef struct
{
guint32 pixel;
guint16 red;
guint16 green;
guint16 blue;
} GdkColor;

#ifndef GDK_VISUAL_TRUE_COLOR
#define GDK_VISUAL_TRUE_COLOR 0
#endif

#ifndef GTK_ICON_SIZE_BUTTON
#define GTK_ICON_SIZE_BUTTON 0
#endif

#ifndef GTK_WIN_POS_CENTER
#define GTK_WIN_POS_CENTER 0
#endif

#ifndef GTK_WINDOW_TOPLEVEL
#define GTK_WINDOW_TOPLEVEL 0
#endif

#ifndef GTK_TYPE_MENU_BAR
#define GTK_TYPE_MENU_BAR GTK_TYPE_BOX
#endif

#ifndef GTK_STATE_NORMAL
#define GTK_STATE_NORMAL GTK_STATE_FLAG_NORMAL
#endif

#ifndef GTK_FILL
#define GTK_FILL 1
#endif

#ifndef GTK_SHRINK
#define GTK_SHRINK 2
#endif

#ifndef GDK_EXPOSURE_MASK
#define GDK_EXPOSURE_MASK 0
#endif
#ifndef GDK_LEAVE_NOTIFY_MASK
#define GDK_LEAVE_NOTIFY_MASK 0
#endif
#ifndef GDK_BUTTON_PRESS_MASK
#define GDK_BUTTON_PRESS_MASK 0
#endif
#ifndef GDK_BUTTON_RELEASE_MASK
#define GDK_BUTTON_RELEASE_MASK 0
#endif
#ifndef GDK_SCROLL_MASK
#define GDK_SCROLL_MASK 0
#endif
#ifndef GDK_POINTER_MOTION_MASK
#define GDK_POINTER_MOTION_MASK 0
#endif
#ifndef GDK_POINTER_MOTION_HINT_MASK
#define GDK_POINTER_MOTION_HINT_MASK 0
#endif

#ifndef GTK_UPDATE_CONTINUOUS
#define GTK_UPDATE_CONTINUOUS 0
#endif
#ifndef GTK_UPDATE_DISCONTINUOUS
#define GTK_UPDATE_DISCONTINUOUS 1
#endif
#ifndef GTK_UPDATE_DELAYED
#define GTK_UPDATE_DELAYED 2
#endif

#ifndef GTK_EXPAND
#define GTK_EXPAND (1 << 0)
#endif
#ifndef GTK_SHRINK
#define GTK_SHRINK (1 << 1)
#endif
#ifndef GTK_FILL
#define GTK_FILL (1 << 2)
#endif

#ifndef GDK_Insert
#define GDK_Insert GDK_KEY_Insert
#endif
#ifndef GDK_Delete
#define GDK_Delete GDK_KEY_Delete
#endif
#ifndef GDK_Escape
#define GDK_Escape GDK_KEY_Escape
#endif
#ifndef GDK_F1
#define GDK_F1 GDK_KEY_F1
#endif
#ifndef GDK_F2
#define GDK_F2 GDK_KEY_F2
#endif
#ifndef GDK_F9
#define GDK_F9 GDK_KEY_F9
#endif
#ifndef GDK_F12
#define GDK_F12 GDK_KEY_F12
#endif

#ifndef GTK_STOCK_APPLY
#define GTK_STOCK_APPLY "gtk-apply"
#endif
#ifndef GTK_STOCK_CANCEL
#define GTK_STOCK_CANCEL "gtk-cancel"
#endif
#ifndef GTK_STOCK_CLEAR
#define GTK_STOCK_CLEAR "gtk-clear"
#endif
#ifndef GTK_STOCK_CLOSE
#define GTK_STOCK_CLOSE "gtk-close"
#endif
#ifndef GTK_STOCK_DELETE
#define GTK_STOCK_DELETE "gtk-delete"
#endif
#ifndef GTK_STOCK_EXECUTE
#define GTK_STOCK_EXECUTE "gtk-execute"
#endif
#ifndef GTK_STOCK_FIND
#define GTK_STOCK_FIND "gtk-find"
#endif
#ifndef GTK_STOCK_OPEN
#define GTK_STOCK_OPEN "gtk-open"
#endif
#ifndef GTK_STOCK_REFRESH
#define GTK_STOCK_REFRESH "gtk-refresh"
#endif
#ifndef GTK_STOCK_REMOVE
#define GTK_STOCK_REMOVE "gtk-remove"
#endif
#ifndef GTK_STOCK_SAVE
#define GTK_STOCK_SAVE "gtk-save"
#endif

static inline void gdis_gtk_widget_apply_box_layout(GtkWidget *child,
                                                    gboolean expand,
                                                    guint padding)
{
gtk_widget_set_hexpand(child, expand);
gtk_widget_set_vexpand(child, expand);
gtk_widget_set_margin_top(child, padding);
gtk_widget_set_margin_bottom(child, padding);
gtk_widget_set_margin_start(child, padding);
gtk_widget_set_margin_end(child, padding);
}

static inline void gdis_gtk_widget_detach_from_parent(GtkWidget *widget)
{
GtkWidget *parent;

if (!widget)
  return;

parent = gtk_widget_get_parent(widget);
if (!parent)
  return;

if (GTK_IS_BOX(parent))
  {
  gtk_box_remove(GTK_BOX(parent), widget);
  return;
  }
if (GTK_IS_PANED(parent))
  {
  if (gtk_paned_get_start_child(GTK_PANED(parent)) == widget)
    gtk_paned_set_start_child(GTK_PANED(parent), NULL);
  else if (gtk_paned_get_end_child(GTK_PANED(parent)) == widget)
    gtk_paned_set_end_child(GTK_PANED(parent), NULL);
  return;
  }
if (GTK_IS_SCROLLED_WINDOW(parent))
  {
  if (gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(parent)) == widget)
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(parent), NULL);
  return;
  }
if (GTK_IS_FRAME(parent))
  {
  if (gtk_frame_get_child(GTK_FRAME(parent)) == widget)
    gtk_frame_set_child(GTK_FRAME(parent), NULL);
  return;
  }
if (GTK_IS_BUTTON(parent))
  {
  if (gtk_button_get_child(GTK_BUTTON(parent)) == widget)
    gtk_button_set_child(GTK_BUTTON(parent), NULL);
  return;
  }
if (GTK_IS_WINDOW(parent))
  {
  if (gtk_window_get_child(GTK_WINDOW(parent)) == widget)
    gtk_window_set_child(GTK_WINDOW(parent), NULL);
  return;
  }
#if GTK_MAJOR_VERSION >= 4
if (GTK_IS_POPOVER(parent))
  {
  if (gtk_popover_get_child(GTK_POPOVER(parent)) == widget)
    gtk_popover_set_child(GTK_POPOVER(parent), NULL);
  return;
  }
#endif

gtk_widget_unparent(widget);
}

static inline void gdis_gtk_widget_prepare_for_parent(GtkWidget *widget,
                                                      GtkWidget *parent)
{
GtkWidget *old_parent;

if (!widget)
  return;

old_parent = gtk_widget_get_parent(widget);
if (!old_parent || old_parent == parent)
  return;

gdis_gtk_widget_detach_from_parent(widget);
}

static inline void gdis_gtk_widget_destroy(GtkWidget *widget)
{
if (!widget)
  return;

if (GTK_IS_WINDOW(widget))
  {
  gtk_window_destroy(GTK_WINDOW(widget));
  return;
  }

if (!gtk_widget_get_parent(widget))
  {
  g_object_unref(widget);
  return;
  }

gdis_gtk_widget_detach_from_parent(widget);
}

static inline void gdis_gtk_range_set_update_policy(GtkRange *range,
                                                    gint policy)
{
(void) range;
(void) policy;
}

static inline GtkWidget *gdis_gtk_bin_get_child(GtkWidget *widget)
{
if (!widget)
  return(NULL);

if (GTK_IS_COMBO_BOX(widget))
  return(gtk_combo_box_get_child(GTK_COMBO_BOX(widget)));
if (GTK_IS_WINDOW(widget))
  return(gtk_window_get_child(GTK_WINDOW(widget)));
if (GTK_IS_FRAME(widget))
  return(gtk_frame_get_child(GTK_FRAME(widget)));
if (GTK_IS_BUTTON(widget))
  return(gtk_button_get_child(GTK_BUTTON(widget)));
if (GTK_IS_SCROLLED_WINDOW(widget))
  return(gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(widget)));

return(gtk_widget_get_first_child(widget));
}

static inline GtkWidget *gdis_gtk_color_button_new_with_color(const GdkColor *colour)
{
GdkRGBA rgba = {0.0, 0.0, 0.0, 1.0};

if (colour)
  {
  rgba.red = colour->red * INV_COLOUR_SCALE;
  rgba.green = colour->green * INV_COLOUR_SCALE;
  rgba.blue = colour->blue * INV_COLOUR_SCALE;
  }

return(gtk_color_button_new_with_rgba(&rgba));
}

static inline GtkWidget *gdis_gtk_table_new(guint rows,
                                            guint columns,
                                            gboolean homogeneous)
{
GtkWidget *grid;

(void) rows;
(void) columns;

grid = gtk_grid_new();
gtk_grid_set_row_homogeneous(GTK_GRID(grid), homogeneous);
gtk_grid_set_column_homogeneous(GTK_GRID(grid), homogeneous);

return(grid);
}

static inline void gdis_gtk_table_attach_defaults(GtkGrid *table,
                                                  GtkWidget *child,
                                                  guint left_attach,
                                                  guint right_attach,
                                                  guint top_attach,
                                                  guint bottom_attach)
{
gtk_widget_set_hexpand(child, TRUE);
gtk_widget_set_vexpand(child, TRUE);
gtk_grid_attach(table, child,
                left_attach, top_attach,
                MAXIMUM(1, (gint) (right_attach - left_attach)),
                MAXIMUM(1, (gint) (bottom_attach - top_attach)));
}

static inline void gdis_gtk_table_attach(GtkGrid *table,
                                         GtkWidget *child,
                                         guint left_attach,
                                         guint right_attach,
                                         guint top_attach,
                                         guint bottom_attach,
                                         guint xoptions,
                                         guint yoptions,
                                         guint xpadding,
                                         guint ypadding)
{
gtk_widget_set_hexpand(child, (xoptions & GTK_FILL) || (xoptions & GTK_EXPAND));
gtk_widget_set_vexpand(child, (yoptions & GTK_FILL) || (yoptions & GTK_EXPAND));
gtk_widget_set_margin_start(child, xpadding);
gtk_widget_set_margin_end(child, xpadding);
gtk_widget_set_margin_top(child, ypadding);
gtk_widget_set_margin_bottom(child, ypadding);
gtk_grid_attach(table, child,
                left_attach, top_attach,
                MAXIMUM(1, (gint) (right_attach - left_attach)),
                MAXIMUM(1, (gint) (bottom_attach - top_attach)));
}

static inline void gdis_gtk_table_set_row_spacings(GtkGrid *table,
                                                   guint spacing)
{
gtk_grid_set_row_spacing(table, spacing);
}

static inline void gdis_gtk_table_set_col_spacings(GtkGrid *table,
                                                   guint spacing)
{
gtk_grid_set_column_spacing(table, spacing);
}

static inline GtkWidget *gdis_gtk_entry_new_with_max_length(gint max)
{
GtkWidget *entry;

entry = gtk_entry_new();
gtk_entry_set_max_length(GTK_ENTRY(entry), max);

return(entry);
}

static inline GtkWidget *gdis_gtk_hseparator_new(void)
{
return(gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
}

static inline GtkWidget *gdis_gtk_vseparator_new(void)
{
return(gtk_separator_new(GTK_ORIENTATION_VERTICAL));
}

static inline GtkWidget *gdis_gtk_hscale_new_with_range(gdouble min,
                                                        gdouble max,
                                                        gdouble step)
{
return(gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, min, max, step));
}

static inline GtkWidget *gdis_gtk_vscale_new_with_range(gdouble min,
                                                        gdouble max,
                                                        gdouble step)
{
return(gtk_scale_new_with_range(GTK_ORIENTATION_VERTICAL, min, max, step));
}

static inline GtkWidget *gdis_gtk_radio_button_new_with_label(GSList *group,
                                                              const gchar *label)
{
GtkWidget *button;
GSList *button_group;

button = gtk_toggle_button_new_with_label(label);

if (group && group->data)
  {
  gtk_toggle_button_set_group(GTK_TOGGLE_BUTTON(button),
                              GTK_TOGGLE_BUTTON(group->data));
  button_group = g_slist_copy(group);
  }
else
  {
  button_group = NULL;
  }

button_group = g_slist_prepend(button_group, button);
g_object_set_data_full(G_OBJECT(button), "gdis-radio-group",
                       button_group, (GDestroyNotify) g_slist_free);

return(button);
}

static inline GSList *gdis_gtk_radio_button_group(GtkToggleButton *button)
{
GSList *group;

if (!button)
  return(NULL);

group = g_object_get_data(G_OBJECT(button), "gdis-radio-group");
if (!group)
  {
  group = g_slist_prepend(NULL, button);
  g_object_set_data_full(G_OBJECT(button), "gdis-radio-group",
                         group, (GDestroyNotify) g_slist_free);
  }

return(group);
}

typedef struct
{
GMainLoop *loop;
gint response;
} GdisGtkDialogRunState;

static inline void gdis_gtk_dialog_run_response_cb(GtkDialog *dialog,
                                                   gint response_id,
                                                   gpointer user_data)
{
GdisGtkDialogRunState *state = user_data;

(void) dialog;

state->response = response_id;
if (state->loop && g_main_loop_is_running(state->loop))
  g_main_loop_quit(state->loop);
}

static inline void gdis_gtk_dialog_run_destroy_cb(GtkWidget *widget,
                                                  gpointer user_data)
{
GdisGtkDialogRunState *state = user_data;

(void) widget;

if (state->response == GTK_RESPONSE_NONE)
  state->response = GTK_RESPONSE_DELETE_EVENT;
if (state->loop && g_main_loop_is_running(state->loop))
  g_main_loop_quit(state->loop);
}

static inline gint gdis_gtk_dialog_run(GtkDialog *dialog)
{
GdisGtkDialogRunState state;
gulong response_handler;
gulong destroy_handler;

g_return_val_if_fail(dialog != NULL, GTK_RESPONSE_NONE);

state.loop = g_main_loop_new(NULL, FALSE);
state.response = GTK_RESPONSE_NONE;

g_object_ref(dialog);
response_handler = g_signal_connect(dialog, "response",
                                    G_CALLBACK(gdis_gtk_dialog_run_response_cb),
                                    &state);
destroy_handler = g_signal_connect(dialog, "destroy",
                                   G_CALLBACK(gdis_gtk_dialog_run_destroy_cb),
                                   &state);

gtk_window_present(GTK_WINDOW(dialog));
g_main_loop_run(state.loop);

if (g_signal_handler_is_connected(dialog, response_handler))
  g_signal_handler_disconnect(dialog, response_handler);
if (g_signal_handler_is_connected(dialog, destroy_handler))
  g_signal_handler_disconnect(dialog, destroy_handler);

g_main_loop_unref(state.loop);
g_object_unref(dialog);

return(state.response);
}

static inline gchar *gdis_gtk_file_chooser_get_filename(GtkFileChooser *chooser)
{
GFile *file;
gchar *path;

file = gtk_file_chooser_get_file(chooser);
if (!file)
  return(NULL);

path = g_file_get_path(file);
g_object_unref(file);

return(path);
}

static inline void gdis_gtk_text_buffer_insert_pixbuf(GtkTextBuffer *buffer,
                                                      GtkTextIter *iter,
                                                      GdkPixbuf *pixbuf)
{
GdkTexture *texture;

g_return_if_fail(buffer != NULL);
g_return_if_fail(iter != NULL);
g_return_if_fail(pixbuf != NULL);

texture = gdk_texture_new_for_pixbuf(pixbuf);
gtk_text_buffer_insert_paintable(buffer, iter, GDK_PAINTABLE(texture));
g_object_unref(texture);
}

static inline GdkWindow *gdis_gdk_get_default_root_window(void)
{
return(GINT_TO_POINTER(1));
}

static inline void gdis_gdk_window_get_geometry(GdkWindow *window,
                                                gint *x,
                                                gint *y,
                                                gint *width,
                                                gint *height,
                                                gint *depth)
{
GdkDisplay *display;
GListModel *monitors;
GdkMonitor *monitor;
GdkRectangle geometry;

(void) window;

geometry.x = 0;
geometry.y = 0;
geometry.width = 1024;
geometry.height = 768;

display = gdk_display_get_default();
if (display)
  {
  monitors = gdk_display_get_monitors(display);
  if (monitors && g_list_model_get_n_items(monitors) > 0)
    {
    monitor = g_list_model_get_item(monitors, 0);
    if (monitor)
      {
      gdk_monitor_get_geometry(monitor, &geometry);
      g_object_unref(monitor);
      }
    }
  }

if (x)
  *x = geometry.x;
if (y)
  *y = geometry.y;
if (width)
  *width = geometry.width;
if (height)
  *height = geometry.height;
if (depth)
  *depth = 24;
}

static inline GtkAccelGroup *gdis_gtk_accel_group_new(void)
{
return(GINT_TO_POINTER(1));
}

static inline void gdis_gtk_window_add_accel_group(GtkWindow *window,
                                                   GtkAccelGroup *accel_group)
{
(void) window;
(void) accel_group;
}

static inline void gdis_gdk_flush(void)
{
}

static inline void gdis_gdk_beep(void)
{
}

static inline void gdis_gdk_threads_init(void)
{
}

static inline void gdis_gtk_box_pack_start(GtkBox *box,
                                           GtkWidget *child,
                                           gboolean expand,
                                           gboolean fill,
                                           guint padding)
{
(void) fill;

gdis_gtk_widget_prepare_for_parent(child, GTK_WIDGET(box));
gdis_gtk_widget_apply_box_layout(child, expand, padding);
gtk_box_append(box, child);
}

static inline void gdis_gtk_box_pack_end(GtkBox *box,
                                         GtkWidget *child,
                                         gboolean expand,
                                         gboolean fill,
                                         guint padding)
{
(void) fill;

gdis_gtk_widget_prepare_for_parent(child, GTK_WIDGET(box));
gdis_gtk_widget_apply_box_layout(child, expand, padding);
gtk_box_append(box, child);
}

static inline void gdis_gtk_container_add(GtkWidget *container,
                                          GtkWidget *child)
{
if (GTK_IS_WINDOW(container))
  {
  gdis_gtk_widget_prepare_for_parent(child, container);
  gtk_window_set_child(GTK_WINDOW(container), child);
  return;
  }
if (GTK_IS_SCROLLED_WINDOW(container))
  {
  gdis_gtk_widget_prepare_for_parent(child, container);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(container), child);
  return;
  }
if (GTK_IS_FRAME(container))
  {
  gdis_gtk_widget_prepare_for_parent(child, container);
  gtk_frame_set_child(GTK_FRAME(container), child);
  return;
  }
if (GTK_IS_BUTTON(container))
  {
  gdis_gtk_widget_prepare_for_parent(child, container);
  gtk_button_set_child(GTK_BUTTON(container), child);
  return;
  }
if (GTK_IS_BOX(container))
  {
  gdis_gtk_widget_prepare_for_parent(child, container);
  gtk_box_append(GTK_BOX(container), child);
  return;
  }
}

static inline void gdis_gtk_container_set_border_width(GtkWidget *container,
                                                       guint border_width)
{
gtk_widget_set_margin_top(container, border_width);
gtk_widget_set_margin_bottom(container, border_width);
gtk_widget_set_margin_start(container, border_width);
gtk_widget_set_margin_end(container, border_width);
}

static inline void gdis_gtk_widget_show_all(GtkWidget *widget)
{
gtk_widget_set_visible(widget, TRUE);
if (GTK_IS_WINDOW(widget))
  gtk_window_present(GTK_WINDOW(widget));
}

static inline GtkTooltips *gdis_gtk_tooltips_new(void)
{
return(GINT_TO_POINTER(1));
}

static inline void gdis_gtk_tooltips_set_delay(GtkTooltips *tooltips,
                                               guint delay)
{
(void) tooltips;
(void) delay;
}

static inline void gdis_gtk_tooltips_set_tip(GtkTooltips *tooltips,
                                             GtkWidget *widget,
                                             const gchar *text,
                                             const gchar *private_text)
{
(void) tooltips;
(void) private_text;

if (widget)
  gtk_widget_set_tooltip_text(widget, text);
}

static inline const gchar *gdis_gtk_stock_icon_name(const gchar *id)
{
if (g_strcmp0(id, GTK_STOCK_APPLY) == 0)
  return("object-select-symbolic");
if (g_strcmp0(id, GTK_STOCK_CANCEL) == 0)
  return("process-stop-symbolic");
if (g_strcmp0(id, GTK_STOCK_CLEAR) == 0)
  return("edit-clear-symbolic");
if (g_strcmp0(id, GTK_STOCK_CLOSE) == 0)
  return("window-close-symbolic");
if (g_strcmp0(id, GTK_STOCK_DELETE) == 0)
  return("edit-delete-symbolic");
if (g_strcmp0(id, GTK_STOCK_EXECUTE) == 0)
  return("system-run-symbolic");
if (g_strcmp0(id, GTK_STOCK_FIND) == 0)
  return("edit-find-symbolic");
if (g_strcmp0(id, GTK_STOCK_OPEN) == 0)
  return("document-open-symbolic");
if (g_strcmp0(id, GTK_STOCK_REFRESH) == 0)
  return("view-refresh-symbolic");
if (g_strcmp0(id, GTK_STOCK_REMOVE) == 0)
  return("list-remove-symbolic");
if (g_strcmp0(id, GTK_STOCK_SAVE) == 0)
  return("document-save-symbolic");

return(NULL);
}

static inline const gchar *gdis_gtk_stock_label(const gchar *id)
{
if (g_strcmp0(id, GTK_STOCK_APPLY) == 0)
  return("_Apply");
if (g_strcmp0(id, GTK_STOCK_CANCEL) == 0)
  return("_Cancel");
if (g_strcmp0(id, GTK_STOCK_CLEAR) == 0)
  return("C_lear");
if (g_strcmp0(id, GTK_STOCK_CLOSE) == 0)
  return("_Close");
if (g_strcmp0(id, GTK_STOCK_DELETE) == 0)
  return("_Delete");
if (g_strcmp0(id, GTK_STOCK_EXECUTE) == 0)
  return("_Execute");
if (g_strcmp0(id, GTK_STOCK_FIND) == 0)
  return("_Find");
if (g_strcmp0(id, GTK_STOCK_OPEN) == 0)
  return("_Open");
if (g_strcmp0(id, GTK_STOCK_REFRESH) == 0)
  return("_Refresh");
if (g_strcmp0(id, GTK_STOCK_REMOVE) == 0)
  return("_Remove");
if (g_strcmp0(id, GTK_STOCK_SAVE) == 0)
  return("_Save");

return(id);
}

static inline GtkWidget *gdis_gtk_image_new_from_stock(const gchar *id,
                                                       GtkIconSize size)
{
const gchar *icon_name;

(void) size;

icon_name = gdis_gtk_stock_icon_name(id);
if (!icon_name)
  return(gtk_image_new());

return(gtk_image_new_from_icon_name(icon_name));
}

static inline GtkWidget *gdis_gtk_button_new_from_stock(const gchar *id)
{
GtkWidget *button;
GtkWidget *box;
GtkWidget *image;
GtkWidget *label;

button = gtk_button_new();
box = gdis_gtk_box_new(GTK_ORIENTATION_HORIZONTAL, FALSE, 6);
image = gdis_gtk_image_new_from_stock(id, 0);
label = gtk_label_new_with_mnemonic(gdis_gtk_stock_label(id));
gtk_box_append(GTK_BOX(box), image);
gtk_box_append(GTK_BOX(box), label);
gtk_button_set_child(GTK_BUTTON(button), box);

return(button);
}

typedef struct
{
const gchar *path;
const gchar *accelerator;
gpointer callback;
guint callback_action;
const gchar *item_type;
} GtkItemFactoryEntry;

typedef struct
{
GtkWidget *menu_bar;
GtkAccelGroup *accel_group;
GHashTable *menu_sections;
} GtkItemFactory;

typedef GtkWidget GtkToolbar;
typedef GtkWidget GtkToolItem;

#define GTK_TOOLBAR(widget) GTK_WIDGET(widget)
#define gtk_hbox_new(homogeneous, spacing) \
        gdis_gtk_box_new(GTK_ORIENTATION_HORIZONTAL, homogeneous, spacing)
#define gtk_vbox_new(homogeneous, spacing) \
        gdis_gtk_box_new(GTK_ORIENTATION_VERTICAL, homogeneous, spacing)
#define gtk_combo_box_new_text() gtk_combo_box_text_new()
#define gtk_combo_box_append_text(combo, text) \
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), text)
#define gtk_combo_box_get_active_text(combo) \
        gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(combo))
#define GTK_MISC(widget) GTK_WIDGET(widget)
#define GTK_BIN(widget) GTK_WIDGET(widget)
#define GTK_IS_BIN(widget) (gdis_gtk_bin_get_child(GTK_WIDGET(widget)) != NULL)
#define GTK_RADIO_BUTTON(widget) GTK_TOGGLE_BUTTON(widget)
#define gtk_bin_get_child(bin) \
        gdis_gtk_bin_get_child(GTK_WIDGET(bin))
#define GTK_TABLE(widget) GTK_GRID(widget)
#define gtk_entry_get_text(entry) \
        gtk_editable_get_text(GTK_EDITABLE(entry))
#define gtk_entry_set_text(entry, text) \
        gtk_editable_set_text(GTK_EDITABLE(entry), text)
#define gtk_entry_set_editable(entry, editable) \
        gtk_editable_set_editable(GTK_EDITABLE(entry), editable)
#define gtk_entry_set_width_chars(entry, width_chars) \
        gtk_editable_set_width_chars(GTK_EDITABLE(entry), width_chars)
#define gtk_entry_new_with_max_length(max) \
        gdis_gtk_entry_new_with_max_length(max)
#define gtk_hscale_new_with_range(min, max, step) \
        gdis_gtk_hscale_new_with_range(min, max, step)
#define gtk_vscale_new_with_range(min, max, step) \
        gdis_gtk_vscale_new_with_range(min, max, step)
#define gtk_hseparator_new() \
        gdis_gtk_hseparator_new()
#define gtk_vseparator_new() \
        gdis_gtk_vseparator_new()
#define gtk_table_new(rows, cols, homogeneous) \
        gdis_gtk_table_new(rows, cols, homogeneous)
#define gtk_table_attach_defaults(table, child, left, right, top, bottom) \
        gdis_gtk_table_attach_defaults(GTK_GRID(table), child, left, right, top, bottom)
#define gtk_table_attach(table, child, left, right, top, bottom, xoptions, yoptions, xpadding, ypadding) \
        gdis_gtk_table_attach(GTK_GRID(table), child, left, right, top, bottom, xoptions, yoptions, xpadding, ypadding)
#define gtk_table_set_row_spacings(table, spacing) \
        gdis_gtk_table_set_row_spacings(GTK_GRID(table), spacing)
#define gtk_table_set_col_spacings(table, spacing) \
        gdis_gtk_table_set_col_spacings(GTK_GRID(table), spacing)
#define gtk_radio_button_new_with_label(group, label) \
        gdis_gtk_radio_button_new_with_label(group, label)
#define gtk_radio_button_group(button) \
        gdis_gtk_radio_button_group(GTK_TOGGLE_BUTTON(button))

static inline void gdis_gtk_scrolled_window_add_with_viewport(GtkScrolledWindow *swin,
                                                              GtkWidget *child)
{
gdis_gtk_widget_prepare_for_parent(child, GTK_WIDGET(swin));
gtk_scrolled_window_set_child(swin, child);
}

#define gtk_scrolled_window_add_with_viewport gdis_gtk_scrolled_window_add_with_viewport

static inline GtkItemFactory *gtk_item_factory_new(GType type,
                                                   const gchar *path,
                                                   GtkAccelGroup *accel_group)
{
GtkItemFactory *factory;

(void) type;
(void) path;

factory = g_new0(GtkItemFactory, 1);
factory->menu_bar = gdis_gtk_box_new(GTK_ORIENTATION_HORIZONTAL, FALSE, 4);
factory->accel_group = accel_group;
factory->menu_sections = g_hash_table_new_full(g_str_hash, g_str_equal,
                                               g_free, NULL);

return(factory);
}

static inline gboolean gdis_gtk_item_factory_is_top_level_path(const gchar *path)
{
const gchar *rest;

if (!path || *path != '/')
  return(FALSE);

rest = strchr(path + 1, '/');
return(rest == NULL);
}

static inline gchar *gdis_gtk_item_factory_clean_label(const gchar *label)
{
gchar *clean;
const gchar *src;
gchar *dst;

if (!label)
  return(g_strdup(""));

clean = g_strdup(label);
dst = clean;
for (src=label ; *src ; src++)
  {
  if (*src != '_')
    *dst++ = *src;
  }
*dst = '\0';

return(clean);
}

static inline gchar *gdis_gtk_item_factory_top_label(const gchar *path)
{
gchar **parts;
gchar *label;

if (!path || *path != '/')
  return(g_strdup(""));

parts = g_strsplit(path + 1, "/", 2);
label = gdis_gtk_item_factory_clean_label(parts[0]);
g_strfreev(parts);

return(label);
}

static inline gchar *gdis_gtk_item_factory_action_label(const gchar *path)
{
gchar **parts;
GString *label;
guint i;

if (!path || *path != '/')
  return(g_strdup(""));

parts = g_strsplit(path + 1, "/", -1);
label = g_string_new(NULL);

for (i=1 ; parts[i] ; i++)
  {
  gchar *segment;

  if (label->len)
    g_string_append(label, " > ");

  segment = gdis_gtk_item_factory_clean_label(parts[i]);
  g_string_append(label, segment);
  g_free(segment);
  }

g_strfreev(parts);

return(g_string_free(label, FALSE));
}

static inline void gdis_gtk_item_factory_debug_menu_state(GObject *object,
                                                          GParamSpec *pspec,
                                                          gpointer data)
{
GtkMenuButton *menu_button;
GtkPopover *popover;

(void) pspec;

if (!g_getenv("GDIS_GTK4_MENU_DEBUG"))
  return;

menu_button = GTK_MENU_BUTTON(object);
popover = gtk_menu_button_get_popover(menu_button);

g_printerr("GDIS GTK4 menu debug: %s active=%d popover=%p\n",
           (const gchar *) data,
           gtk_menu_button_get_active(menu_button),
           popover);
}

static inline GtkWidget *gdis_gtk_item_factory_section_box(GtkItemFactory *factory,
                                                           const gchar *path)
{
GtkWidget *menu_button;
GtkWidget *popover;
GtkWidget *scroller;
GtkWidget *box;
gchar *top_label;

g_return_val_if_fail(factory != NULL, NULL);

top_label = gdis_gtk_item_factory_top_label(path);
box = g_hash_table_lookup(factory->menu_sections, top_label);
if (box)
  {
  g_free(top_label);
  return(box);
  }

  menu_button = gtk_menu_button_new();
  gtk_box_append(GTK_BOX(factory->menu_bar), menu_button);
  gtk_menu_button_set_label(GTK_MENU_BUTTON(menu_button), top_label);
  gtk_menu_button_set_direction(GTK_MENU_BUTTON(menu_button), GTK_ARROW_DOWN);
  popover = gtk_popover_new();
  scroller = gtk_scrolled_window_new();
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroller),
                                 GTK_POLICY_NEVER,
                                 GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_has_frame(GTK_SCROLLED_WINDOW(scroller), FALSE);
  gtk_scrolled_window_set_min_content_width(GTK_SCROLLED_WINDOW(scroller), 320);
  gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scroller), 420);
  gtk_scrolled_window_set_max_content_width(GTK_SCROLLED_WINDOW(scroller), 360);
  gtk_scrolled_window_set_max_content_height(GTK_SCROLLED_WINDOW(scroller), 760);
  gtk_scrolled_window_set_propagate_natural_width(GTK_SCROLLED_WINDOW(scroller),
                                                  TRUE);
  gtk_scrolled_window_set_propagate_natural_height(GTK_SCROLLED_WINDOW(scroller),
                                                   TRUE);
  box = gdis_gtk_box_new(GTK_ORIENTATION_VERTICAL, FALSE, 2);
  gtk_widget_set_margin_top(box, 6);
  gtk_widget_set_margin_bottom(box, 6);
  gtk_widget_set_margin_start(box, 6);
  gtk_widget_set_margin_end(box, 6);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroller), box);
  gtk_popover_set_child(GTK_POPOVER(popover), scroller);
  gtk_menu_button_set_popover(GTK_MENU_BUTTON(menu_button), popover);
  if (g_getenv("GDIS_GTK4_MENU_DEBUG"))
    {
    g_printerr("GDIS GTK4 menu init: %s popover=%p box=%p\n",
               top_label, popover, box);
    g_signal_connect(menu_button, "notify::active",
                     G_CALLBACK(gdis_gtk_item_factory_debug_menu_state),
                     top_label);
    }
  g_hash_table_insert(factory->menu_sections, top_label, box);

return(box);
}

static inline void gtk_item_factory_create_items(GtkItemFactory *factory,
                                                 guint n_entries,
                                                 GtkItemFactoryEntry *entries,
                                                 gpointer callback_data)
{
guint i;

(void) callback_data;

for (i=0 ; i<n_entries ; i++)
  {
  GtkWidget *section;

  section = gdis_gtk_item_factory_section_box(factory, entries[i].path);
  if (!section)
    continue;

  if (entries[i].item_type &&
      g_ascii_strcasecmp(entries[i].item_type, "<Branch>") == 0)
    {
    GtkWidget *header;
    gchar *label;

    if (gdis_gtk_item_factory_is_top_level_path(entries[i].path))
      continue;

    label = gdis_gtk_item_factory_action_label(entries[i].path);
    header = gtk_label_new(label);
    gtk_widget_set_halign(header, GTK_ALIGN_START);
    gtk_widget_set_margin_top(header, 4);
    gtk_widget_set_margin_bottom(header, 2);
    gtk_box_append(GTK_BOX(section), header);
    g_free(label);
    continue;
    }

  if (entries[i].item_type &&
      g_ascii_strcasecmp(entries[i].item_type, "<Separator>") == 0)
    {
    gtk_box_append(GTK_BOX(section),
                   gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
    continue;
    }

  {
  GtkWidget *item;
  gchar *label;

  label = gdis_gtk_item_factory_action_label(entries[i].path);
  item = gtk_button_new_with_label(label);
  gtk_widget_set_halign(item, GTK_ALIGN_FILL);
  if (entries[i].callback)
    g_signal_connect(item, "clicked", G_CALLBACK(entries[i].callback),
                     entries[i].callback_action ?
                       GINT_TO_POINTER(entries[i].callback_action) : NULL);
  gtk_box_append(GTK_BOX(section), item);
  g_free(label);
  }
  }
}

static inline GtkWidget *gtk_item_factory_get_widget(GtkItemFactory *factory,
                                                     const gchar *path)
{
(void) path;

return(factory->menu_bar);
}

static inline GtkWidget *gtk_toolbar_new(void)
{
return(gdis_gtk_box_new(GTK_ORIENTATION_HORIZONTAL, FALSE, 2));
}

static inline void gdis_gtk_init(gint *argc, gchar ***argv)
{
(void) argc;
(void) argv;

gtk_init();
}

static inline GtkWidget *gdis_gtk_window_new(gint type)
{
(void) type;

return(gtk_window_new());
}

static inline GtkWidget *gdis_gtk_scrolled_window_new(GtkAdjustment *hadjustment,
                                                      GtkAdjustment *vadjustment)
{
(void) hadjustment;
(void) vadjustment;

return(gtk_scrolled_window_new());
}

static inline GtkWidget *gdis_gtk_hpaned_new(void)
{
return(gtk_paned_new(GTK_ORIENTATION_HORIZONTAL));
}

static inline GtkWidget *gdis_gtk_vpaned_new(void)
{
return(gtk_paned_new(GTK_ORIENTATION_VERTICAL));
}

static inline void gdis_gtk_paned_pack1(GtkPaned *paned,
                                        GtkWidget *child,
                                        gboolean resize,
                                        gboolean shrink)
{
gtk_paned_set_start_child(paned, child);
gtk_paned_set_resize_start_child(paned, resize);
gtk_paned_set_shrink_start_child(paned, shrink);
}

static inline void gdis_gtk_paned_pack2(GtkPaned *paned,
                                        GtkWidget *child,
                                        gboolean resize,
                                        gboolean shrink)
{
gtk_paned_set_end_child(paned, child);
gtk_paned_set_resize_end_child(paned, resize);
gtk_paned_set_shrink_end_child(paned, shrink);
}

static inline GtkWidget *gdis_gtk_event_box_new(void)
{
return(gdis_gtk_box_new(GTK_ORIENTATION_HORIZONTAL, FALSE, 0));
}

static inline void gdis_gtk_widget_set_events(GtkWidget *widget, gint events)
{
(void) widget;
(void) events;
}

static inline void gdis_gtk_toolbar_append_space(GtkToolbar *toolbar)
{
GtkWidget *item;

item = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
gtk_box_append(GTK_BOX(toolbar), item);
}

static inline GtkToolItem *gdis_gtk_toolbar_append_item(GtkToolbar *toolbar,
                                                        const gchar *text,
                                                        const gchar *tooltip,
                                                        const gchar *tooltip_private,
                                                        GtkWidget *icon,
                                                        GCallback callback,
                                                        gpointer user_data)
{
GtkWidget *button;
GtkWidget *box;

(void) tooltip_private;

button = gtk_button_new();
box = gdis_gtk_box_new(GTK_ORIENTATION_HORIZONTAL, FALSE, 4);
if (icon)
  gtk_box_append(GTK_BOX(box), icon);
if (text)
  gtk_box_append(GTK_BOX(box), gtk_label_new(text));
gtk_button_set_child(GTK_BUTTON(button), box);
if (tooltip)
  gtk_widget_set_tooltip_text(button, tooltip);
if (callback)
  g_signal_connect(button, "clicked", callback, user_data);
gtk_box_append(GTK_BOX(toolbar), button);

return(button);
}

static inline GtkToolItem *gdis_gtk_toolbar_append_widget(GtkToolbar *toolbar,
                                                          GtkWidget *widget,
                                                          const gchar *tooltip,
                                                          const gchar *tooltip_private)
{
(void) tooltip_private;

if (tooltip)
  gtk_widget_set_tooltip_text(widget, tooltip);
gtk_box_append(GTK_BOX(toolbar), widget);

return(widget);
}

static inline GdkVisual *gdis_gdk_visual_get_best_with_type(GdkVisualType type)
{
(void) type;

return(GINT_TO_POINTER(1));
}

static inline GdkColormap *gdis_gdk_colormap_new(GdkVisual *visual, gboolean allocate)
{
(void) allocate;

return(visual ? visual : GINT_TO_POINTER(1));
}

static inline gint gdis_gdk_visual_get_depth(GdkVisual *visual)
{
(void) visual;

return(24);
}

static inline void gdis_gtk_window_set_policy(GtkWindow *window,
                                              gboolean allow_shrink,
                                              gboolean allow_grow,
                                              gboolean auto_shrink)
{
(void) allow_shrink;
(void) auto_shrink;

gtk_window_set_resizable(window, allow_grow);
}

static inline void gdis_gtk_misc_set_alignment(GtkWidget *widget,
                                               gfloat xalign,
                                               gfloat yalign)
{
GtkAlign halign = GTK_ALIGN_FILL;
GtkAlign valign = GTK_ALIGN_FILL;

if (xalign <= 0.0)
  halign = GTK_ALIGN_START;
else if (xalign >= 1.0)
  halign = GTK_ALIGN_END;
else
  halign = GTK_ALIGN_CENTER;

if (yalign <= 0.0)
  valign = GTK_ALIGN_START;
else if (yalign >= 1.0)
  valign = GTK_ALIGN_END;
else
  valign = GTK_ALIGN_CENTER;

gtk_widget_set_halign(widget, halign);
gtk_widget_set_valign(widget, valign);
if (GTK_IS_LABEL(widget))
  gtk_label_set_xalign(GTK_LABEL(widget), xalign);
}

#define gtk_box_pack_start(box, child, expand, fill, padding) \
        gdis_gtk_box_pack_start(GTK_BOX(box), child, expand, fill, padding)
#define gtk_box_pack_end(box, child, expand, fill, padding) \
        gdis_gtk_box_pack_end(GTK_BOX(box), child, expand, fill, padding)
#define gtk_init(argc, argv) \
        gdis_gtk_init(argc, argv)
#define gtk_window_new(type) \
        gdis_gtk_window_new(type)
#define gtk_container_add(container, child) \
        gdis_gtk_container_add(GTK_WIDGET(container), child)
#define gtk_container_set_border_width(container, border_width) \
        gdis_gtk_container_set_border_width(GTK_WIDGET(container), border_width)
#define gtk_container_border_width(container, border_width) \
        gtk_container_set_border_width(container, border_width)
#define gtk_widget_show_all(widget) \
        gdis_gtk_widget_show_all(GTK_WIDGET(widget))
#define gtk_tooltips_new() \
        gdis_gtk_tooltips_new()
#define gtk_tooltips_set_delay(tooltips, delay) \
        gdis_gtk_tooltips_set_delay(tooltips, delay)
#define gtk_tooltips_set_tip(tooltips, widget, text, private_text) \
        gdis_gtk_tooltips_set_tip(tooltips, widget, text, private_text)
#define gtk_button_new_from_stock(id) \
        gdis_gtk_button_new_from_stock(id)
#define gtk_image_new_from_stock(id, size) \
        gdis_gtk_image_new_from_stock(id, size)
#define gtk_scrolled_window_new(hadjustment, vadjustment) \
        gdis_gtk_scrolled_window_new(hadjustment, vadjustment)
#define gtk_hpaned_new() \
        gdis_gtk_hpaned_new()
#define gtk_vpaned_new() \
        gdis_gtk_vpaned_new()
#define gtk_paned_pack1(paned, child, resize, shrink) \
        gdis_gtk_paned_pack1(GTK_PANED(paned), child, resize, shrink)
#define gtk_paned_pack2(paned, child, resize, shrink) \
        gdis_gtk_paned_pack2(GTK_PANED(paned), child, resize, shrink)
#define gtk_event_box_new() \
        gdis_gtk_event_box_new()
#define gtk_widget_set_events(widget, events) \
        gdis_gtk_widget_set_events(GTK_WIDGET(widget), events)
#define gtk_widget_destroy gdis_gtk_widget_destroy
#define gtk_widget_modify_bg(widget, state, colour) \
        gdis_widget_set_background_colour(GTK_WIDGET(widget), colour)
#define gtk_range_set_update_policy(range, policy) \
        gdis_gtk_range_set_update_policy(GTK_RANGE(range), policy)
#define gtk_color_button_new_with_color(colour) \
        gdis_gtk_color_button_new_with_color(colour)
#define gtk_misc_set_alignment(widget, xalign, yalign) \
        gdis_gtk_misc_set_alignment(GTK_WIDGET(widget), xalign, yalign)
#define gtk_toolbar_append_space(toolbar) \
        gdis_gtk_toolbar_append_space(GTK_TOOLBAR(toolbar))
#define gtk_toolbar_append_item(toolbar, text, tooltip, tooltip_private, icon, callback, user_data) \
        gdis_gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), text, tooltip, tooltip_private, icon, (GCallback) (callback), user_data)
#define gtk_toolbar_append_widget(toolbar, widget, tooltip, tooltip_private) \
        gdis_gtk_toolbar_append_widget(GTK_TOOLBAR(toolbar), widget, tooltip, tooltip_private)
#define gdk_visual_get_best_with_type(type) \
        gdis_gdk_visual_get_best_with_type(type)
#define gdk_colormap_new(visual, allocate) \
        gdis_gdk_colormap_new(visual, allocate)
#define gtk_widget_push_colormap(colormap) ((void) (colormap))
#define gtk_widget_push_visual(visual) ((void) (visual))
#define gtk_widget_pop_colormap() ((void) 0)
#define gtk_widget_pop_visual() ((void) 0)
#define gtk_window_set_policy(window, allow_shrink, allow_grow, auto_shrink) \
        gdis_gtk_window_set_policy(window, allow_shrink, allow_grow, auto_shrink)
#define gtk_window_set_position(window, position) ((void) (window))
#define gtk_dialog_run(dialog) \
        gdis_gtk_dialog_run(GTK_DIALOG(dialog))
#define gtk_file_chooser_get_filename(chooser) \
        gdis_gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser))
#define gtk_about_dialog_set_name(dialog, name) \
        gtk_about_dialog_set_program_name(dialog, name)
#define gtk_text_buffer_insert_pixbuf(buffer, iter, pixbuf) \
        gdis_gtk_text_buffer_insert_pixbuf(buffer, iter, pixbuf)
#define gdk_get_default_root_window() \
        gdis_gdk_get_default_root_window()
#define gdk_window_get_geometry(window, x, y, width, height, depth) \
        gdis_gdk_window_get_geometry(window, x, y, width, height, depth)
#define gtk_accel_group_new() \
        gdis_gtk_accel_group_new()
#define gtk_window_add_accel_group(window, accel_group) \
        gdis_gtk_window_add_accel_group(GTK_WINDOW(window), accel_group)
#define gtk_exit(status) exit(status)
#define gdk_flush() gdis_gdk_flush()
#define gdk_beep() gdis_gdk_beep()
#define gdk_threads_init() gdis_gdk_threads_init()
#define gdk_threads_enter() ((void) 0)
#define gdk_threads_leave() ((void) 0)

static GMainLoop *gdis_gtk_main_loop = NULL;

static inline void gdis_gtk_main(void)
{
if (!gdis_gtk_main_loop)
  gdis_gtk_main_loop = g_main_loop_new(NULL, FALSE);

g_main_loop_run(gdis_gtk_main_loop);
}

#define gtk_main() gdis_gtk_main()
#endif

#if GTK_MAJOR_VERSION < 3
static inline void gdis_gtk_combo_box_append_text(GtkComboBox *combo,
                                                  const gchar *text)
{
GtkListStore *store;
GtkTreeIter iter;

store = GTK_LIST_STORE(gtk_combo_box_get_model(combo));
if (!store)
  return;

gtk_list_store_append(store, &iter);
gtk_list_store_set(store, &iter, 0, text, -1);
}

static inline gchar *gdis_gtk_combo_box_get_active_text(GtkComboBox *combo)
{
GtkTreeIter iter;
GtkTreeModel *model;
gchar *text;

model = gtk_combo_box_get_model(combo);
if (!model)
  return(NULL);

if (!gtk_combo_box_get_active_iter(combo, &iter))
  return(NULL);

text = NULL;
gtk_tree_model_get(model, &iter, 0, &text, -1);

return(text);
}
#endif

#if GTK_MAJOR_VERSION >= 3 && GTK_MAJOR_VERSION < 4
typedef gpointer GtkTooltips;
typedef gpointer GdkColormap;
typedef gpointer GdkDrawable;
typedef gpointer GdkFont;
typedef gpointer GdkGC;
typedef gpointer GdkPixmap;

#ifndef GTK_UPDATE_CONTINUOUS
#define GTK_UPDATE_CONTINUOUS 0
#endif
#ifndef GTK_UPDATE_DISCONTINUOUS
#define GTK_UPDATE_DISCONTINUOUS 1
#endif
#ifndef GTK_UPDATE_DELAYED
#define GTK_UPDATE_DELAYED 2
#endif

#define gtk_entry_set_editable(entry, editable) \
        gtk_editable_set_editable(GTK_EDITABLE(entry), editable)
#define gtk_about_dialog_set_name(dialog, name) \
        gtk_about_dialog_set_program_name(dialog, name)
#define gtk_container_border_width(container, border_width) \
        gtk_container_set_border_width(GTK_CONTAINER(container), border_width)
#define gtk_radio_button_group(button) \
        gtk_radio_button_get_group(GTK_RADIO_BUTTON(button))
#define gtk_range_set_update_policy(range, policy) \
        ((void) (range), (void) (policy))
#define gtk_tooltips_new() \
        GINT_TO_POINTER(1)
#define gtk_tooltips_set_delay(tooltips, delay) \
        ((void) (tooltips), (void) (delay))
#define gtk_tooltips_set_tip(tooltips, widget, text, private_text) \
        do { \
          (void) (tooltips); \
          (void) (private_text); \
          if (widget) \
            gtk_widget_set_tooltip_text(GTK_WIDGET(widget), text); \
        } while (0)

static inline GtkWidget *gdis_gtk_entry_new_with_max_length(gint max)
{
GtkWidget *entry;

entry = gtk_entry_new();
gtk_entry_set_max_length(GTK_ENTRY(entry), max);

return(entry);
}

#define gtk_entry_new_with_max_length(max) \
        gdis_gtk_entry_new_with_max_length(max)

typedef struct
{
const gchar *path;
const gchar *accelerator;
gpointer callback;
guint callback_action;
const gchar *item_type;
} GtkItemFactoryEntry;

typedef struct
{
GtkWidget *menu_bar;
GtkAccelGroup *accel_group;
GHashTable *submenus;
} GtkItemFactory;

#define gtk_hbox_new(homogeneous, spacing) \
        gdis_gtk_box_new(GTK_ORIENTATION_HORIZONTAL, homogeneous, spacing)
#define gtk_vbox_new(homogeneous, spacing) \
        gdis_gtk_box_new(GTK_ORIENTATION_VERTICAL, homogeneous, spacing)
#define gtk_combo_box_new_text() gtk_combo_box_text_new()
#define gtk_combo_box_append_text(combo, text) \
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), text)
#define gtk_combo_box_get_active_text(combo) \
        gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(combo))

static inline void gdis_gtk_scrolled_window_add_with_viewport(GtkScrolledWindow *swin,
                                                              GtkWidget *child)
{
gtk_container_add(GTK_CONTAINER(swin), child);
}

#define gtk_scrolled_window_add_with_viewport gdis_gtk_scrolled_window_add_with_viewport

static inline const gchar *gdis_menu_item_leaf(const gchar *path)
{
const gchar *leaf;

leaf = strrchr(path, '/');
if (leaf)
  return(leaf+1);

return(path);
}

static inline gchar *gdis_menu_item_parent(const gchar *path)
{
const gchar *leaf;

leaf = strrchr(path, '/');
if (!leaf || leaf == path)
  return(g_strdup(""));

return(g_strndup(path, leaf-path));
}

static inline GtkWidget *gdis_menu_item_parent_shell(GtkItemFactory *factory,
                                                     const gchar *path)
{
GtkWidget *shell;
gchar *parent;

parent = gdis_menu_item_parent(path);
if (!parent || *parent == '\0')
  {
  g_free(parent);
  return(factory->menu_bar);
  }

shell = g_hash_table_lookup(factory->submenus, parent);
g_free(parent);

if (!shell)
  return(factory->menu_bar);

return(shell);
}

static inline GtkItemFactory *gtk_item_factory_new(GType type,
                                                   const gchar *path,
                                                   GtkAccelGroup *accel_group)
{
GtkItemFactory *factory;

(void) type;
(void) path;

factory = g_new0(GtkItemFactory, 1);
factory->menu_bar = gtk_menu_bar_new();
factory->accel_group = accel_group;
factory->submenus = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

return(factory);
}

static inline void gtk_item_factory_create_items(GtkItemFactory *factory,
                                                 guint n_entries,
                                                 GtkItemFactoryEntry *entries,
                                                 gpointer callback_data)
{
guint i;

(void) callback_data;

for (i=0 ; i<n_entries ; i++)
  {
  GtkWidget *item;
  GtkWidget *shell;
  const gchar *label;
  const gchar *accel;

  shell = gdis_menu_item_parent_shell(factory, entries[i].path);
  label = gdis_menu_item_leaf(entries[i].path);
  accel = entries[i].accelerator;

  if (entries[i].item_type && g_ascii_strcasecmp(entries[i].item_type, "<Branch>") == 0)
    {
    GtkWidget *submenu;

    item = gtk_menu_item_new_with_mnemonic(label);
    submenu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);
    gtk_menu_shell_append(GTK_MENU_SHELL(shell), item);
    g_hash_table_replace(factory->submenus, g_strdup(entries[i].path), submenu);
    continue;
    }

  if (entries[i].item_type && g_ascii_strcasecmp(entries[i].item_type, "<Separator>") == 0)
    {
    item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(shell), item);
    continue;
    }

  item = gtk_menu_item_new_with_mnemonic(label);
  gtk_menu_shell_append(GTK_MENU_SHELL(shell), item);

  if (entries[i].callback)
    g_signal_connect(item, "activate", G_CALLBACK(entries[i].callback),
                     entries[i].callback_action ? GINT_TO_POINTER(entries[i].callback_action) : NULL);

  if (factory->accel_group && accel)
    {
    guint key = 0;
    GdkModifierType modifiers = 0;

    gtk_accelerator_parse(accel, &key, &modifiers);
    if (key)
      gtk_widget_add_accelerator(item, "activate", factory->accel_group,
                                 key, modifiers, GTK_ACCEL_VISIBLE);
    }
  }
}

static inline GtkWidget *gtk_item_factory_get_widget(GtkItemFactory *factory,
                                                     const gchar *path)
{
(void) path;

return(factory->menu_bar);
}

static inline void gdis_gtk_toolbar_append_space(GtkToolbar *toolbar)
{
GtkToolItem *item;

item = gtk_separator_tool_item_new();
gtk_toolbar_insert(toolbar, item, -1);
}

static inline GtkToolItem *gdis_gtk_toolbar_append_item(GtkToolbar *toolbar,
                                                        const gchar *text,
                                                        const gchar *tooltip,
                                                        const gchar *tooltip_private,
                                                        GtkWidget *icon,
                                                        GCallback callback,
                                                        gpointer user_data)
{
GtkToolItem *item;

(void) tooltip_private;

item = gtk_tool_button_new(icon, text);
if (tooltip)
  gtk_widget_set_tooltip_text(GTK_WIDGET(item), tooltip);
if (callback)
  g_signal_connect(item, "clicked", callback, user_data);
gtk_toolbar_insert(toolbar, item, -1);

return(item);
}

static inline GtkToolItem *gdis_gtk_toolbar_append_widget(GtkToolbar *toolbar,
                                                          GtkWidget *widget,
                                                          const gchar *tooltip,
                                                          const gchar *tooltip_private)
{
GtkToolItem *item;

(void) tooltip_private;

item = gtk_tool_item_new();
gtk_container_add(GTK_CONTAINER(item), widget);
if (tooltip)
  gtk_widget_set_tooltip_text(GTK_WIDGET(item), tooltip);
gtk_toolbar_insert(toolbar, item, -1);

return(item);
}

static inline GdkVisual *gdis_gdk_visual_get_best_with_type(GdkVisualType type)
{
(void) type;

return(gdk_screen_get_system_visual(gdk_screen_get_default()));
}

static inline GdkColormap *gdis_gdk_colormap_new(GdkVisual *visual, gboolean allocate)
{
(void) visual;
(void) allocate;

return(visual ? (GdkColormap *) visual : GINT_TO_POINTER(1));
}

static inline gint gdis_gdk_visual_get_depth(GdkVisual *visual)
{
return(gdk_visual_get_depth(visual));
}

static inline void gdis_gtk_window_set_policy(GtkWindow *window,
                                              gboolean allow_shrink,
                                              gboolean allow_grow,
                                              gboolean auto_shrink)
{
(void) allow_shrink;
(void) auto_shrink;

gtk_window_set_resizable(window, allow_grow);
}

static inline void gdis_gtk_misc_set_alignment(GtkWidget *widget,
                                               gfloat xalign,
                                               gfloat yalign)
{
GtkAlign halign = GTK_ALIGN_FILL;
GtkAlign valign = GTK_ALIGN_FILL;

if (xalign <= 0.0)
  halign = GTK_ALIGN_START;
else if (xalign >= 1.0)
  halign = GTK_ALIGN_END;
else
  halign = GTK_ALIGN_CENTER;

if (yalign <= 0.0)
  valign = GTK_ALIGN_START;
else if (yalign >= 1.0)
  valign = GTK_ALIGN_END;
else
  valign = GTK_ALIGN_CENTER;

gtk_widget_set_halign(widget, halign);
gtk_widget_set_valign(widget, valign);
if (GTK_IS_LABEL(widget))
  {
  gtk_label_set_xalign(GTK_LABEL(widget), xalign);
  gtk_label_set_yalign(GTK_LABEL(widget), yalign);
  }
}

#define gtk_misc_set_alignment(widget, xalign, yalign) \
        gdis_gtk_misc_set_alignment(GTK_WIDGET(widget), xalign, yalign)
#define gtk_toolbar_append_space(toolbar) \
        gdis_gtk_toolbar_append_space(GTK_TOOLBAR(toolbar))
#define gtk_toolbar_append_item(toolbar, text, tooltip, tooltip_private, icon, callback, user_data) \
        gdis_gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), text, tooltip, tooltip_private, icon, (GCallback) (callback), user_data)
#define gtk_toolbar_append_widget(toolbar, widget, tooltip, tooltip_private) \
        gdis_gtk_toolbar_append_widget(GTK_TOOLBAR(toolbar), widget, tooltip, tooltip_private)
#define gdk_visual_get_best_with_type(type) \
        gdis_gdk_visual_get_best_with_type(type)
#define gdk_colormap_new(visual, allocate) \
        gdis_gdk_colormap_new(visual, allocate)
#define gtk_widget_push_colormap(colormap) ((void) (colormap))
#define gtk_widget_push_visual(visual) ((void) (visual))
#define gtk_widget_pop_colormap() ((void) 0)
#define gtk_widget_pop_visual() ((void) 0)
#define gtk_window_set_policy(window, allow_shrink, allow_grow, auto_shrink) \
        gdis_gtk_window_set_policy(window, allow_shrink, allow_grow, auto_shrink)
#define gtk_exit(status) exit(status)
#define gdk_threads_enter() ((void) 0)
#define gdk_threads_leave() ((void) 0)
#elif GTK_MAJOR_VERSION < 3
#define gtk_combo_box_append_text(combo, text) \
        gdis_gtk_combo_box_append_text(GTK_COMBO_BOX(combo), text)
#define gtk_combo_box_get_active_text(combo) \
        gdis_gtk_combo_box_get_active_text(GTK_COMBO_BOX(combo))

static inline void gdis_gtk_label_set_xalign(GtkLabel *label, gfloat xalign)
{
gtk_misc_set_alignment(GTK_MISC(label), xalign, 0.5);
}

#define gtk_label_set_xalign(label, xalign) \
        gdis_gtk_label_set_xalign(GTK_LABEL(label), xalign)

static inline gint gdis_gdk_visual_get_depth(GdkVisual *visual)
{
if (!visual)
  return(0);

return(visual->depth);
}
#endif

static inline void gdis_gtk_widget_get_allocation_compat(GtkWidget *widget,
                                                         GtkAllocation *allocation)
{
#if GTK_MAJOR_VERSION >= 3
gtk_widget_get_allocation(widget, allocation);
#else
*allocation = widget->allocation;
#endif
}

static inline gint gdis_gtk_widget_get_x(GtkWidget *widget)
{
GtkAllocation allocation;

gdis_gtk_widget_get_allocation_compat(widget, &allocation);
return(allocation.x);
}

static inline gint gdis_gtk_widget_get_y(GtkWidget *widget)
{
GtkAllocation allocation;

gdis_gtk_widget_get_allocation_compat(widget, &allocation);
return(allocation.y);
}

static inline gint gdis_gtk_widget_get_width(GtkWidget *widget)
{
GtkAllocation allocation;

gdis_gtk_widget_get_allocation_compat(widget, &allocation);
return(allocation.width);
}

static inline gint gdis_gtk_widget_get_height(GtkWidget *widget)
{
GtkAllocation allocation;

gdis_gtk_widget_get_allocation_compat(widget, &allocation);
return(allocation.height);
}

static inline gint gdis_gtk_widget_font_size(GtkWidget *widget)
{
#if GTK_MAJOR_VERSION >= 3
PangoContext *context;
const PangoFontDescription *desc;

context = gtk_widget_get_pango_context(widget);
if (!context)
  return(12);

desc = pango_context_get_font_description(context);
if (!desc)
  return(12);

return(pango_font_description_get_size(desc) / PANGO_SCALE);
#else
GtkStyle *style;

style = gtk_widget_get_style(widget);
if (!style || !style->font_desc)
  return(12);

return(pango_font_description_get_size(style->font_desc) / PANGO_SCALE);
#endif
}

static inline GdkPixbuf *gdis_gdk_pixbuf_get_from_window(GdkWindow *window,
                                                         gint src_x,
                                                         gint src_y,
                                                         gint width,
                                                         gint height)
{
#if GTK_MAJOR_VERSION >= 4
(void) window;
(void) src_x;
(void) src_y;
(void) width;
(void) height;
return(NULL);
#elif GTK_MAJOR_VERSION >= 3
return(gdk_pixbuf_get_from_window(window, src_x, src_y, width, height));
#else
return(gdk_pixbuf_get_from_drawable(NULL, window, NULL,
                                    src_x, src_y, 0, 0, width, height));
#endif
}

#if GTK_MAJOR_VERSION < 4
static inline gboolean gdis_gdk_event_get_position_compat(GdkEvent *event,
                                                          gdouble *x,
                                                          gdouble *y)
{
if (!event)
  return(FALSE);

switch (event->type)
  {
  case GDK_BUTTON_PRESS:
  case GDK_2BUTTON_PRESS:
  case GDK_3BUTTON_PRESS:
  case GDK_BUTTON_RELEASE:
    if (x)
      *x = event->button.x;
    if (y)
      *y = event->button.y;
    return(TRUE);

  case GDK_MOTION_NOTIFY:
    if (x)
      *x = event->motion.x;
    if (y)
      *y = event->motion.y;
    return(TRUE);

  case GDK_SCROLL:
    if (x)
      *x = event->scroll.x;
    if (y)
      *y = event->scroll.y;
    return(TRUE);

  case GDK_ENTER_NOTIFY:
  case GDK_LEAVE_NOTIFY:
    if (x)
      *x = event->crossing.x;
    if (y)
      *y = event->crossing.y;
    return(TRUE);

  default:
    break;
  }

return(FALSE);
}

static inline GdkModifierType gdis_gdk_event_get_modifier_state_compat(GdkEvent *event)
{
if (!event)
  return(0);

switch (event->type)
  {
  case GDK_BUTTON_PRESS:
  case GDK_2BUTTON_PRESS:
  case GDK_3BUTTON_PRESS:
  case GDK_BUTTON_RELEASE:
    return(event->button.state);

  case GDK_MOTION_NOTIFY:
    return(event->motion.state);

  case GDK_SCROLL:
    return(event->scroll.state);

  case GDK_KEY_PRESS:
  case GDK_KEY_RELEASE:
    return(event->key.state);

  case GDK_ENTER_NOTIFY:
  case GDK_LEAVE_NOTIFY:
    return(event->crossing.state);

  default:
    break;
  }

return(0);
}

static inline guint gdis_gdk_button_event_get_button_compat(GdkEvent *event)
{
if (!event)
  return(0);

switch (event->type)
  {
  case GDK_BUTTON_PRESS:
  case GDK_2BUTTON_PRESS:
  case GDK_3BUTTON_PRESS:
  case GDK_BUTTON_RELEASE:
    return(event->button.button);

  default:
    break;
  }

return(0);
}

static inline GdkScrollDirection gdis_gdk_scroll_event_get_direction_compat(GdkEvent *event)
{
if (event && event->type == GDK_SCROLL)
  return(event->scroll.direction);

return(GDK_SCROLL_LEFT);
}

static inline guint gdis_gdk_key_event_get_keyval_compat(GdkEvent *event)
{
if (!event)
  return(0);

switch (event->type)
  {
  case GDK_KEY_PRESS:
  case GDK_KEY_RELEASE:
    return(event->key.keyval);

  default:
    break;
  }

return(0);
}

#define gdk_event_get_position(event, x, y) \
        gdis_gdk_event_get_position_compat((GdkEvent *) (event), x, y)
#define gdk_event_get_modifier_state(event) \
        gdis_gdk_event_get_modifier_state_compat((GdkEvent *) (event))
#define gdk_button_event_get_button(event) \
        gdis_gdk_button_event_get_button_compat((GdkEvent *) (event))
#define gdk_scroll_event_get_direction(event) \
        gdis_gdk_scroll_event_get_direction_compat((GdkEvent *) (event))
#define gdk_key_event_get_keyval(event) \
        gdis_gdk_key_event_get_keyval_compat((GdkEvent *) (event))
#endif

static inline void gdis_gdk_draw_pixbuf(GdkWindow *window,
                                        GdkPixbuf *pixbuf,
                                        gint src_x,
                                        gint src_y,
                                        gint dest_x,
                                        gint dest_y,
                                        gint width,
                                        gint height)
{
#if GTK_MAJOR_VERSION >= 4
(void) window;
(void) pixbuf;
(void) src_x;
(void) src_y;
(void) dest_x;
(void) dest_y;
(void) width;
(void) height;
#elif GTK_MAJOR_VERSION >= 3
cairo_t *cr;

cr = gdk_cairo_create(window);
gdk_cairo_set_source_pixbuf(cr, pixbuf, dest_x - src_x, dest_y - src_y);
cairo_rectangle(cr, dest_x, dest_y, width, height);
cairo_fill(cr);
cairo_destroy(cr);
#else
gdk_draw_pixbuf(window, NULL, pixbuf,
                src_x, src_y, dest_x, dest_y, width, height,
                GDK_RGB_DITHER_NONE, 0, 0);
#endif
}

static inline GtkWidget *gdis_colour_dialog_new(const gchar *title)
{
#if GTK_MAJOR_VERSION >= 3
GtkWidget *dialog;

dialog = gtk_color_chooser_dialog_new(title, NULL);
g_object_set_data(G_OBJECT(dialog), "gdis-colour-chooser", dialog);
g_object_set_data(G_OBJECT(dialog), "gdis-colour-ok",
                  gtk_dialog_get_widget_for_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK));
g_object_set_data(G_OBJECT(dialog), "gdis-colour-cancel",
                  gtk_dialog_get_widget_for_response(GTK_DIALOG(dialog), GTK_RESPONSE_CANCEL));

return(dialog);
#else
return(gtk_color_selection_dialog_new(title));
#endif
}

static inline GtkWidget *gdis_colour_dialog_get_chooser(GtkWidget *dialog)
{
#if GTK_MAJOR_VERSION >= 3
return(g_object_get_data(G_OBJECT(dialog), "gdis-colour-chooser"));
#else
return(GTK_COLOR_SELECTION_DIALOG(dialog)->colorsel);
#endif
}

static inline GtkWidget *gdis_colour_dialog_get_ok_button(GtkWidget *dialog)
{
#if GTK_MAJOR_VERSION >= 3
return(g_object_get_data(G_OBJECT(dialog), "gdis-colour-ok"));
#else
return(GTK_COLOR_SELECTION_DIALOG(dialog)->ok_button);
#endif
}

static inline GtkWidget *gdis_colour_dialog_get_cancel_button(GtkWidget *dialog)
{
#if GTK_MAJOR_VERSION >= 3
return(g_object_get_data(G_OBJECT(dialog), "gdis-colour-cancel"));
#else
return(GTK_COLOR_SELECTION_DIALOG(dialog)->cancel_button);
#endif
}

static inline void gdis_colour_chooser_get_current_color(GtkWidget *chooser,
                                                         GdkColor *colour)
{
#if GTK_MAJOR_VERSION >= 4
GdkRGBA rgba;

gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(chooser), &rgba);
colour->red = rgba.red * COLOUR_SCALE;
colour->green = rgba.green * COLOUR_SCALE;
colour->blue = rgba.blue * COLOUR_SCALE;
#elif GTK_MAJOR_VERSION >= 3
GdkRGBA rgba;

gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(chooser), &rgba);
colour->red = rgba.red * COLOUR_SCALE;
colour->green = rgba.green * COLOUR_SCALE;
colour->blue = rgba.blue * COLOUR_SCALE;
#else
gtk_color_selection_get_current_color(GTK_COLOR_SELECTION(chooser), colour);
#endif
}

static inline void gdis_colour_chooser_set_rgb(GtkWidget *chooser,
                                               const gdouble *rgb)
{
#if GTK_MAJOR_VERSION >= 4
GdkRGBA rgba;

rgba.red = rgb[0];
rgba.green = rgb[1];
rgba.blue = rgb[2];
rgba.alpha = 1.0;
gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(chooser), &rgba);
#elif GTK_MAJOR_VERSION >= 3
GdkRGBA rgba;

rgba.red = rgb[0];
rgba.green = rgb[1];
rgba.blue = rgb[2];
rgba.alpha = 1.0;
gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(chooser), &rgba);
#else
GdkColor colour;

colour.red = rgb[0] * COLOUR_SCALE;
colour.green = rgb[1] * COLOUR_SCALE;
colour.blue = rgb[2] * COLOUR_SCALE;
gtk_color_selection_set_current_color(GTK_COLOR_SELECTION(chooser), &colour);
#endif
}

static inline void gdis_widget_set_background_colour(GtkWidget *widget,
                                                     const GdkColor *colour)
{
#if GTK_MAJOR_VERSION >= 4
(void) widget;
(void) colour;
#elif GTK_MAJOR_VERSION >= 3
GdkRGBA rgba;

rgba.red = colour->red * INV_COLOUR_SCALE;
rgba.green = colour->green * INV_COLOUR_SCALE;
rgba.blue = colour->blue * INV_COLOUR_SCALE;
rgba.alpha = 1.0;
gtk_widget_override_background_color(widget, GTK_STATE_FLAG_NORMAL, &rgba);
#else
gtk_widget_modify_bg(widget, GTK_STATE_NORMAL, colour);
#endif
}

static inline gulong gdis_colour_chooser_connect_changed(GtkWidget *chooser,
                                                         GCallback callback,
                                                         gpointer user_data)
{
#if GTK_MAJOR_VERSION >= 3
return(g_signal_connect(chooser, "notify::rgba", callback, user_data));
#else
return(g_signal_connect(chooser, "color_changed", callback, user_data));
#endif
}

static inline GtkWidget *gdis_font_dialog_new(const gchar *title)
{
#if GTK_MAJOR_VERSION >= 3
GtkWidget *dialog;

dialog = gtk_font_chooser_dialog_new(title, NULL);
g_object_set_data(G_OBJECT(dialog), "gdis-font-ok",
                  gtk_dialog_get_widget_for_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK));
g_object_set_data(G_OBJECT(dialog), "gdis-font-cancel",
                  gtk_dialog_get_widget_for_response(GTK_DIALOG(dialog), GTK_RESPONSE_CANCEL));

return(dialog);
#else
return(gtk_font_selection_dialog_new(title));
#endif
}

static inline gchar *gdis_font_dialog_get_font_name(GtkWidget *dialog)
{
#if GTK_MAJOR_VERSION >= 3
return(gtk_font_chooser_get_font(GTK_FONT_CHOOSER(dialog)));
#else
return(gtk_font_selection_dialog_get_font_name(GTK_FONT_SELECTION_DIALOG(dialog)));
#endif
}

static inline GtkWidget *gdis_font_dialog_get_ok_button(GtkWidget *dialog)
{
#if GTK_MAJOR_VERSION >= 3
return(g_object_get_data(G_OBJECT(dialog), "gdis-font-ok"));
#else
return(GTK_FONT_SELECTION_DIALOG(dialog)->ok_button);
#endif
}

static inline GtkWidget *gdis_font_dialog_get_cancel_button(GtkWidget *dialog)
{
#if GTK_MAJOR_VERSION >= 3
return(g_object_get_data(G_OBJECT(dialog), "gdis-font-cancel"));
#else
return(GTK_FONT_SELECTION_DIALOG(dialog)->cancel_button);
#endif
}

#endif
