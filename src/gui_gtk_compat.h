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

#if GTK_MAJOR_VERSION >= 3
typedef gpointer GdkColormap;

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
#else
#define gtk_combo_box_append_text(combo, text) \
        gdis_gtk_combo_box_append_text(GTK_COMBO_BOX(combo), text)
#define gtk_combo_box_get_active_text(combo) \
        gdis_gtk_combo_box_get_active_text(GTK_COMBO_BOX(combo))

static inline gint gdis_gdk_visual_get_depth(GdkVisual *visual)
{
if (!visual)
  return(0);

return(visual->depth);
}
#endif

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
#if GTK_MAJOR_VERSION >= 3
return(gdk_pixbuf_get_from_window(window, src_x, src_y, width, height));
#else
return(gdk_pixbuf_get_from_drawable(NULL, window, NULL,
                                    src_x, src_y, 0, 0, width, height));
#endif
}

static inline void gdis_gdk_draw_pixbuf(GdkWindow *window,
                                        GdkPixbuf *pixbuf,
                                        gint src_x,
                                        gint src_y,
                                        gint dest_x,
                                        gint dest_y,
                                        gint width,
                                        gint height)
{
#if GTK_MAJOR_VERSION >= 3
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
#if GTK_MAJOR_VERSION >= 3
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
#if GTK_MAJOR_VERSION >= 3
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
#if GTK_MAJOR_VERSION >= 3
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
