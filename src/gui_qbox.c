/*
Copyright (C) 2026

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

The GNU GPL can also be found at http://www.gnu.org
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <glib/gstdio.h>

#include "gdis.h"
#include "dialog.h"
#include "file.h"
#include "gui_shorts.h"
#include "interface.h"
#include "parse.h"
#include "task.h"

extern struct sysenv_pak sysenv;
extern struct elem_pak elements[];

struct qbox_species_pak
{
  gchar *symbol;
  gchar *path;
};

struct qbox_gui_pak
{
  struct model_pak *model;
  gchar *workdir;
  gchar *input_name;
  gchar *xml_name;
  gchar *log_name;
  gchar *xc;
  gchar *wf_dyn;
  gdouble ecut;
  gdouble ecutprec;
  gint randomize_wf;
  gint load_saved_xml;
  gint write_model_block;
  gint write_default_block;
  gint auto_save_xml_cmd;
  gint auto_quit;
  gchar *run_cmd;
  gchar *include_cmd_file;
  gchar *pre_commands;
  gchar *post_commands;
  GtkTextBuffer *pre_buffer;
  GtkTextBuffer *post_buffer;
  gulong pre_buffer_handler;
  gulong post_buffer_handler;
  GtkWidget *entry_xc;
  GtkWidget *entry_wf_dyn;
  GtkWidget *entry_run_cmd;
  GtkWidget *entry_include_cmd_file;
  GtkWidget *check_randomize_wf;
  GtkWidget *check_load_saved_xml;
  GtkWidget *check_write_model_block;
  GtkWidget *check_write_default_block;
  GtkWidget *check_auto_save_xml_cmd;
  GtkWidget *check_auto_quit;
  GSList *species;
};

struct qbox_task_pak
{
  struct model_pak *model;
  gchar *qbox_path;
  gchar *workdir;
  gchar *input_name;
  gchar *xml_name;
  gchar *log_name;
  gchar *xc;
  gchar *wf_dyn;
  gchar *input_path;
  gchar *xml_path;
  gchar *log_path;
  gdouble ecut;
  gdouble ecutprec;
  gint randomize_wf;
  gint load_saved_xml;
  gint write_model_block;
  gint write_default_block;
  gint auto_save_xml_cmd;
  gint auto_quit;
  gchar *run_cmd;
  gchar *include_cmd_file;
  gchar *pre_commands;
  gchar *post_commands;
  GSList *species;
  gchar *message;
  gchar *error;
};

static struct qbox_species_pak *qbox_species_new(const gchar *symbol, const gchar *path)
{
  struct qbox_species_pak *species;

  g_return_val_if_fail(symbol != NULL, NULL);

  species = g_malloc0(sizeof(struct qbox_species_pak));
  species->symbol = g_strdup(symbol);
  species->path = path ? g_strdup(path) : NULL;

  return(species);
}

static void qbox_species_free(gpointer data)
{
  struct qbox_species_pak *species = data;

  if (!species)
    return;

  g_free(species->symbol);
  g_free(species->path);
  g_free(species);
}

static GSList *qbox_species_copy_list(GSList *list)
{
  GSList *copy = NULL;
  GSList *item;

  for (item=list ; item ; item=g_slist_next(item))
    {
    struct qbox_species_pak *species = item->data;

    copy = g_slist_append(copy, qbox_species_new(species->symbol, species->path));
    }

  return(copy);
}

static struct qbox_species_pak *qbox_species_lookup(GSList *list, const gchar *symbol)
{
  GSList *item;

  g_return_val_if_fail(symbol != NULL, NULL);

  for (item=list ; item ; item=g_slist_next(item))
    {
    struct qbox_species_pak *species = item->data;

    if (g_ascii_strcasecmp(species->symbol, symbol) == 0)
      return(species);
    }

  return(NULL);
}

static void qbox_string_replace(gchar **target, const gchar *value)
{
  g_return_if_fail(target != NULL);

  g_free(*target);
  if (value && strlen(value))
    *target = g_strdup(value);
  else
    *target = NULL;

  gui_relation_update_widget(target);
}

static gchar *qbox_model_stem(struct model_pak *model)
{
  if (model && model->basename && strlen(model->basename))
    return(g_strdup(model->basename));

  return(g_strdup("model"));
}

static gchar *qbox_default_workdir(void)
{
  return(g_build_filename(sysenv.cwd, "tmp", "qbox-gui", NULL));
}

static gchar *qbox_demo_potential_path(const gchar *symbol)
{
  const gchar *filename = NULL;
  gchar *path;

  if (!symbol)
    return(NULL);

  if (g_ascii_strcasecmp(symbol, "C") == 0)
    filename = "C_HSCV_PBE-1.0.xml";
  else if (g_ascii_strcasecmp(symbol, "H") == 0)
    filename = "H_HSCV_PBE-1.0.xml";
  else if (g_ascii_strcasecmp(symbol, "O") == 0)
    filename = "O_HSCV_PBE-1.0.xml";
  else if (g_ascii_strcasecmp(symbol, "Si") == 0)
    filename = "Si_VBC_LDA-1.0.xml";

  if (!filename)
    return(NULL);

  path = g_build_filename(sysenv.cwd, ".localdeps", "qbox-public",
                          "test", "potentials", filename, NULL);
  if (!g_file_test(path, G_FILE_TEST_EXISTS))
    {
    g_free(path);
    return(NULL);
    }

  return(path);
}

static GSList *qbox_symbol_list_add_unique(GSList *list, const gchar *symbol)
{
  GSList *item;

  g_return_val_if_fail(symbol != NULL, list);

  for (item=list ; item ; item=g_slist_next(item))
    {
    if (g_ascii_strcasecmp(item->data, symbol) == 0)
      return(list);
    }

  return(g_slist_append(list, g_strdup(symbol)));
}

static gchar *qbox_symbol_list_string(GSList *list)
{
  GString *text;
  GSList *item;

  text = g_string_new(NULL);
  for (item=list ; item ; item=g_slist_next(item))
    {
    if (item != list)
      g_string_append(text, ", ");
    g_string_append(text, item->data);
    }

  return(g_string_free(text, FALSE));
}

static void qbox_symbol_list_free(GSList *list)
{
  g_slist_free_full(list, g_free);
}

static GSList *qbox_collect_species(struct model_pak *model)
{
  GSList *species = NULL;
  GSList *labels;
  GSList *item;

  g_return_val_if_fail(model != NULL, NULL);

  labels = find_unique(ELEMENT, model);
  for (item=labels ; item ; item=g_slist_next(item))
    {
    gint code;
    const gchar *symbol;
    gchar *demo_path;

    code = GPOINTER_TO_INT(item->data);
    if (code < 0 || code >= MAX_ELEMENTS)
      continue;

    symbol = elements[code].symbol;
    if (!symbol || !strlen(symbol))
      continue;

    demo_path = qbox_demo_potential_path(symbol);
    species = g_slist_append(species, qbox_species_new(symbol, demo_path));
    g_free(demo_path);
    }

  g_slist_free(labels);

  return(species);
}

static void qbox_gui_state_free(struct qbox_gui_pak *state)
{
  if (!state)
    return;

  g_free(state->workdir);
  g_free(state->input_name);
  g_free(state->xml_name);
  g_free(state->log_name);
  g_free(state->xc);
  g_free(state->wf_dyn);
  g_free(state->run_cmd);
  g_free(state->include_cmd_file);
  g_free(state->pre_commands);
  g_free(state->post_commands);
  g_slist_free_full(state->species, qbox_species_free);
  g_free(state);
}

static void qbox_gui_state_free_cb(GtkWidget *w, gpointer data)
{
  (void) w;

  qbox_gui_state_free(data);
}

static void qbox_task_free(struct qbox_task_pak *job)
{
  if (!job)
    return;

  g_free(job->qbox_path);
  g_free(job->workdir);
  g_free(job->input_name);
  g_free(job->xml_name);
  g_free(job->log_name);
  g_free(job->xc);
  g_free(job->wf_dyn);
  g_free(job->run_cmd);
  g_free(job->include_cmd_file);
  g_free(job->pre_commands);
  g_free(job->post_commands);
  g_free(job->input_path);
  g_free(job->xml_path);
  g_free(job->log_path);
  g_slist_free_full(job->species, qbox_species_free);
  g_free(job->message);
  g_free(job->error);
  g_free(job);
}

static void qbox_write_line(FILE *dest, const gchar *text)
{
  gchar *tmp;

  g_return_if_fail(dest != NULL);

  if (!text)
    return;

  tmp = g_strdup(text);
  g_strstrip(tmp);
  if (strlen(tmp))
    fprintf(dest, "%s\n", tmp);
  g_free(tmp);
}

static void qbox_write_text_block(FILE *dest, const gchar *text)
{
  gchar **line;
  gchar **lines;

  g_return_if_fail(dest != NULL);

  if (!text || !strlen(text))
    return;

  lines = g_strsplit(text, "\n", -1);
  for (line=lines ; line && *line ; line++)
    {
    gchar *tmp;

    tmp = g_strdup(*line);
    g_strstrip(tmp);
    if (!strlen(tmp))
      {
      g_free(tmp);
      continue;
      }
    fprintf(dest, "%s\n", tmp);
    g_free(tmp);
    }
  g_strfreev(lines);
}

static gboolean qbox_line_has_command(const gchar *line, const gchar *cmd)
{
  gchar *tmp;
  gint len;
  gboolean match = FALSE;

  g_return_val_if_fail(cmd != NULL, FALSE);

  if (!line)
    return(FALSE);

  tmp = g_strdup(line);
  g_strstrip(tmp);

  if (!strlen(tmp) || tmp[0] == '#')
    {
    g_free(tmp);
    return(FALSE);
    }

  len = strlen(cmd);
  if (g_ascii_strncasecmp(tmp, cmd, len) == 0)
    {
    gchar tail = tmp[len];
    if (tail == '\0' || g_ascii_isspace(tail))
      match = TRUE;
    }

  g_free(tmp);
  return(match);
}

static gboolean qbox_text_has_command(const gchar *text, const gchar *cmd)
{
  gchar **line;
  gchar **lines;

  g_return_val_if_fail(cmd != NULL, FALSE);

  if (!text || !strlen(text))
    return(FALSE);

  lines = g_strsplit(text, "\n", -1);
  for (line=lines ; line && *line ; line++)
    {
    if (qbox_line_has_command(*line, cmd))
      {
      g_strfreev(lines);
      return(TRUE);
      }
    }
  g_strfreev(lines);

  return(FALSE);
}

enum
{
  QBOX_PRESET_SCF = 0,
  QBOX_PRESET_BAND,
  QBOX_PRESET_GEOOPT,
  QBOX_PRESET_FROZEN_PHONON,
  QBOX_PRESET_HOMO_LUMO
};

static void qbox_text_buffer_changed(GtkTextBuffer *buffer, gpointer data)
{
  gchar **target = data;
  GtkTextIter start, end;

  g_return_if_fail(buffer != NULL);
  g_return_if_fail(target != NULL);

  gtk_text_buffer_get_bounds(buffer, &start, &end);
  g_free(*target);
  *target = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
}

static GtkWidget *qbox_text_window_new(gchar **text, GtkTextBuffer **buffer_out,
                                       gulong *handler_out)
{
  GtkWidget *swin;
  GtkWidget *view;
  GtkTextBuffer *buffer;

  g_return_val_if_fail(text != NULL, NULL);
  g_return_val_if_fail(buffer_out != NULL, NULL);
  g_return_val_if_fail(handler_out != NULL, NULL);

  swin = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  view = gtk_text_view_new();
  gtk_text_view_set_editable(GTK_TEXT_VIEW(view), TRUE);
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(view), GTK_WRAP_WORD_CHAR);
  g_object_set(G_OBJECT(view), "monospace", TRUE, NULL);
  gtk_container_add(GTK_CONTAINER(swin), view);

  buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
  if (*text && strlen(*text))
    gtk_text_buffer_set_text(buffer, *text, -1);

  *handler_out = g_signal_connect(G_OBJECT(buffer), "changed",
                                  GTK_SIGNAL_FUNC(qbox_text_buffer_changed), text);

  *buffer_out = buffer;
  return(swin);
}

static void qbox_state_set_bool(gint *target, gint value, GtkWidget *widget)
{
  g_return_if_fail(target != NULL);

  *target = value;

  if (!widget)
    return;

#if GTK_MAJOR_VERSION >= 4
  if (GTK_IS_CHECK_BUTTON(widget))
    {
    gtk_check_button_set_active(GTK_CHECK_BUTTON(widget), value ? TRUE : FALSE);
    return;
    }
#endif
  if (GTK_IS_TOGGLE_BUTTON(widget))
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), value ? TRUE : FALSE);
}

static void qbox_state_set_string(gchar **target, const gchar *value, GtkWidget *entry)
{
  const gchar *text;

  g_return_if_fail(target != NULL);

  text = (value && strlen(value)) ? value : "";

  g_free(*target);
  if (strlen(text))
    *target = g_strdup(text);
  else
    *target = NULL;

  if (entry && GTK_IS_ENTRY(entry))
    {
    const gchar *current;

    current = gtk_entry_get_text(GTK_ENTRY(entry));
    if (g_strcmp0(current, text) != 0)
      gtk_entry_set_text(GTK_ENTRY(entry), text);
    }
}

static void qbox_state_set_textblock(gchar **target, GtkTextBuffer *buffer,
                                     gulong handler, const gchar *value)
{
  const gchar *text;

  g_return_if_fail(target != NULL);

  text = (value && strlen(value)) ? value : "";

  if (buffer)
    {
    if (handler)
      g_signal_handler_block(buffer, handler);
    gtk_text_buffer_set_text(buffer, text, -1);
    if (handler)
      g_signal_handler_unblock(buffer, handler);
    }

  g_free(*target);
  *target = g_strdup(text);
}

static const gchar *qbox_preset_name(gint preset)
{
  switch (preset)
    {
    case QBOX_PRESET_SCF:
      return("SCF");
    case QBOX_PRESET_BAND:
      return("Band");
    case QBOX_PRESET_GEOOPT:
      return("GeoOpt");
    case QBOX_PRESET_FROZEN_PHONON:
      return("FrozenPhonon");
    case QBOX_PRESET_HOMO_LUMO:
      return("HOMO-LUMO");
    }

  return("Unknown");
}

static void qbox_apply_preset(struct qbox_gui_pak *state, gint preset)
{
  g_return_if_fail(state != NULL);

/*
 * Expert presets should update expert command composition fields only.
 * Keep setup run parameters (XC/WF dyn/default run/randomize/load) unchanged.
 */
  qbox_state_set_bool(&state->write_model_block, TRUE, state->check_write_model_block);
  qbox_state_set_bool(&state->write_default_block, TRUE, state->check_write_default_block);
  qbox_state_set_bool(&state->auto_save_xml_cmd, TRUE, state->check_auto_save_xml_cmd);
  qbox_state_set_bool(&state->auto_quit, TRUE, state->check_auto_quit);
  qbox_state_set_string(&state->include_cmd_file, NULL, state->entry_include_cmd_file);
  qbox_state_set_textblock(&state->pre_commands, state->pre_buffer, state->pre_buffer_handler, NULL);
  qbox_state_set_textblock(&state->post_commands, state->post_buffer, state->post_buffer_handler, NULL);

  switch (preset)
    {
    case QBOX_PRESET_SCF:
      qbox_state_set_textblock(&state->pre_commands, state->pre_buffer, state->pre_buffer_handler,
                               "set scf_tol 1e-8");
      break;

    case QBOX_PRESET_BAND:
      qbox_state_set_textblock(&state->pre_commands, state->pre_buffer, state->pre_buffer_handler,
                               "set scf_tol 1e-8\n"
                               "set nempty 8");
      qbox_state_set_textblock(&state->post_commands, state->post_buffer, state->post_buffer_handler,
                               "set wf_dyn JD\n"
                               "kpoint delete 0 0 0\n"
                               "# Add your path points with zero weights:\n"
                               "# kpoint add kx ky kz 0.0\n"
                               "run 0 1 200\n"
                               "run 0 1");
      break;

    case QBOX_PRESET_GEOOPT:
      qbox_state_set_textblock(&state->pre_commands, state->pre_buffer, state->pre_buffer_handler,
                               "set scf_tol 1e-8");
      qbox_state_set_textblock(&state->post_commands, state->post_buffer, state->post_buffer_handler,
                               "set atoms_dyn CG\n"
                               "set dt 20\n"
                               "run 100 50");
      break;

    case QBOX_PRESET_FROZEN_PHONON:
      qbox_state_set_textblock(&state->pre_commands, state->pre_buffer, state->pre_buffer_handler,
                               "set scf_tol 1e-8");
      qbox_state_set_string(&state->include_cmd_file, "moves.i", state->entry_include_cmd_file);
      qbox_state_set_textblock(&state->post_commands, state->post_buffer, state->post_buffer_handler,
                               "# moves.i should contain move/run sequences for +/- displacements.");
      break;

    case QBOX_PRESET_HOMO_LUMO:
      qbox_state_set_textblock(&state->pre_commands, state->pre_buffer, state->pre_buffer_handler,
                               "set scf_tol 1e-8\n"
                               "set nempty 1");
      qbox_state_set_textblock(&state->post_commands, state->post_buffer, state->post_buffer_handler,
                               "set wf_dyn JD\n"
                               "set nempty 1\n"
                               "run 0 100\n"
                               "# plot -wf X HOMO.cube\n"
                               "# plot -wf Y LUMO.cube");
      break;
    }
}

static void qbox_apply_preset_from_dialog(gpointer dialog, gint preset)
{
  struct qbox_gui_pak *state;
  gchar *msg;

  g_return_if_fail(dialog != NULL);

  state = dialog_child_get(dialog, "qbox_state");
  if (!state)
    return;

  qbox_apply_preset(state, preset);

  msg = g_strdup_printf("Applied Qbox preset: %s\n", qbox_preset_name(preset));
  gui_text_show(INFO, msg);
  g_free(msg);
}

static void qbox_preset_scf_cb(GtkWidget *w, gpointer dialog)
{
  (void) w;
  qbox_apply_preset_from_dialog(dialog, QBOX_PRESET_SCF);
}

static void qbox_preset_band_cb(GtkWidget *w, gpointer dialog)
{
  (void) w;
  qbox_apply_preset_from_dialog(dialog, QBOX_PRESET_BAND);
}

static void qbox_preset_geoopt_cb(GtkWidget *w, gpointer dialog)
{
  (void) w;
  qbox_apply_preset_from_dialog(dialog, QBOX_PRESET_GEOOPT);
}

static void qbox_preset_frozen_phonon_cb(GtkWidget *w, gpointer dialog)
{
  (void) w;
  qbox_apply_preset_from_dialog(dialog, QBOX_PRESET_FROZEN_PHONON);
}

static void qbox_preset_homo_lumo_cb(GtkWidget *w, gpointer dialog)
{
  (void) w;
  qbox_apply_preset_from_dialog(dialog, QBOX_PRESET_HOMO_LUMO);
}

static gint qbox_prepare_paths(struct qbox_task_pak *job)
{
  g_return_val_if_fail(job != NULL, 1);

  if (g_mkdir_with_parents(job->workdir, 0755) != 0)
    {
    job->error = g_strdup_printf("Unable to create Qbox working directory:\n%s\n",
                                 job->workdir);
    return(1);
    }

  g_free(job->input_path);
  g_free(job->xml_path);
  g_free(job->log_path);
  job->input_path = g_build_filename(job->workdir, job->input_name, NULL);
  job->xml_path = g_build_filename(job->workdir, job->xml_name, NULL);
  job->log_path = g_build_filename(job->workdir, job->log_name, NULL);

  return(0);
}

static gint qbox_write_runtime_input(struct qbox_task_pak *job)
{
  FILE *src = NULL;
  FILE *dest = NULL;
  gchar *line;
  gchar *temp_path = NULL;
  gboolean has_save;
  gboolean has_quit;
  GSList *missing = NULL;
  GSList *invalid = NULL;

  g_return_val_if_fail(job != NULL, 1);

  dest = fopen(job->input_path, "wt");
  if (!dest)
    {
    job->error = g_strdup_printf("Unable to write Qbox input:\n%s\n", job->input_path);
    return(1);
    }

  if (job->write_model_block)
    {
    temp_path = g_strdup_printf("%s.export", job->input_path);

    if (write_qbox(temp_path, job->model))
      {
      fclose(dest);
      job->error = g_strdup_printf("Qbox input export failed for:\n%s\n", job->input_path);
      g_free(temp_path);
      return(1);
      }

    src = fopen(temp_path, "rt");
    if (!src)
      {
      fclose(dest);
      job->error = g_strdup_printf("Unable to reopen temporary Qbox input:\n%s\n", temp_path);
      g_free(temp_path);
      return(1);
      }

    while ((line = file_read_line(src)))
      {
      gchar *tmp;

      tmp = g_strdup(line);
      g_strstrip(tmp);

      if (g_str_has_prefix(tmp, "species "))
        {
        gint num_tokens;
        gchar **buff;
        struct qbox_species_pak *species;

        buff = tokenize(tmp, &num_tokens);
        species = NULL;

        if (num_tokens >= 2)
          species = qbox_species_lookup(job->species, buff[1]);

        if (species && species->path && strlen(species->path))
          {
          fprintf(dest, "species %-4s %s\n", species->symbol, species->path);
          if (!g_file_test(species->path, G_FILE_TEST_EXISTS))
            invalid = qbox_symbol_list_add_unique(invalid, species->symbol);
          g_strfreev(buff);
          g_free(tmp);
          g_free(line);
          continue;
          }

        if (num_tokens >= 2)
          missing = qbox_symbol_list_add_unique(missing, buff[1]);

        g_strfreev(buff);
        }

      fputs(line, dest);
      g_free(tmp);
      g_free(line);
      }

    fclose(src);
    g_remove(temp_path);
    g_free(temp_path);
    }
  else
    {
    fprintf(dest, "# Qbox input generated by GDIS (model export disabled)\n");
    fprintf(dest, "# Provide your own cell/species/atom or load command below.\n");
    }

  qbox_write_text_block(dest, job->pre_commands);

  if (job->write_default_block)
    {
    fprintf(dest, "set ecut %.1f\n", job->ecut);
    fprintf(dest, "set xc %s\n", job->xc && strlen(job->xc) ? job->xc : "PBE");
    if (job->randomize_wf)
      fprintf(dest, "randomize_wf\n");
    fprintf(dest, "set wf_dyn %s\n", job->wf_dyn && strlen(job->wf_dyn) ? job->wf_dyn : "PSDA");
    fprintf(dest, "set ecutprec %.1f\n", job->ecutprec);
    qbox_write_line(dest, job->run_cmd);
    }

  qbox_write_line(dest, job->include_cmd_file);
  qbox_write_text_block(dest, job->post_commands);

  has_save = qbox_line_has_command(job->run_cmd, "save")
             || qbox_text_has_command(job->pre_commands, "save")
             || qbox_text_has_command(job->post_commands, "save");
  has_quit = qbox_line_has_command(job->run_cmd, "quit")
             || qbox_text_has_command(job->pre_commands, "quit")
             || qbox_text_has_command(job->post_commands, "quit");

  if (job->auto_save_xml_cmd && !has_save)
    fprintf(dest, "save %s\n", job->xml_path);
  if (job->auto_quit && !has_quit)
    fprintf(dest, "quit\n");

  fclose(dest);

  if (missing || invalid)
    {
    GString *msg;
    gchar *text;

    msg = g_string_new("Qbox run was not started.\n"
                       "A runnable input template was written here:\n");
    g_string_append_printf(msg, "%s\n", job->input_path);

    if (missing)
      {
      text = qbox_symbol_list_string(missing);
      g_string_append_printf(msg, "Missing pseudopotential paths: %s\n", text);
      g_free(text);
      }
    if (invalid)
      {
      text = qbox_symbol_list_string(invalid);
      g_string_append_printf(msg, "Paths do not exist yet: %s\n", text);
      g_free(text);
      }

    g_string_append(msg, "Set the missing species paths in the Qbox dialog and run again.\n");
    job->error = g_string_free(msg, FALSE);

    qbox_symbol_list_free(missing);
    qbox_symbol_list_free(invalid);
    return(1);
    }

  return(0);
}

static void exec_qbox_task(struct qbox_task_pak *job, struct task_pak *task)
{
  gchar *cmd;
  gchar *qbox_quoted;
  gchar *input_quoted;
  gchar *log_quoted;

  g_return_if_fail(job != NULL);
  g_return_if_fail(task != NULL);

  if (qbox_prepare_paths(job))
    return;

  if (qbox_write_runtime_input(job))
    return;

  qbox_quoted = g_shell_quote(job->qbox_path);
  input_quoted = g_shell_quote(job->input_path);
  log_quoted = g_shell_quote(job->log_path);
  cmd = g_strdup_printf("%s < %s > %s 2>&1", qbox_quoted, input_quoted, log_quoted);

  task->status_file = g_strdup(job->log_path);
  task->is_async = TRUE;
  if (!task_async(cmd, &(task->pid)))
    {
    task->is_async = FALSE;
    g_free(job->error);
    job->error = g_strdup_printf("Unable to launch Qbox with:\n%s\n", job->qbox_path);
    }

  g_free(cmd);
  g_free(qbox_quoted);
  g_free(input_quoted);
  g_free(log_quoted);
}

static void cleanup_qbox_task(gpointer ptr)
{
  struct qbox_task_pak *job = ptr;
  gchar *text;

  g_return_if_fail(job != NULL);

  if (job->error)
    {
    gui_text_show(ERROR, job->error);
    qbox_task_free(job);
    return;
    }

  if (!job->xml_path || !g_file_test(job->xml_path, G_FILE_TEST_EXISTS))
    {
    if (job->load_saved_xml)
      {
      text = g_strdup_printf("Qbox finished, but no saved XML file was found:\n%s\n",
                             job->xml_path ? job->xml_path : "(unknown)");
      gui_text_show(ERROR, text);
      g_free(text);
      qbox_task_free(job);
      return;
      }

    text = g_strdup_printf("Qbox finished.\nLog:\n%s\n(no XML file detected)\n",
                           job->log_path ? job->log_path : "(unknown)");
    gui_text_show(STANDARD, text);
    g_free(text);
    qbox_task_free(job);
    return;
    }

  text = g_strdup_printf("Qbox saved:\n%s\nLog:\n%s\n",
                         job->xml_path,
                         job->log_path ? job->log_path : "(unknown)");
  gui_text_show(STANDARD, text);
  g_free(text);

  if (job->load_saved_xml)
    file_load(job->xml_path, NULL);

  qbox_task_free(job);
}

static struct qbox_task_pak *qbox_task_new_from_dialog(gpointer dialog)
{
  struct model_pak *model;
  struct qbox_gui_pak *state;
  struct qbox_task_pak *job;

  g_return_val_if_fail(dialog != NULL, NULL);

  model = dialog_model(dialog);
  state = dialog_child_get(dialog, "qbox_state");
  if (!model || !state)
    return(NULL);

  job = g_malloc0(sizeof(struct qbox_task_pak));
  job->model = model;
  job->qbox_path = g_strdup(sysenv.qbox_path);
  job->workdir = g_strdup(state->workdir);
  job->input_name = g_strdup(state->input_name);
  job->xml_name = g_strdup(state->xml_name);
  job->log_name = g_strdup(state->log_name);
  job->xc = g_strdup(state->xc);
  job->wf_dyn = g_strdup(state->wf_dyn);
  job->ecut = state->ecut;
  job->ecutprec = state->ecutprec;
  job->randomize_wf = state->randomize_wf;
  job->load_saved_xml = state->load_saved_xml;
  job->write_model_block = state->write_model_block;
  job->write_default_block = state->write_default_block;
  job->auto_save_xml_cmd = state->auto_save_xml_cmd;
  job->auto_quit = state->auto_quit;
  job->run_cmd = g_strdup(state->run_cmd);
  job->include_cmd_file = g_strdup(state->include_cmd_file);
  job->pre_commands = g_strdup(state->pre_commands);
  job->post_commands = g_strdup(state->post_commands);
  job->species = qbox_species_copy_list(state->species);

  return(job);
}

static void qbox_apply_demo_potentials(GtkWidget *w, gpointer dialog)
{
  struct qbox_gui_pak *state;
  GSList *item;

  (void) w;

  g_return_if_fail(dialog != NULL);

  state = dialog_child_get(dialog, "qbox_state");
  if (!state)
    return;

  for (item=state->species ; item ; item=g_slist_next(item))
    {
    struct qbox_species_pak *species = item->data;
    gchar *path;

    path = qbox_demo_potential_path(species->symbol);
    qbox_string_replace(&species->path, path);
    g_free(path);
    }
}

static void qbox_clear_potentials(GtkWidget *w, gpointer dialog)
{
  struct qbox_gui_pak *state;
  GSList *item;

  (void) w;

  g_return_if_fail(dialog != NULL);

  state = dialog_child_get(dialog, "qbox_state");
  if (!state)
    return;

  for (item=state->species ; item ; item=g_slist_next(item))
    {
    struct qbox_species_pak *species = item->data;

    qbox_string_replace(&species->path, NULL);
    }
}

static void qbox_write_input_cb(GtkWidget *w, gpointer dialog)
{
  struct qbox_task_pak *job;
  gchar *text;

  (void) w;

  g_return_if_fail(dialog != NULL);

  job = qbox_task_new_from_dialog(dialog);
  if (!job)
    return;

  if (!job->workdir || !strlen(job->workdir) ||
      !job->input_name || !strlen(job->input_name) ||
      !job->xml_name || !strlen(job->xml_name) ||
      !job->log_name || !strlen(job->log_name))
    {
    gui_text_show(ERROR, "Qbox needs a working directory plus input, XML, and log filenames.\n");
    qbox_task_free(job);
    return;
    }

  if (qbox_prepare_paths(job) || qbox_write_runtime_input(job))
    {
    gui_text_show(ERROR, job->error);
    qbox_task_free(job);
    return;
    }

  text = g_strdup_printf("Wrote runnable Qbox input:\n%s\n", job->input_path);
  gui_text_show(STANDARD, text);
  g_free(text);

  qbox_task_free(job);
}

static void qbox_run_cb(GtkWidget *w, gpointer dialog)
{
  struct qbox_task_pak *job;
  gchar *text;

  (void) w;

  g_return_if_fail(dialog != NULL);

  if (!sysenv.qbox_path || !g_file_test(sysenv.qbox_path, G_FILE_TEST_IS_EXECUTABLE))
    {
    gui_text_show(ERROR, "Qbox executable was not found. Set it in View > Executable paths...\n");
    return;
    }

  job = qbox_task_new_from_dialog(dialog);
  if (!job)
    return;

  if (!job->workdir || !strlen(job->workdir) ||
      !job->input_name || !strlen(job->input_name) ||
      !job->xml_name || !strlen(job->xml_name) ||
      !job->log_name || !strlen(job->log_name))
    {
    qbox_task_free(job);
    gui_text_show(ERROR, "Qbox needs a working directory plus input, XML, and log filenames.\n");
    return;
    }

  text = g_strdup_printf("Queued Qbox job for model: %s\n",
                         job->model->basename ? job->model->basename : "model");
  gui_text_show(INFO, text);
  g_free(text);

  task_new("Qbox", &exec_qbox_task, job, &cleanup_qbox_task, job, job->model);
}

static GtkWidget *qbox_note_label_new(const gchar *text)
{
  GtkWidget *label;

  label = gtk_label_new(text);
#if GTK_MAJOR_VERSION >= 4
  gtk_label_set_wrap(GTK_LABEL(label), TRUE);
#else
  gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
#endif
  gtk_label_set_xalign(GTK_LABEL(label), 0.0);

  return(label);
}

void gui_qbox_dialog(void)
{
  gpointer dialog;
  GtkWidget *window;
  GtkWidget *notebook;
  GtkWidget *page;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *swin;
  GtkWidget *label;
  struct model_pak *model;
  struct qbox_gui_pak *state;
  GSList *item;
  gchar *stem;
  gchar *text;

  model = sysenv.active_model;
  if (!model)
    {
    gui_text_show(ERROR, "Load or create a model first.\n");
    return;
    }

  dialog = dialog_request(QBOX, "Qbox", NULL, NULL, model);
  if (!dialog)
    return;

  window = dialog_window(dialog);
  gtk_window_set_default_size(GTK_WINDOW(window), 720, 520);

  state = g_malloc0(sizeof(struct qbox_gui_pak));
  state->model = model;
  state->workdir = qbox_default_workdir();
  stem = qbox_model_stem(model);
  state->input_name = g_strdup_printf("%s.i", stem);
  state->xml_name = g_strdup_printf("%s.xml", stem);
  state->log_name = g_strdup_printf("%s.log", stem);
  state->xc = g_strdup("PBE");
  state->wf_dyn = g_strdup("PSDA");
  state->ecut = 35.0;
  state->ecutprec = 5.0;
  state->randomize_wf = TRUE;
  state->load_saved_xml = TRUE;
  state->write_model_block = TRUE;
  state->write_default_block = TRUE;
  state->auto_save_xml_cmd = TRUE;
  state->auto_quit = TRUE;
  state->run_cmd = g_strdup("run 0 40");
  state->include_cmd_file = NULL;
  state->pre_commands = NULL;
  state->post_commands = NULL;
  state->species = qbox_collect_species(model);
  g_free(stem);

  dialog_child_set(dialog, "qbox_state", state);
  g_signal_connect(GTK_OBJECT(window), "destroy",
                   GTK_SIGNAL_FUNC(qbox_gui_state_free_cb), state);

  notebook = gtk_notebook_new();
  gtk_box_pack_start(GTK_BOX(GDIS_DIALOG_CONTENTS(window)), notebook, TRUE, TRUE, 0);

  page = gtk_vbox_new(FALSE, PANEL_SPACING);
  gtk_container_set_border_width(GTK_CONTAINER(page), PANEL_SPACING);

  frame = gtk_frame_new("Model");
  gtk_box_pack_start(GTK_BOX(page), frame, FALSE, FALSE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(frame), PANEL_SPACING);
  vbox = gtk_vbox_new(FALSE, PANEL_SPACING);
  gtk_container_add(GTK_CONTAINER(frame), vbox);

  text = g_strdup_printf("Active model: %s", model->basename ? model->basename : "model");
  label = gtk_label_new(text);
  gtk_label_set_xalign(GTK_LABEL(label), 0.0);
  gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
  g_free(text);

  text = g_strdup_printf("Qbox executable: %s",
                         sysenv.qbox_path ? sysenv.qbox_path : "(not configured)");
  label = gtk_label_new(text);
  gtk_label_set_xalign(GTK_LABEL(label), 0.0);
  gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
  g_free(text);

  frame = gtk_frame_new("Files");
  gtk_box_pack_start(GTK_BOX(page), frame, FALSE, FALSE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(frame), PANEL_SPACING);
  vbox = gtk_vbox_new(FALSE, PANEL_SPACING);
  gtk_container_add(GTK_CONTAINER(frame), vbox);

  gui_text_entry("Work dir", &state->workdir, TRUE, TRUE, vbox);
  gui_text_entry("Input", &state->input_name, TRUE, TRUE, vbox);
  gui_text_entry("Saved XML", &state->xml_name, TRUE, TRUE, vbox);
  gui_text_entry("Log", &state->log_name, TRUE, TRUE, vbox);

  frame = gtk_frame_new("Run Settings");
  gtk_box_pack_start(GTK_BOX(page), frame, FALSE, FALSE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(frame), PANEL_SPACING);
  vbox = gtk_vbox_new(FALSE, PANEL_SPACING);
  gtk_container_add(GTK_CONTAINER(frame), vbox);

  gui_direct_spin("Ecut", &state->ecut, 5.0, 300.0, 5.0, NULL, NULL, vbox);
  gui_direct_spin("Ecut precision", &state->ecutprec, 1.0, 40.0, 1.0, NULL, NULL, vbox);
  state->entry_xc = gui_text_entry("XC", &state->xc, TRUE, TRUE, vbox);
  state->entry_wf_dyn = gui_text_entry("WF dyn", &state->wf_dyn, TRUE, TRUE, vbox);
  state->entry_run_cmd = gui_text_entry("Default run command", &state->run_cmd, TRUE, TRUE, vbox);
  state->check_randomize_wf = gui_direct_check("Randomize wavefunction", &state->randomize_wf,
                                               NULL, NULL, vbox);
  state->check_load_saved_xml = gui_direct_check("Load saved XML after Qbox finishes", &state->load_saved_xml,
                                                 NULL, NULL, vbox);

  label = qbox_note_label_new("Write Input writes a runnable script with model export + defaults."
                              " Use the Expert tab to add any official Qbox commands, include external command files,"
                              " or disable auto-generated sections.");
  gtk_box_pack_start(GTK_BOX(page), label, FALSE, FALSE, 0);

  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page, gtk_label_new("Setup"));

  page = gtk_vbox_new(FALSE, PANEL_SPACING);
  gtk_container_set_border_width(GTK_CONTAINER(page), PANEL_SPACING);

  hbox = gtk_hbox_new(FALSE, PANEL_SPACING);
  gtk_box_pack_start(GTK_BOX(page), hbox, FALSE, FALSE, 0);
  gui_button("Use Demo Potentials", qbox_apply_demo_potentials, dialog, hbox, TT);
  gui_button("Clear Paths", qbox_clear_potentials, dialog, hbox, TT);

  label = qbox_note_label_new("Set one XML pseudopotential path per element in the active model."
                              " The bundled demo files are available for C, H, O, and Si only.");
  gtk_box_pack_start(GTK_BOX(page), label, FALSE, FALSE, 0);

  swin = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start(GTK_BOX(page), swin, TRUE, TRUE, 0);

  vbox = gtk_vbox_new(FALSE, PANEL_SPACING);
  gtk_container_set_border_width(GTK_CONTAINER(vbox), PANEL_SPACING);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(swin), vbox);

  if (!state->species)
    {
    label = gtk_label_new("No atoms were found in the active model.");
    gtk_label_set_xalign(GTK_LABEL(label), 0.0);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
    }
  else
    {
    for (item=state->species ; item ; item=g_slist_next(item))
      {
      struct qbox_species_pak *species = item->data;

      gui_text_entry(species->symbol, &species->path, TRUE, TRUE, vbox);
      }
    }

  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page, gtk_label_new("Potentials"));

  page = gtk_vbox_new(FALSE, PANEL_SPACING);
  gtk_container_set_border_width(GTK_CONTAINER(page), PANEL_SPACING);

  frame = gtk_frame_new("Preset Profiles");
  gtk_box_pack_start(GTK_BOX(page), frame, FALSE, FALSE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(frame), PANEL_SPACING);
  vbox = gtk_vbox_new(FALSE, PANEL_SPACING);
  gtk_container_add(GTK_CONTAINER(frame), vbox);

  label = qbox_note_label_new("Profiles auto-fill command fields. Review/edit, then use Write Input or Execute.");
  gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

  hbox = gtk_hbox_new(FALSE, PANEL_SPACING);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
  gui_button("SCF", qbox_preset_scf_cb, dialog, hbox, TT);
  gui_button("Band", qbox_preset_band_cb, dialog, hbox, TT);
  gui_button("GeoOpt", qbox_preset_geoopt_cb, dialog, hbox, TT);

  hbox = gtk_hbox_new(FALSE, PANEL_SPACING);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
  gui_button("FrozenPhonon", qbox_preset_frozen_phonon_cb, dialog, hbox, TT);
  gui_button("HOMO-LUMO", qbox_preset_homo_lumo_cb, dialog, hbox, TT);

  frame = gtk_frame_new("Input Composition");
  gtk_box_pack_start(GTK_BOX(page), frame, FALSE, FALSE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(frame), PANEL_SPACING);
  vbox = gtk_vbox_new(FALSE, PANEL_SPACING);
  gtk_container_add(GTK_CONTAINER(frame), vbox);

  state->check_write_model_block = gui_direct_check("Write model block (cell/species/atom)", &state->write_model_block,
                                                    NULL, NULL, vbox);
  state->check_write_default_block = gui_direct_check("Write default settings block", &state->write_default_block,
                                                      NULL, NULL, vbox);
  state->check_auto_save_xml_cmd = gui_direct_check("Auto save Saved XML filename", &state->auto_save_xml_cmd,
                                                    NULL, NULL, vbox);
  state->check_auto_quit = gui_direct_check("Auto append quit", &state->auto_quit,
                                            NULL, NULL, vbox);
  state->entry_include_cmd_file = gui_text_entry("Include command file (optional)", &state->include_cmd_file, TRUE, TRUE, vbox);

  label = qbox_note_label_new("Include command file can be a relative path such as moves.i."
                              " Disable model/default blocks for full manual scripts.");
  gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

  frame = gtk_frame_new("Pre-default Commands");
  gtk_box_pack_start(GTK_BOX(page), frame, TRUE, TRUE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(frame), PANEL_SPACING);
  vbox = gtk_vbox_new(FALSE, PANEL_SPACING);
  gtk_container_add(GTK_CONTAINER(frame), vbox);
  label = qbox_note_label_new("Commands inserted before default settings block.");
  gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
  swin = qbox_text_window_new(&state->pre_commands, &state->pre_buffer, &state->pre_buffer_handler);
  gtk_widget_set_size_request(swin, -1, 110);
  gtk_box_pack_start(GTK_BOX(vbox), swin, TRUE, TRUE, 0);

  frame = gtk_frame_new("Post/default-override Commands");
  gtk_box_pack_start(GTK_BOX(page), frame, TRUE, TRUE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(frame), PANEL_SPACING);
  vbox = gtk_vbox_new(FALSE, PANEL_SPACING);
  gtk_container_add(GTK_CONTAINER(frame), vbox);
  label = qbox_note_label_new("Commands inserted after defaults. Use for kpoint/additional run/plot/response/spectrum/constraints.");
  gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
  swin = qbox_text_window_new(&state->post_commands, &state->post_buffer, &state->post_buffer_handler);
  gtk_widget_set_size_request(swin, -1, 140);
  gtk_box_pack_start(GTK_BOX(vbox), swin, TRUE, TRUE, 0);

  label = qbox_note_label_new("Examples: set scf_tol 1e-8, set nempty 8, kpoint add ... 0.0,"
                              " compute_mlwf, response, plot -wf, move atom1 by ..., run ...");
  gtk_box_pack_start(GTK_BOX(page), label, FALSE, FALSE, 0);

  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page, gtk_label_new("Expert"));

  gui_button("Write Input", qbox_write_input_cb, dialog, GDIS_DIALOG_ACTIONS(window), TT);
  gui_stock_button(GTK_STOCK_EXECUTE, qbox_run_cb, dialog, GDIS_DIALOG_ACTIONS(window));
  gui_stock_button(GTK_STOCK_CLOSE, dialog_destroy, dialog, GDIS_DIALOG_ACTIONS(window));

  gtk_widget_show_all(window);
}
