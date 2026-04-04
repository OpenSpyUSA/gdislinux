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
  g_free(job->input_path);
  g_free(job->xml_path);
  g_free(job->log_path);
  g_slist_free_full(job->species, qbox_species_free);
  g_free(job->message);
  g_free(job->error);
  g_free(job);
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
  FILE *src, *dest;
  gchar *line;
  gchar *temp_path;
  GSList *missing = NULL;
  GSList *invalid = NULL;

  g_return_val_if_fail(job != NULL, 1);

  temp_path = g_strdup_printf("%s.export", job->input_path);

  if (write_qbox(temp_path, job->model))
    {
    job->error = g_strdup_printf("Qbox input export failed for:\n%s\n", job->input_path);
    g_free(temp_path);
    return(1);
    }

  src = fopen(temp_path, "rt");
  if (!src)
    {
    job->error = g_strdup_printf("Unable to reopen temporary Qbox input:\n%s\n", temp_path);
    g_free(temp_path);
    return(1);
    }

  dest = fopen(job->input_path, "wt");
  if (!dest)
    {
    fclose(src);
    job->error = g_strdup_printf("Unable to write Qbox input:\n%s\n", job->input_path);
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

  fprintf(dest, "set ecut %.1f\n", job->ecut);
  fprintf(dest, "set xc %s\n", job->xc && strlen(job->xc) ? job->xc : "PBE");
  if (job->randomize_wf)
    fprintf(dest, "randomize_wf\n");
  fprintf(dest, "set wf_dyn %s\n", job->wf_dyn && strlen(job->wf_dyn) ? job->wf_dyn : "PSDA");
  fprintf(dest, "set ecutprec %.1f\n", job->ecutprec);
  fprintf(dest, "save %s\n", job->xml_path);
  fprintf(dest, "quit\n");

  fclose(src);
  fclose(dest);
  g_remove(temp_path);
  g_free(temp_path);

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
    text = g_strdup_printf("Qbox finished, but no saved XML file was found:\n%s\n",
                           job->xml_path ? job->xml_path : "(unknown)");
    gui_text_show(ERROR, text);
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

  text = g_strdup_printf("Queued Qbox save for model: %s\n",
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
  gui_text_entry("XC", &state->xc, TRUE, TRUE, vbox);
  gui_text_entry("WF dyn", &state->wf_dyn, TRUE, TRUE, vbox);
  gui_direct_check("Randomize wavefunction", &state->randomize_wf, NULL, NULL, vbox);
  gui_direct_check("Load saved XML after Qbox finishes", &state->load_saved_xml,
                   NULL, NULL, vbox);

  label = qbox_note_label_new("Write Input now writes a runnable Qbox input file using the current settings."
                              " Execute runs Qbox and loads the saved XML when it finishes.");
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

  gui_button("Write Input", qbox_write_input_cb, dialog, GDIS_DIALOG_ACTIONS(window), TT);
  gui_stock_button(GTK_STOCK_EXECUTE, qbox_run_cb, dialog, GDIS_DIALOG_ACTIONS(window));
  gui_stock_button(GTK_STOCK_CLOSE, dialog_destroy, dialog, GDIS_DIALOG_ACTIONS(window));
}
