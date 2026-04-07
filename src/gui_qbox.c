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
#include <sys/wait.h>
#include <glib/gstdio.h>

#include "gdis.h"
#include "coords.h"
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
  gchar *xyz_name;
  gchar *mpirun_path;
  gchar *xc;
  gchar *scf_tol;
  gchar *wf_dyn;
  gchar *cell_lock;
  gchar *ext_stress;
  gchar *ref_cell;
  gchar *vext;
  gchar *e_field;
  gchar *polarization;
  gchar *iter_cmd;
  gchar *atoms_dyn;
  gchar *cell_dyn;
  gchar *thermostat;
  gchar *run_timeout;
  gdouble ecut;
  gdouble ecutprec;
  gdouble dt;
  gdouble fermi_temp;
  gdouble charge_mix_coeff;
  gdouble charge_mix_ndim;
  gdouble charge_mix_rcut;
  gdouble cell_mass;
  gdouble emass;
  gdouble randomize_v;
  gdouble force_tol;
  gdouble stress_tol;
  gdouble th_temp;
  gdouble th_time;
  gdouble th_width;
  gdouble mpi_ranks;
  gdouble omp_threads;
  gint randomize_wf;
  gdouble nempty;
  gdouble nspin;
  gdouble delta_spin;
  gdouble net_charge;
  gint use_mpi;
  gint use_nspin;
  gint use_delta_spin;
  gint use_net_charge;
  gint use_atoms_dyn;
  gint use_dt;
  gint use_fermi_temp;
  gint use_charge_mix_coeff;
  gint use_charge_mix_ndim;
  gint use_charge_mix_rcut;
  gint use_randomize_v;
  gint use_nempty;
  gint use_force_tol;
  gint use_ref_cell;
  gint use_vext;
  gint use_e_field;
  gint use_polarization;
  gint use_iter_cmd;
  gint use_iter_cmd_period;
  gint use_lock_cm;
  gint use_cell_lock;
  gint use_cell_mass;
  gint use_cell_dyn;
  gint use_stress;
  gint use_ext_stress;
  gint use_stress_tol;
  gint use_emass;
  gint use_thermostat;
  gint use_th_temp;
  gint use_th_time;
  gint use_th_width;
  gdouble iter_cmd_period;
  gint auto_convert_xyz;
  gint load_xyz_after_convert;
  gint open_animation_after_xyz;
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
  GtkWidget *entry_scf_tol;
  GtkWidget *entry_wf_dyn;
  GtkWidget *entry_cell_lock;
  GtkWidget *entry_ext_stress;
  GtkWidget *entry_ref_cell;
  GtkWidget *entry_vext;
  GtkWidget *entry_e_field;
  GtkWidget *entry_polarization;
  GtkWidget *entry_iter_cmd;
  GtkWidget *entry_atoms_dyn;
  GtkWidget *entry_cell_dyn;
  GtkWidget *entry_thermostat;
  GtkWidget *entry_run_timeout;
  GtkWidget *entry_run_cmd;
  GtkWidget *entry_include_cmd_file;
  GtkWidget *check_randomize_wf;
  GtkWidget *check_use_nspin;
  GtkWidget *check_use_delta_spin;
  GtkWidget *check_use_net_charge;
  GtkWidget *check_use_atoms_dyn;
  GtkWidget *check_use_dt;
  GtkWidget *check_use_fermi_temp;
  GtkWidget *check_use_charge_mix_coeff;
  GtkWidget *check_use_charge_mix_ndim;
  GtkWidget *check_use_charge_mix_rcut;
  GtkWidget *check_use_randomize_v;
  GtkWidget *check_use_nempty;
  GtkWidget *check_use_force_tol;
  GtkWidget *check_use_ref_cell;
  GtkWidget *check_use_vext;
  GtkWidget *check_use_e_field;
  GtkWidget *check_use_polarization;
  GtkWidget *check_use_iter_cmd;
  GtkWidget *check_use_iter_cmd_period;
  GtkWidget *check_use_lock_cm;
  GtkWidget *check_use_cell_lock;
  GtkWidget *check_use_cell_mass;
  GtkWidget *check_use_cell_dyn;
  GtkWidget *check_use_stress;
  GtkWidget *check_use_ext_stress;
  GtkWidget *check_use_stress_tol;
  GtkWidget *check_use_emass;
  GtkWidget *check_use_thermostat;
  GtkWidget *check_use_th_temp;
  GtkWidget *check_use_th_time;
  GtkWidget *check_use_th_width;
  GtkWidget *check_auto_convert_xyz;
  GtkWidget *check_load_xyz_after_convert;
  GtkWidget *check_open_animation_after_xyz;
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
  gchar *xyz_name;
  gchar *mpirun_path;
  gchar *xc;
  gchar *scf_tol;
  gchar *wf_dyn;
  gchar *cell_lock;
  gchar *ext_stress;
  gchar *ref_cell;
  gchar *vext;
  gchar *e_field;
  gchar *polarization;
  gchar *iter_cmd;
  gchar *atoms_dyn;
  gchar *cell_dyn;
  gchar *thermostat;
  gchar *run_timeout;
  gchar *input_path;
  gchar *xml_path;
  gchar *log_path;
  gchar *xyz_path;
  gdouble ecut;
  gdouble ecutprec;
  gdouble dt;
  gdouble fermi_temp;
  gdouble charge_mix_coeff;
  gdouble charge_mix_ndim;
  gdouble charge_mix_rcut;
  gdouble cell_mass;
  gdouble emass;
  gdouble randomize_v;
  gdouble force_tol;
  gdouble stress_tol;
  gdouble th_temp;
  gdouble th_time;
  gdouble th_width;
  gdouble mpi_ranks;
  gdouble omp_threads;
  gint randomize_wf;
  gdouble nempty;
  gdouble nspin;
  gdouble delta_spin;
  gdouble net_charge;
  gint use_mpi;
  gint use_nspin;
  gint use_delta_spin;
  gint use_net_charge;
  gint use_atoms_dyn;
  gint use_dt;
  gint use_fermi_temp;
  gint use_charge_mix_coeff;
  gint use_charge_mix_ndim;
  gint use_charge_mix_rcut;
  gint use_randomize_v;
  gint use_nempty;
  gint use_force_tol;
  gint use_ref_cell;
  gint use_vext;
  gint use_e_field;
  gint use_polarization;
  gint use_iter_cmd;
  gint use_iter_cmd_period;
  gint use_lock_cm;
  gint use_cell_lock;
  gint use_cell_mass;
  gint use_cell_dyn;
  gint use_stress;
  gint use_ext_stress;
  gint use_stress_tol;
  gint use_emass;
  gint use_thermostat;
  gint use_th_temp;
  gint use_th_time;
  gint use_th_width;
  gdouble iter_cmd_period;
  gint auto_convert_xyz;
  gint load_xyz_after_convert;
  gint open_animation_after_xyz;
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

static gint qbox_resource_count(gdouble value)
{
  gint count;

  count = (gint) (value + 0.5);
  if (count < 1)
    count = 1;

  return(count);
}

static gchar *qbox_resolve_executable(const gchar *value)
{
  if (!value || !strlen(value))
    return(NULL);

  if (g_path_is_absolute(value) || strchr(value, G_DIR_SEPARATOR))
    {
    if (g_file_test(value, G_FILE_TEST_IS_EXECUTABLE))
      return(g_strdup(value));
    return(NULL);
    }

  return(g_find_program_in_path(value));
}

static gchar *qbox_find_local_named_executable(const gchar *name)
{
  gchar *path;
  gchar *project_root;

  if (!name || !strlen(name))
    return(NULL);

  if (g_path_is_absolute(name) || strchr(name, G_DIR_SEPARATOR))
    return(NULL);

  if (sysenv.gdis_path && strlen(sysenv.gdis_path))
    {
    path = g_build_filename(sysenv.gdis_path, name, NULL);
    if (g_file_test(path, G_FILE_TEST_IS_EXECUTABLE))
      return(path);
    g_free(path);

    project_root = g_path_get_dirname(sysenv.gdis_path);
    path = g_build_filename(project_root, "bin", name, NULL);
    g_free(project_root);
    if (g_file_test(path, G_FILE_TEST_IS_EXECUTABLE))
      return(path);
    g_free(path);
    }

  if (sysenv.cwd && strlen(sysenv.cwd))
    {
    path = g_build_filename(sysenv.cwd, "bin", name, NULL);
    if (g_file_test(path, G_FILE_TEST_IS_EXECUTABLE))
      return(path);
    g_free(path);
    }

  return(NULL);
}

static gchar *qbox_detect_executable(void)
{
  gchar *path;

  path = qbox_resolve_executable(sysenv.qbox_path);
  if (path)
    return(path);

  if (sysenv.qbox_exe && strlen(sysenv.qbox_exe))
    {
    path = qbox_find_local_named_executable(sysenv.qbox_exe);
    if (path)
      return(path);

    path = qbox_resolve_executable(sysenv.qbox_exe);
    if (path)
      return(path);
    }

  if (!sysenv.qbox_exe || g_ascii_strcasecmp(sysenv.qbox_exe, "qbox") != 0)
    {
    path = qbox_find_local_named_executable("qbox");
    if (path)
      return(path);

    path = qbox_resolve_executable("qbox");
    if (path)
      return(path);
    }

  if (!sysenv.qbox_exe || g_ascii_strcasecmp(sysenv.qbox_exe, "qb") != 0)
    {
    path = qbox_find_local_named_executable("qb");
    if (path)
      return(path);

    path = qbox_resolve_executable("qb");
    if (path)
      return(path);
    }

  return(NULL);
}

static gboolean qbox_refresh_executable_path(void)
{
  gchar *resolved;

  resolved = qbox_detect_executable();
  if (!resolved)
    return(FALSE);

  if (g_strcmp0(sysenv.qbox_path, resolved) != 0)
    {
    g_free(sysenv.qbox_path);
    sysenv.qbox_path = resolved;
    write_gdisrc();
    }
  else
    g_free(resolved);

  return(TRUE);
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

static gchar *qbox_find_symbol_xml_in_dir(const gchar *dir_path, const gchar *symbol_canon)
{
  gchar *candidate;
  gchar *path;
  gchar *best = NULL;
  GDir *dir;
  const gchar *name;

  g_return_val_if_fail(symbol_canon != NULL, NULL);

  if (!dir_path || !strlen(dir_path))
    return(NULL);
  if (!g_file_test(dir_path, G_FILE_TEST_IS_DIR))
    return(NULL);
  if (g_getenv("GDIS_QBOX_DEBUG"))
    {
    fprintf(stderr, "[qbox-ui] lookup symbol=%s dir=%s\n", symbol_canon, dir_path);
    fflush(stderr);
    }

  candidate = g_strdup_printf("%s_ONCV_PBE_sr.xml", symbol_canon);
  path = g_build_filename(dir_path, candidate, NULL);
  g_free(candidate);
  if (g_file_test(path, G_FILE_TEST_EXISTS))
    {
    if (g_getenv("GDIS_QBOX_DEBUG"))
      {
      fprintf(stderr, "[qbox-ui] found %s\n", path);
      fflush(stderr);
      }
    return(path);
    }
  g_free(path);

  candidate = g_strdup_printf("%s_ONCV_PBE_fr.xml", symbol_canon);
  path = g_build_filename(dir_path, candidate, NULL);
  g_free(candidate);
  if (g_file_test(path, G_FILE_TEST_EXISTS))
    {
    if (g_getenv("GDIS_QBOX_DEBUG"))
      {
      fprintf(stderr, "[qbox-ui] found %s\n", path);
      fflush(stderr);
      }
    return(path);
    }
  g_free(path);

  candidate = g_strdup_printf("%s.xml", symbol_canon);
  path = g_build_filename(dir_path, candidate, NULL);
  g_free(candidate);
  if (g_file_test(path, G_FILE_TEST_EXISTS))
    {
    if (g_getenv("GDIS_QBOX_DEBUG"))
      {
      fprintf(stderr, "[qbox-ui] found %s\n", path);
      fflush(stderr);
      }
    return(path);
    }
  g_free(path);

  dir = g_dir_open(dir_path, 0, NULL);
  if (!dir)
    return(NULL);

  candidate = g_strdup_printf("%s_", symbol_canon);
  while ((name = g_dir_read_name(dir)))
    {
    if (!g_str_has_prefix(name, candidate) || !g_str_has_suffix(name, ".xml"))
      continue;
    if (!best || g_strrstr(name, "_sr.xml"))
      {
      g_free(best);
      best = g_build_filename(dir_path, name, NULL);
      if (g_strrstr(name, "_sr.xml"))
        break;
      }
    }
  g_free(candidate);
  g_dir_close(dir);
  if (best && g_getenv("GDIS_QBOX_DEBUG"))
    {
    fprintf(stderr, "[qbox-ui] found %s\n", best);
    fflush(stderr);
    }

  return(best);
}

static gchar *qbox_demo_potential_path(const gchar *symbol)
{
  const gchar *legacy_name = NULL;
  gchar *path = NULL;
  gchar *symbol_canon = NULL;
  gchar *project_root = NULL;
  const gchar *env_dir;
  const gchar *search_dirs[3];
  gint i;

  static const gchar *legacy_demo_dir[] =
  {
    ".localdeps", "qbox-public", "test", "potentials", NULL
  };

  if (!symbol)
    return(NULL);

  if (g_ascii_strcasecmp(symbol, "C") == 0)
    legacy_name = "C_HSCV_PBE-1.0.xml";
  else if (g_ascii_strcasecmp(symbol, "H") == 0)
    legacy_name = "H_HSCV_PBE-1.0.xml";
  else if (g_ascii_strcasecmp(symbol, "O") == 0)
    legacy_name = "O_HSCV_PBE-1.0.xml";
  else if (g_ascii_strcasecmp(symbol, "Si") == 0)
    legacy_name = "Si_VBC_LDA-1.0.xml";

  symbol_canon = g_strdup(symbol);
  if (symbol_canon[0])
    {
    gchar *p;
    symbol_canon[0] = g_ascii_toupper(symbol_canon[0]);
    for (p=symbol_canon+1 ; *p ; p++)
      *p = g_ascii_tolower(*p);
    }

  env_dir = g_getenv("GDIS_QBOX_POTENTIAL_DIR");
  search_dirs[0] = "external/pseudos/qbox-xml-oncv-sr";
  search_dirs[1] = "external/pseudos/qbox-xml-oncv";
  search_dirs[2] = NULL;
  if (sysenv.gdis_path && strlen(sysenv.gdis_path))
    project_root = g_path_get_dirname(sysenv.gdis_path);
  if (g_getenv("GDIS_QBOX_DEBUG"))
    {
    fprintf(stderr, "[qbox-ui] demo lookup symbol=%s cwd=%s gdis_path=%s project_root=%s\n",
            symbol_canon,
            sysenv.cwd ? sysenv.cwd : "(null)",
            sysenv.gdis_path ? sysenv.gdis_path : "(null)",
            project_root ? project_root : "(null)");
    fflush(stderr);
    }

  if (legacy_name)
    {
    path = g_build_filename(sysenv.cwd,
                            legacy_demo_dir[0], legacy_demo_dir[1], legacy_demo_dir[2],
                            legacy_demo_dir[3], legacy_name, NULL);
    if (!g_file_test(path, G_FILE_TEST_EXISTS) && project_root)
      {
      g_free(path);
      path = g_build_filename(project_root,
                              legacy_demo_dir[0], legacy_demo_dir[1], legacy_demo_dir[2],
                              legacy_demo_dir[3], legacy_name, NULL);
      }
    if (g_file_test(path, G_FILE_TEST_EXISTS))
      {
      g_free(project_root);
      g_free(symbol_canon);
      return(path);
      }
    g_free(path);
    path = NULL;
    }

  if (env_dir && strlen(env_dir))
    {
    const gchar *base_dirs[3];
    gint j;

    if (g_path_is_absolute(env_dir))
      {
      path = qbox_find_symbol_xml_in_dir(env_dir, symbol_canon);
      if (path)
        {
        g_free(project_root);
        g_free(symbol_canon);
        return(path);
        }
      }
    else
      {
      base_dirs[0] = sysenv.cwd;
      base_dirs[1] = project_root;
      base_dirs[2] = NULL;
      for (j=0 ; base_dirs[j] ; j++)
        {
        gchar *dir_path;

        if (!strlen(base_dirs[j]))
          continue;

        dir_path = g_build_filename(base_dirs[j], env_dir, NULL);
        if (g_getenv("GDIS_QBOX_DEBUG"))
          {
          fprintf(stderr, "[qbox-ui] try env dir=%s\n", dir_path);
          fflush(stderr);
          }
        path = qbox_find_symbol_xml_in_dir(dir_path, symbol_canon);
        g_free(dir_path);
        if (path)
          {
          g_free(project_root);
          g_free(symbol_canon);
          return(path);
          }
        }
      }
    }

  for (i=0 ; search_dirs[i] ; i++)
    {
    const gchar *dir_text = search_dirs[i];
    const gchar *base_dirs[3];
    gint j;

    if (!dir_text || !strlen(dir_text))
      continue;

    if (g_path_is_absolute(dir_text))
      {
      path = qbox_find_symbol_xml_in_dir(dir_text, symbol_canon);
      if (path)
        {
        g_free(project_root);
        g_free(symbol_canon);
        return(path);
        }
      continue;
      }

    base_dirs[0] = sysenv.cwd;
    base_dirs[1] = project_root;
    base_dirs[2] = NULL;

    for (j=0 ; base_dirs[j] ; j++)
      {
      gchar *dir_path;

      if (!strlen(base_dirs[j]))
        continue;

      dir_path = g_build_filename(base_dirs[j], dir_text, NULL);
      if (g_getenv("GDIS_QBOX_DEBUG"))
        {
        fprintf(stderr, "[qbox-ui] try relative dir=%s\n", dir_path);
        fflush(stderr);
        }
      path = qbox_find_symbol_xml_in_dir(dir_path, symbol_canon);
      g_free(dir_path);
      if (path)
        {
        g_free(project_root);
        g_free(symbol_canon);
        return(path);
        }
      }
    }

  g_free(project_root);
  g_free(symbol_canon);
  return(NULL);
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
  GSList *item;
  GHashTable *seen;
  struct elem_pak elem;

  g_return_val_if_fail(model != NULL, NULL);

  seen = g_hash_table_new_full(&g_str_hash, &hash_strcmp, &g_free, NULL);

  for (item=model->cores ; item ; item=g_slist_next(item))
    {
    const gchar *symbol;
    struct core_pak *core;
    gchar *demo_path;

    core = item->data;
    if (!core || (core->status & DELETED))
      continue;

    if (get_elem_data(core->atom_code, &elem, NULL))
      continue;

    symbol = elem.symbol;
    if (!symbol || !strlen(symbol))
      continue;
    if (g_hash_table_lookup(seen, symbol))
      continue;

    g_hash_table_insert(seen, g_strdup(symbol), GINT_TO_POINTER(1));

    demo_path = qbox_demo_potential_path(symbol);
    if (g_getenv("GDIS_QBOX_DEBUG"))
      {
      fprintf(stderr, "[qbox-ui] collect species=%s demo=%s\n",
              symbol, demo_path ? demo_path : "(none)");
      fflush(stderr);
      }
    species = g_slist_append(species, qbox_species_new(symbol, demo_path));
    g_free(demo_path);
    }

  g_hash_table_destroy(seen);

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
  g_free(state->xyz_name);
  g_free(state->mpirun_path);
  g_free(state->xc);
  g_free(state->scf_tol);
  g_free(state->wf_dyn);
  g_free(state->cell_lock);
  g_free(state->ext_stress);
  g_free(state->ref_cell);
  g_free(state->vext);
  g_free(state->e_field);
  g_free(state->polarization);
  g_free(state->iter_cmd);
  g_free(state->atoms_dyn);
  g_free(state->cell_dyn);
  g_free(state->thermostat);
  g_free(state->run_timeout);
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
  g_free(job->xyz_name);
  g_free(job->mpirun_path);
  g_free(job->xc);
  g_free(job->scf_tol);
  g_free(job->wf_dyn);
  g_free(job->cell_lock);
  g_free(job->ext_stress);
  g_free(job->ref_cell);
  g_free(job->vext);
  g_free(job->e_field);
  g_free(job->polarization);
  g_free(job->iter_cmd);
  g_free(job->atoms_dyn);
  g_free(job->cell_dyn);
  g_free(job->thermostat);
  g_free(job->run_timeout);
  g_free(job->run_cmd);
  g_free(job->include_cmd_file);
  g_free(job->pre_commands);
  g_free(job->post_commands);
  g_free(job->input_path);
  g_free(job->xml_path);
  g_free(job->log_path);
  g_free(job->xyz_path);
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

static void qbox_state_append_textblock(gchar **target, GtkTextBuffer *buffer,
                                        gulong handler, const gchar *value)
{
  gchar *text;
  const gchar *current;

  g_return_if_fail(target != NULL);

  if (!value || !strlen(value))
    return;

  current = (*target && strlen(*target)) ? *target : NULL;
  if (current)
    {
    if (current[strlen(current)-1] == '\n')
      text = g_strconcat(current, value, NULL);
    else
      text = g_strconcat(current, "\n", value, NULL);
    }
  else
    text = g_strdup(value);

  qbox_state_set_textblock(target, buffer, handler, text);
  g_free(text);
}

#if GTK_MAJOR_VERSION >= 4
static void qbox_ensure_compact_action_css(GtkWidget *widget)
{
  static GtkCssProvider *provider = NULL;
  GdkDisplay *display;
  const gchar *css =
    ".gdis-qbox-actions {"
    "  margin-top: 0;"
    "  margin-bottom: 0;"
    "  padding-top: 0;"
    "  padding-bottom: 0;"
    "}"
    ".gdis-qbox-actions button {"
    "  min-height: 0;"
    "  min-width: 0;"
    "  padding-top: 1px;"
    "  padding-bottom: 1px;"
    "  padding-left: 10px;"
    "  padding-right: 10px;"
    "}";

  g_return_if_fail(widget != NULL);

  display = gtk_widget_get_display(widget);
  if (!display)
    return;

  if (!provider)
    {
    provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider, css, -1);
    }

  if (!g_object_get_data(G_OBJECT(display), "gdis-qbox-compact-actions"))
    {
    gtk_style_context_add_provider_for_display(display,
                                               GTK_STYLE_PROVIDER(provider),
                                               GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_set_data(G_OBJECT(display), "gdis-qbox-compact-actions",
                      GINT_TO_POINTER(TRUE));
    }
}

static void qbox_compact_action_row(GtkWidget *actions)
{
  g_return_if_fail(actions != NULL);

  qbox_ensure_compact_action_css(actions);
  gtk_widget_add_css_class(actions, "gdis-qbox-actions");
  gtk_box_set_spacing(GTK_BOX(actions), 3);
  gtk_widget_set_margin_top(actions, 0);
  gtk_widget_set_margin_bottom(actions, 0);
  gtk_widget_set_margin_start(actions, 4);
  gtk_widget_set_margin_end(actions, 4);
  gtk_widget_set_valign(actions, GTK_ALIGN_END);
  gtk_widget_set_vexpand(actions, FALSE);
}

static void qbox_compact_action_button(GtkWidget *button)
{
  g_return_if_fail(button != NULL);

  qbox_ensure_compact_action_css(button);
  gtk_widget_set_margin_top(button, 0);
  gtk_widget_set_margin_bottom(button, 0);
}
#endif

enum
{
  QBOX_TEMPLATE_PRE = 0,
  QBOX_TEMPLATE_POST
};

static void qbox_template_append_cb(GtkWidget *w, gpointer dialog)
{
  struct qbox_gui_pak *state;
  const gchar *text;
  gint target;
  gchar *msg;

  g_return_if_fail(dialog != NULL);

  state = dialog_child_get(dialog, "qbox_state");
  if (!state)
    return;

  text = g_object_get_data(G_OBJECT(w), "qbox_template_text");
  target = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w), "qbox_template_target"));
  if (!text || !strlen(text))
    return;

  if (target == QBOX_TEMPLATE_PRE)
    qbox_state_append_textblock(&state->pre_commands, state->pre_buffer,
                                state->pre_buffer_handler, text);
  else
    qbox_state_append_textblock(&state->post_commands, state->post_buffer,
                                state->post_buffer_handler, text);

  msg = g_strdup_printf("Appended Qbox template: %s\n",
                        gtk_button_get_label(GTK_BUTTON(w)));
  gui_text_show(INFO, msg);
  g_free(msg);
}

static GtkWidget *qbox_template_button_new(const gchar *label, const gchar *text,
                                           gint target, gpointer dialog, GtkWidget *box)
{
  GtkWidget *button;

  g_return_val_if_fail(label != NULL, NULL);
  g_return_val_if_fail(text != NULL, NULL);
  g_return_val_if_fail(box != NULL, NULL);

  button = gui_button((gchar *) label, qbox_template_append_cb, dialog, box, TT);
  g_object_set_data(G_OBJECT(button), "qbox_template_text", (gpointer) text);
  g_object_set_data(G_OBJECT(button), "qbox_template_target", GINT_TO_POINTER(target));

  return(button);
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
  gchar *dot;

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
  g_free(job->xyz_path);

  if (job->auto_convert_xyz && (!job->xyz_name || !strlen(job->xyz_name)))
    {
    gchar *base;

    if (job->log_name && strlen(job->log_name))
      base = g_strdup(job->log_name);
    else if (job->input_name && strlen(job->input_name))
      base = g_strdup(job->input_name);
    else
      base = g_strdup("qbox");

    dot = g_strrstr(base, ".");
    if (dot)
      *dot = '\0';
    if (!strlen(base))
      {
      g_free(base);
      base = g_strdup("qbox");
      }
    g_free(job->xyz_name);
    job->xyz_name = g_strdup_printf("%s.xyz", base);
    g_free(base);
    }

  job->input_path = g_build_filename(job->workdir, job->input_name, NULL);
  job->xml_path = g_build_filename(job->workdir, job->xml_name, NULL);
  job->log_path = g_build_filename(job->workdir, job->log_name, NULL);
  if (job->xyz_name && strlen(job->xyz_name))
    job->xyz_path = g_build_filename(job->workdir, job->xyz_name, NULL);
  else
    job->xyz_path = NULL;

  return(0);
}

static gchar *qbox_find_xyz_converter(void)
{
  gchar *path;
  gchar *project_root;

  path = g_build_filename(sysenv.cwd, "qbox-out-to-xyz.sh", NULL);
  if (g_file_test(path, G_FILE_TEST_IS_EXECUTABLE))
    return(path);
  g_free(path);

  if (sysenv.gdis_path && strlen(sysenv.gdis_path))
    {
    project_root = g_path_get_dirname(sysenv.gdis_path);
    path = g_build_filename(project_root, "qbox-out-to-xyz.sh", NULL);
    g_free(project_root);
    if (g_file_test(path, G_FILE_TEST_IS_EXECUTABLE))
      return(path);
    g_free(path);
    }

  path = g_find_program_in_path("qbox-out-to-xyz.sh");
  if (path && g_file_test(path, G_FILE_TEST_IS_EXECUTABLE))
    return(path);
  g_free(path);

  return(NULL);
}

static gchar *qbox_convert_log_to_xyz(struct qbox_task_pak *job, gboolean *converted_ok)
{
  gchar *converter;
  gchar *argv[4];
  gchar *stdout_text = NULL;
  gchar *stderr_text = NULL;
  gchar *stderr_trim = NULL;
  gchar *note;
  GError *error = NULL;
  gint status = 0;

  if (converted_ok)
    *converted_ok = FALSE;

  g_return_val_if_fail(job != NULL, NULL);

  if (!job->auto_convert_xyz)
    return(NULL);

  if (!job->log_path || !g_file_test(job->log_path, G_FILE_TEST_EXISTS))
    return(g_strdup("XYZ conversion skipped: Qbox log file was not found.\n"));

  if (!job->xyz_path || !strlen(job->xyz_path))
    return(g_strdup("XYZ conversion skipped: XYZ trajectory filename is empty.\n"));

  converter = qbox_find_xyz_converter();
  if (!converter)
    return(g_strdup("XYZ conversion skipped: qbox-out-to-xyz.sh was not found.\n"));

  argv[0] = converter;
  argv[1] = job->log_path;
  argv[2] = job->xyz_path;
  argv[3] = NULL;

  if (g_spawn_sync(NULL, argv, NULL, 0, NULL, NULL,
                   &stdout_text, &stderr_text, &status, &error))
    {
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0 && g_file_test(job->xyz_path, G_FILE_TEST_EXISTS))
      {
      note = g_strdup_printf("XYZ trajectory:\n%s\n", job->xyz_path);
      if (converted_ok)
        *converted_ok = TRUE;
      }
    else
      {
      if (stderr_text && strlen(stderr_text))
        {
        stderr_trim = g_strdup(stderr_text);
        g_strstrip(stderr_trim);
        }
      note = g_strdup_printf("XYZ conversion failed for:\n%s\n%s%s%s",
                             job->log_path,
                             (stderr_trim && strlen(stderr_trim)) ? "Details:\n" : "",
                             (stderr_trim && strlen(stderr_trim)) ? stderr_trim : "",
                             (stderr_trim && strlen(stderr_trim)) ? "\n" : "");
      }
    }
  else
    {
    note = g_strdup_printf("XYZ conversion failed: unable to run converter.\n%s\n",
                           error ? error->message : "(unknown error)");
    }

  g_free(converter);
  g_free(stdout_text);
  g_free(stderr_text);
  g_free(stderr_trim);
  g_clear_error(&error);

  return(note);
}

static struct model_pak *qbox_find_loaded_model_by_path(const gchar *path)
{
  GSList *item;
  gchar *target;

  if (!path || !strlen(path))
    return(NULL);

  target = g_canonicalize_filename(path, NULL);
  if (!target)
    return(NULL);

  for (item=sysenv.mal ; item ; item=g_slist_next(item))
    {
    struct model_pak *model;
    gchar *candidate;

    model = item->data;
    if (!model || !strlen(model->filename))
      continue;

    candidate = g_canonicalize_filename(model->filename, NULL);
    if (candidate && g_strcmp0(candidate, target) == 0)
      {
      g_free(candidate);
      g_free(target);
      return(model);
      }
    g_free(candidate);
    }

  g_free(target);
  return(NULL);
}

static void qbox_remove_loaded_models_by_path(const gchar *path)
{
  struct model_pak *loaded;

  while ((loaded = qbox_find_loaded_model_by_path(path)))
    {
    tree_select_model(loaded);
    tree_select_delete();
    }
}

static struct model_pak *qbox_load_output_model(const gchar *path)
{
  struct model_pak *loaded;

  if (!path || !strlen(path))
    return(NULL);

  qbox_remove_loaded_models_by_path(path);
  file_load((gchar *) path, NULL);

  loaded = sysenv.active_model;
  if (loaded)
    {
    tree_select_model(loaded);
    gui_model_select(loaded);
    }

  return(loaded);
}

static gboolean qbox_open_xyz_animation_idle(gpointer data)
{
  struct model_pak *xyz_model = data;

  if (!xyz_model)
    return(FALSE);
  if (!g_slist_find(sysenv.mal, xyz_model))
    return(FALSE);
  if (!xyz_model->animation || xyz_model->num_frames < 2)
    return(FALSE);

  dialog_destroy_single(ANIM, xyz_model);
  tree_select_model(xyz_model);
  gui_model_select(xyz_model);
  gui_animate_dialog();

  return(FALSE);
}

static void qbox_maybe_open_xyz_animation(struct qbox_task_pak *job, struct model_pak *xyz_model)
{
  g_return_if_fail(job != NULL);

  if (!job->open_animation_after_xyz)
    return;
  if (!xyz_model)
    return;
  if (!xyz_model->animation || xyz_model->num_frames < 2)
    return;

  g_idle_add(qbox_open_xyz_animation_idle, xyz_model);
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
    gint nspin_value;
    gint delta_spin_value;
    gint net_charge_value;
    gint nempty_value;
    gint charge_mix_ndim_value;

    fprintf(dest, "set ecut %.1f\n", job->ecut);
    fprintf(dest, "set xc %s\n", job->xc && strlen(job->xc) ? job->xc : "PBE");
    fprintf(dest, "set scf_tol %s\n",
            job->scf_tol && strlen(job->scf_tol) ? job->scf_tol : "1e-3");
    if (job->randomize_wf)
      fprintf(dest, "randomize_wf\n");
    fprintf(dest, "set wf_dyn %s\n", job->wf_dyn && strlen(job->wf_dyn) ? job->wf_dyn : "PSDA");
    if (job->use_nspin)
      {
      nspin_value = (gint) job->nspin;
      if (nspin_value < 1)
        nspin_value = 1;
      if (nspin_value > 2)
        nspin_value = 2;
      fprintf(dest, "set nspin %d\n", nspin_value);
      }
    if (job->use_delta_spin)
      {
      delta_spin_value = (gint) job->delta_spin;
      if (delta_spin_value < 0)
        delta_spin_value = 0;
      fprintf(dest, "set delta_spin %d\n", delta_spin_value);
      }
    if (job->use_net_charge)
      {
      net_charge_value = (gint) job->net_charge;
      fprintf(dest, "set net_charge %d\n", net_charge_value);
      }
    if (job->use_nempty)
      {
      nempty_value = (gint) job->nempty;
      if (nempty_value < 0)
        nempty_value = 0;
      fprintf(dest, "set nempty %d\n", nempty_value);
      }
    if (job->use_fermi_temp)
      fprintf(dest, "set fermi_temp %.6g\n", job->fermi_temp);
    if (job->use_charge_mix_coeff)
      fprintf(dest, "set charge_mix_coeff %.6g\n", job->charge_mix_coeff);
    if (job->use_charge_mix_ndim)
      {
      charge_mix_ndim_value = (gint) job->charge_mix_ndim;
      if (charge_mix_ndim_value < 0)
        charge_mix_ndim_value = 0;
      fprintf(dest, "set charge_mix_ndim %d\n", charge_mix_ndim_value);
      }
    if (job->use_charge_mix_rcut)
      fprintf(dest, "set charge_mix_rcut %.6g\n", job->charge_mix_rcut);
    if (job->use_force_tol)
      fprintf(dest, "set force_tol %.6g\n", job->force_tol);
    if (job->use_ref_cell && job->ref_cell && strlen(job->ref_cell))
      fprintf(dest, "set ref_cell %s\n", job->ref_cell);
    if (job->use_vext && job->vext && strlen(job->vext))
      fprintf(dest, "set vext %s\n", job->vext);
    if (job->use_e_field && job->e_field && strlen(job->e_field))
      fprintf(dest, "set e_field %s\n", job->e_field);
    if (job->use_polarization && job->polarization && strlen(job->polarization))
      fprintf(dest, "set polarization %s\n", job->polarization);
    if (job->use_lock_cm)
      fprintf(dest, "set lock_cm ON\n");
    if (job->use_cell_lock && job->cell_lock && strlen(job->cell_lock))
      fprintf(dest, "set cell_lock %s\n", job->cell_lock);
    if (job->use_cell_mass)
      fprintf(dest, "set cell_mass %.6g\n", job->cell_mass);
    if (job->use_cell_dyn && job->cell_dyn && strlen(job->cell_dyn))
      fprintf(dest, "set cell_dyn %s\n", job->cell_dyn);
    if (job->use_stress)
      fprintf(dest, "set stress ON\n");
    if (job->use_ext_stress && job->ext_stress && strlen(job->ext_stress))
      fprintf(dest, "set ext_stress %s\n", job->ext_stress);
    if (job->use_stress_tol)
      fprintf(dest, "set stress_tol %.6g\n", job->stress_tol);
    if (job->use_thermostat && job->thermostat && strlen(job->thermostat))
      fprintf(dest, "set thermostat %s\n", job->thermostat);
    if (job->use_th_temp)
      fprintf(dest, "set th_temp %.6g\n", job->th_temp);
    if (job->use_th_time)
      fprintf(dest, "set th_time %.6g\n", job->th_time);
    if (job->use_th_width)
      fprintf(dest, "set th_width %.6g\n", job->th_width);
    fprintf(dest, "set ecutprec %.1f\n", job->ecutprec);
    if (job->use_atoms_dyn && job->atoms_dyn && strlen(job->atoms_dyn))
      fprintf(dest, "set atoms_dyn %s\n", job->atoms_dyn);
    if (job->use_emass)
      fprintf(dest, "set emass %.6g\n", job->emass);
    if (job->use_dt)
      fprintf(dest, "set dt %.6g\n", job->dt);
    if (job->use_randomize_v)
      fprintf(dest, "randomize_v %.6g\n", job->randomize_v);
    if (job->use_iter_cmd && job->iter_cmd && strlen(job->iter_cmd))
      fprintf(dest, "set iter_cmd %s\n", job->iter_cmd);
    if (job->use_iter_cmd_period)
      {
      gint iter_cmd_period_value = (gint) job->iter_cmd_period;
      if (iter_cmd_period_value < 1)
        iter_cmd_period_value = 1;
      fprintf(dest, "set iter_cmd_period %d\n", iter_cmd_period_value);
      }
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
  gchar *base_cmd;
  gchar *qbox_quoted;
  gchar *mpirun_quoted = NULL;
  gchar *input_quoted;
  gchar *log_quoted;
  gchar *timeout_quoted = NULL;
  gchar *timeout_value;
  gboolean use_timeout = FALSE;
  gchar *timeout_path;
  gchar *mpirun_path = NULL;
  gint mpi_ranks;
  gint omp_threads;

  g_return_if_fail(job != NULL);
  g_return_if_fail(task != NULL);

  if (qbox_prepare_paths(job))
    return;

  if (qbox_write_runtime_input(job))
    return;

  qbox_quoted = g_shell_quote(job->qbox_path);
  input_quoted = g_shell_quote(job->input_path);
  log_quoted = g_shell_quote(job->log_path);
  mpi_ranks = qbox_resource_count(job->mpi_ranks);
  omp_threads = qbox_resource_count(job->omp_threads);

  if (job->use_mpi)
    {
    mpirun_path = qbox_resolve_executable(job->mpirun_path);
    if (!mpirun_path)
      {
      job->error = g_strdup_printf("MPI launcher was requested, but mpirun was not found:\n%s\n"
                                   "Set a valid mpirun path in View > Executable paths or in the Qbox dialog.\n",
                                   job->mpirun_path ? job->mpirun_path : "(empty)");
      g_free(qbox_quoted);
      g_free(input_quoted);
      g_free(log_quoted);
      return;
      }
    mpirun_quoted = g_shell_quote(mpirun_path);
    }

  timeout_value = g_strdup(job->run_timeout ? job->run_timeout : "");
  g_strstrip(timeout_value);
  if (strlen(timeout_value) &&
      g_ascii_strcasecmp(timeout_value, "0") != 0 &&
      g_ascii_strcasecmp(timeout_value, "off") != 0 &&
      g_ascii_strcasecmp(timeout_value, "none") != 0)
    {
    timeout_path = g_find_program_in_path("timeout");
    if (timeout_path)
      {
      use_timeout = TRUE;
      timeout_quoted = g_shell_quote(timeout_value);
      g_free(timeout_path);
      }
    }

  if (job->use_mpi)
    base_cmd = g_strdup_printf("%s -np %d %s < %s > %s 2>&1",
                               mpirun_quoted, mpi_ranks, qbox_quoted, input_quoted, log_quoted);
  else
    base_cmd = g_strdup_printf("%s < %s > %s 2>&1",
                               qbox_quoted, input_quoted, log_quoted);

  if (use_timeout)
    cmd = g_strdup_printf("env OMP_NUM_THREADS=%d OPENBLAS_NUM_THREADS=%d MKL_NUM_THREADS=%d BLIS_NUM_THREADS=%d timeout %s %s",
                          omp_threads, omp_threads, omp_threads, omp_threads,
                          timeout_quoted, base_cmd);
  else
    cmd = g_strdup_printf("env OMP_NUM_THREADS=%d OPENBLAS_NUM_THREADS=%d MKL_NUM_THREADS=%d BLIS_NUM_THREADS=%d %s",
                          omp_threads, omp_threads, omp_threads, omp_threads, base_cmd);

  task->status_file = g_strdup(job->log_path);
  task->is_async = TRUE;
  if (!task_async(cmd, &(task->pid)))
    {
    task->is_async = FALSE;
    g_free(job->error);
    job->error = g_strdup_printf("Unable to launch Qbox with:\n%s\n", job->qbox_path);
    }

  g_free(cmd);
  g_free(base_cmd);
  g_free(mpirun_path);
  g_free(mpirun_quoted);
  g_free(timeout_value);
  g_free(timeout_quoted);
  g_free(qbox_quoted);
  g_free(input_quoted);
  g_free(log_quoted);
}

static void cleanup_qbox_task(gpointer ptr)
{
  struct qbox_task_pak *job = ptr;
  gchar *text;
  gchar *xyz_note = NULL;
  gboolean xyz_ready = FALSE;
  gboolean load_xyz_now = FALSE;
  struct model_pak *xyz_model = NULL;

  g_return_if_fail(job != NULL);

  if (job->error)
    {
    gui_text_show(ERROR, job->error);
    qbox_task_free(job);
    return;
    }

  xyz_note = qbox_convert_log_to_xyz(job, &xyz_ready);
  load_xyz_now = xyz_ready &&
                 (job->load_xyz_after_convert || job->open_animation_after_xyz);

  if (!job->xml_path || !g_file_test(job->xml_path, G_FILE_TEST_EXISTS))
    {
    if (job->load_saved_xml)
      {
      text = g_strdup_printf("Qbox finished, but no saved XML file was found:\n%s\n%s",
                             job->xml_path ? job->xml_path : "(unknown)",
                             xyz_note ? xyz_note : "");
      gui_text_show(ERROR, text);
      g_free(text);
      if (load_xyz_now)
        {
        xyz_model = qbox_load_output_model(job->xyz_path);
        }
      qbox_maybe_open_xyz_animation(job, xyz_model);
      g_free(xyz_note);
      qbox_task_free(job);
      return;
      }

    text = g_strdup_printf("Qbox finished.\nLog:\n%s\n(no XML file detected)\n%s",
                           job->log_path ? job->log_path : "(unknown)",
                           xyz_note ? xyz_note : "");
    gui_text_show(STANDARD, text);
    g_free(text);
    if (load_xyz_now)
      {
      xyz_model = qbox_load_output_model(job->xyz_path);
      }
    qbox_maybe_open_xyz_animation(job, xyz_model);
    g_free(xyz_note);
    qbox_task_free(job);
    return;
    }

  text = g_strdup_printf("Qbox saved:\n%s\nLog:\n%s\n%s",
                         job->xml_path,
                         job->log_path ? job->log_path : "(unknown)",
                         xyz_note ? xyz_note : "");
  gui_text_show(STANDARD, text);
  g_free(text);

  if (job->load_saved_xml)
    qbox_load_output_model(job->xml_path);
  if (load_xyz_now)
    {
    xyz_model = qbox_load_output_model(job->xyz_path);
    }
  qbox_maybe_open_xyz_animation(job, xyz_model);

  g_free(xyz_note);
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
  job->xyz_name = g_strdup(state->xyz_name);
  job->mpirun_path = g_strdup(state->mpirun_path);
  job->xc = g_strdup(state->xc);
  job->scf_tol = g_strdup(state->scf_tol);
  job->wf_dyn = g_strdup(state->wf_dyn);
  job->cell_lock = g_strdup(state->cell_lock);
  job->ext_stress = g_strdup(state->ext_stress);
  job->ref_cell = g_strdup(state->ref_cell);
  job->vext = g_strdup(state->vext);
  job->e_field = g_strdup(state->e_field);
  job->polarization = g_strdup(state->polarization);
  job->iter_cmd = g_strdup(state->iter_cmd);
  job->atoms_dyn = g_strdup(state->atoms_dyn);
  job->cell_dyn = g_strdup(state->cell_dyn);
  job->thermostat = g_strdup(state->thermostat);
  job->run_timeout = g_strdup(state->run_timeout);
  job->ecut = state->ecut;
  job->ecutprec = state->ecutprec;
  job->dt = state->dt;
  job->fermi_temp = state->fermi_temp;
  job->charge_mix_coeff = state->charge_mix_coeff;
  job->charge_mix_ndim = state->charge_mix_ndim;
  job->charge_mix_rcut = state->charge_mix_rcut;
  job->cell_mass = state->cell_mass;
  job->emass = state->emass;
  job->randomize_v = state->randomize_v;
  job->force_tol = state->force_tol;
  job->stress_tol = state->stress_tol;
  job->th_temp = state->th_temp;
  job->th_time = state->th_time;
  job->th_width = state->th_width;
  job->mpi_ranks = state->mpi_ranks;
  job->omp_threads = state->omp_threads;
  job->randomize_wf = state->randomize_wf;
  job->nempty = state->nempty;
  job->nspin = state->nspin;
  job->delta_spin = state->delta_spin;
  job->net_charge = state->net_charge;
  job->use_mpi = state->use_mpi;
  job->use_nspin = state->use_nspin;
  job->use_delta_spin = state->use_delta_spin;
  job->use_net_charge = state->use_net_charge;
  job->use_atoms_dyn = state->use_atoms_dyn;
  job->use_dt = state->use_dt;
  job->use_fermi_temp = state->use_fermi_temp;
  job->use_charge_mix_coeff = state->use_charge_mix_coeff;
  job->use_charge_mix_ndim = state->use_charge_mix_ndim;
  job->use_charge_mix_rcut = state->use_charge_mix_rcut;
  job->use_randomize_v = state->use_randomize_v;
  job->use_nempty = state->use_nempty;
  job->use_force_tol = state->use_force_tol;
  job->use_ref_cell = state->use_ref_cell;
  job->use_vext = state->use_vext;
  job->use_e_field = state->use_e_field;
  job->use_polarization = state->use_polarization;
  job->use_iter_cmd = state->use_iter_cmd;
  job->use_iter_cmd_period = state->use_iter_cmd_period;
  job->use_lock_cm = state->use_lock_cm;
  job->use_cell_lock = state->use_cell_lock;
  job->use_cell_mass = state->use_cell_mass;
  job->use_cell_dyn = state->use_cell_dyn;
  job->use_stress = state->use_stress;
  job->use_ext_stress = state->use_ext_stress;
  job->use_stress_tol = state->use_stress_tol;
  job->use_emass = state->use_emass;
  job->use_thermostat = state->use_thermostat;
  job->use_th_temp = state->use_th_temp;
  job->use_th_time = state->use_th_time;
  job->use_th_width = state->use_th_width;
  job->iter_cmd_period = state->iter_cmd_period;
  job->auto_convert_xyz = state->auto_convert_xyz;
  job->load_xyz_after_convert = state->load_xyz_after_convert;
  job->open_animation_after_xyz = state->open_animation_after_xyz;
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
  GSList *missing = NULL;
  gint total = 0;
  gint applied = 0;

  (void) w;

  g_return_if_fail(dialog != NULL);

  state = dialog_child_get(dialog, "qbox_state");
  if (!state)
    return;

  for (item=state->species ; item ; item=g_slist_next(item))
    {
    struct qbox_species_pak *species = item->data;
    gchar *path;

    total++;
    path = qbox_demo_potential_path(species->symbol);
    qbox_string_replace(&species->path, path);
    if (g_getenv("GDIS_QBOX_DEBUG"))
      {
      fprintf(stderr, "[qbox-ui] autofill species=%s path=%s\n",
              species->symbol, path ? path : "(none)");
      fflush(stderr);
      }
    if (path && strlen(path))
      applied++;
    else
      missing = qbox_symbol_list_add_unique(missing, species->symbol);
    g_free(path);
    }

  if (total == 0)
    {
    gui_text_show(INFO, "No element species found in the active model for Qbox potentials.\n");
    return;
    }

  if (applied == total)
    {
    gchar *msg;

    msg = g_strdup_printf("Applied demo Qbox potentials for all %d element species.\n", total);
    gui_text_show(INFO, msg);
    g_free(msg);
    }
  else
    {
    gchar *symbols;
    gchar *msg;

    symbols = qbox_symbol_list_string(missing);
    msg = g_strdup_printf("Applied demo Qbox potentials for %d/%d species. Missing bundled demo files for: %s\n",
                          applied, total, symbols ? symbols : "unknown");
    gui_text_show(INFO, msg);
    g_free(symbols);
    g_free(msg);
    }

  qbox_symbol_list_free(missing);
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
  gint mpi_ranks;
  gint omp_threads;

  (void) w;

  g_return_if_fail(dialog != NULL);

  if (!qbox_refresh_executable_path())
    {
    gui_text_show(ERROR,
                  "Qbox executable was not found.\n"
                  "Checked configured path plus repo/system defaults: bin/qbox, bin/qb, qbox, qb.\n"
                  "Run ./install-qbox-local.sh in this tree or set it in View > Executable paths...\n");
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

  mpi_ranks = qbox_resource_count(job->mpi_ranks);
  omp_threads = qbox_resource_count(job->omp_threads);
  text = g_strdup_printf("Queued Qbox job for model: %s\n",
                         job->model->basename ? job->model->basename : "model");
  if (job->use_mpi)
    {
    gchar *tmp;

    tmp = g_strdup_printf("%sMPI ranks: %d\nOpenMP threads: %d\n",
                          text, mpi_ranks, omp_threads);
    g_free(text);
    text = tmp;
    }
  else
    {
    gchar *tmp;

    tmp = g_strdup_printf("%sMode: serial\nOpenMP threads: %d\n",
                          text, omp_threads);
    g_free(text);
    text = tmp;
    }
  gui_text_show(INFO, text);
  g_free(text);

  task_new("Qbox", &exec_qbox_task, job, &cleanup_qbox_task, job, job->model);
}

static GtkWidget *qbox_setup_section_new(GtkWidget *parent, const gchar *title)
{
  GtkWidget *frame;
  GtkWidget *vbox;

  g_return_val_if_fail(parent != NULL, NULL);
  g_return_val_if_fail(title != NULL, NULL);

  frame = gtk_frame_new(title);
  gtk_box_pack_start(GTK_BOX(parent), frame, FALSE, FALSE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(frame), PANEL_SPACING);

  vbox = gtk_vbox_new(FALSE, PANEL_SPACING);
  gtk_container_add(GTK_CONTAINER(frame), vbox);

  return(vbox);
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
  GtkWidget *actions;
  GtkWidget *button;
  GtkWidget *window;
  GtkWidget *notebook;
  GtkWidget *page;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *run_box;
  GtkWidget *setup_box;
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

  qbox_refresh_executable_path();

  window = dialog_window(dialog);
  gtk_window_set_default_size(GTK_WINDOW(window), 900, 720);
  actions = GDIS_DIALOG_ACTIONS(window);
  gtk_box_set_spacing(GTK_BOX(actions), 4);
#if GTK_MAJOR_VERSION >= 4
  qbox_compact_action_row(actions);
#endif

  state = g_malloc0(sizeof(struct qbox_gui_pak));
  state->model = model;
  state->workdir = qbox_default_workdir();
  stem = qbox_model_stem(model);
  state->input_name = g_strdup_printf("%s.in", stem);
  state->xml_name = g_strdup_printf("%s.xml", stem);
  state->log_name = g_strdup_printf("%s.out", stem);
  state->xyz_name = g_strdup_printf("%s.xyz", stem);
  state->mpirun_path = g_strdup(sysenv.mpirun_path ? sysenv.mpirun_path :
                                (sysenv.mpirun_exe ? sysenv.mpirun_exe : "mpirun"));
  state->xc = g_strdup("PBE");
  state->scf_tol = g_strdup("1e-3");
  state->wf_dyn = g_strdup("PSDA");
  state->cell_lock = g_strdup("OFF");
  state->ext_stress = g_strdup("0 0 0 0 0 0");
  state->ref_cell = g_strdup(NULL);
  state->vext = g_strdup(NULL);
  state->e_field = g_strdup("0 0 0");
  state->polarization = g_strdup("OFF");
  state->iter_cmd = g_strdup(NULL);
  state->atoms_dyn = g_strdup("CG");
  state->cell_dyn = g_strdup("SD");
  state->thermostat = g_strdup("SCALING");
  state->run_timeout = g_strdup("45s");
  state->ecut = 15.0;
  state->ecutprec = 5.0;
  state->dt = 10.0;
  state->fermi_temp = 300.0;
  state->charge_mix_coeff = 0.5;
  state->charge_mix_ndim = 8.0;
  state->charge_mix_rcut = 0.0;
  state->cell_mass = 500.0;
  state->emass = 400.0;
  state->randomize_v = 400.0;
  state->force_tol = 1.0e-4;
  state->stress_tol = 1.0e-5;
  state->th_temp = 300.0;
  state->th_time = 100.0;
  state->th_width = 10.0;
  state->mpi_ranks = 1.0;
  state->omp_threads = 1.0;
  state->randomize_wf = TRUE;
  state->nempty = 1;
  state->nspin = 1.0;
  state->delta_spin = 0.0;
  state->net_charge = 0.0;
  state->use_mpi = FALSE;
  state->use_nspin = FALSE;
  state->use_delta_spin = FALSE;
  state->use_net_charge = FALSE;
  state->use_atoms_dyn = TRUE;
  state->use_dt = FALSE;
  state->use_fermi_temp = FALSE;
  state->use_charge_mix_coeff = FALSE;
  state->use_charge_mix_ndim = FALSE;
  state->use_charge_mix_rcut = FALSE;
  state->use_randomize_v = FALSE;
  state->use_nempty = FALSE;
  state->use_force_tol = FALSE;
  state->use_ref_cell = FALSE;
  state->use_vext = FALSE;
  state->use_e_field = FALSE;
  state->use_polarization = FALSE;
  state->use_iter_cmd = FALSE;
  state->use_iter_cmd_period = FALSE;
  state->use_lock_cm = FALSE;
  state->use_cell_lock = FALSE;
  state->use_cell_mass = FALSE;
  state->use_cell_dyn = FALSE;
  state->use_stress = FALSE;
  state->use_ext_stress = FALSE;
  state->use_stress_tol = FALSE;
  state->use_emass = FALSE;
  state->use_thermostat = FALSE;
  state->use_th_temp = FALSE;
  state->use_th_time = FALSE;
  state->use_th_width = FALSE;
  state->iter_cmd_period = 1.0;
  state->auto_convert_xyz = TRUE;
  state->load_xyz_after_convert = TRUE;
  state->open_animation_after_xyz = TRUE;
  state->load_saved_xml = TRUE;
  state->write_model_block = TRUE;
  state->write_default_block = TRUE;
  state->auto_save_xml_cmd = TRUE;
  state->auto_quit = TRUE;
  state->run_cmd = g_strdup("run 10 3");
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
  swin = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
                                 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start(GTK_BOX(page), swin, TRUE, TRUE, 0);

  setup_box = gtk_vbox_new(FALSE, PANEL_SPACING);
  gtk_container_set_border_width(GTK_CONTAINER(setup_box), PANEL_SPACING);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(swin), setup_box);

  frame = gtk_frame_new("Model");
  gtk_box_pack_start(GTK_BOX(setup_box), frame, FALSE, FALSE, 0);
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
  gtk_box_pack_start(GTK_BOX(setup_box), frame, FALSE, FALSE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(frame), PANEL_SPACING);
  vbox = gtk_vbox_new(FALSE, PANEL_SPACING);
  gtk_container_add(GTK_CONTAINER(frame), vbox);

  gui_text_entry("Work dir", &state->workdir, TRUE, TRUE, vbox);
  gui_text_entry("Input", &state->input_name, TRUE, TRUE, vbox);
  gui_text_entry("Saved XML", &state->xml_name, TRUE, TRUE, vbox);
  gui_text_entry("Log", &state->log_name, TRUE, TRUE, vbox);
  gui_text_entry("Trajectory XYZ", &state->xyz_name, TRUE, TRUE, vbox);

  frame = gtk_frame_new("Run Settings");
  gtk_box_pack_start(GTK_BOX(setup_box), frame, FALSE, FALSE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(frame), PANEL_SPACING);
  run_box = gtk_vbox_new(FALSE, PANEL_SPACING);
  gtk_container_add(GTK_CONTAINER(frame), run_box);

  vbox = qbox_setup_section_new(run_box, "Resources");
  gui_direct_check("Use MPI launcher", &state->use_mpi, NULL, NULL, vbox);
  gui_text_entry("MPI launcher", &state->mpirun_path, TRUE, TRUE, vbox);
  gui_direct_spin("MPI ranks", &state->mpi_ranks, 1.0, 512.0, 1.0, NULL, NULL, vbox);
  gui_direct_spin("OpenMP threads", &state->omp_threads, 1.0, 256.0, 1.0, NULL, NULL, vbox);
  state->entry_run_timeout = gui_text_entry("Run timeout", &state->run_timeout, TRUE, TRUE, vbox);
  state->entry_run_cmd = gui_text_entry("Default run command", &state->run_cmd, TRUE, TRUE, vbox);

  vbox = qbox_setup_section_new(run_box, "Electronic");
  gui_direct_spin("Ecut", &state->ecut, 5.0, 300.0, 5.0, NULL, NULL, vbox);
  gui_direct_spin("Ecut precision", &state->ecutprec, 1.0, 40.0, 1.0, NULL, NULL, vbox);
  state->entry_xc = gui_text_entry("XC", &state->xc, TRUE, TRUE, vbox);
  state->entry_scf_tol = gui_text_entry("SCF tol", &state->scf_tol, TRUE, TRUE, vbox);
  state->entry_wf_dyn = gui_text_entry("WF dyn", &state->wf_dyn, TRUE, TRUE, vbox);
  state->check_randomize_wf = gui_direct_check("Randomize wavefunction", &state->randomize_wf,
                                               NULL, NULL, vbox);
  state->check_use_nspin = gui_direct_check("Add set nspin", &state->use_nspin,
                                            NULL, NULL, vbox);
  gui_direct_spin("nspin", &state->nspin, 1.0, 2.0, 1.0, NULL, NULL, vbox);
  state->check_use_delta_spin = gui_direct_check("Add set delta_spin", &state->use_delta_spin,
                                                 NULL, NULL, vbox);
  gui_direct_spin("delta_spin", &state->delta_spin, 0.0, 64.0, 1.0, NULL, NULL, vbox);
  state->check_use_net_charge = gui_direct_check("Add set net_charge", &state->use_net_charge,
                                                 NULL, NULL, vbox);
  gui_direct_spin("net_charge", &state->net_charge, -20.0, 20.0, 1.0, NULL, NULL, vbox);
  state->check_use_nempty = gui_direct_check("Add set nempty", &state->use_nempty,
                                             NULL, NULL, vbox);
  gui_direct_spin("nempty", &state->nempty, 0.0, 200.0, 1.0, NULL, NULL, vbox);
  state->check_use_fermi_temp = gui_direct_check("Add set fermi_temp", &state->use_fermi_temp,
                                                 NULL, NULL, vbox);
  gui_direct_spin("fermi_temp", &state->fermi_temp, 0.0, 5000.0, 10.0, NULL, NULL, vbox);
  state->check_use_polarization = gui_direct_check("Add set polarization", &state->use_polarization,
                                                   NULL, NULL, vbox);
  state->entry_polarization = gui_text_entry("polarization value", &state->polarization, TRUE, TRUE, vbox);
  state->check_use_e_field = gui_direct_check("Add set e_field", &state->use_e_field,
                                              NULL, NULL, vbox);
  state->entry_e_field = gui_text_entry("e_field value", &state->e_field, TRUE, TRUE, vbox);
  state->check_use_charge_mix_coeff = gui_direct_check("Add set charge_mix_coeff", &state->use_charge_mix_coeff,
                                                       NULL, NULL, vbox);
  gui_direct_spin("charge_mix_coeff", &state->charge_mix_coeff, 0.0, 1.0, 0.05, NULL, NULL, vbox);
  state->check_use_charge_mix_ndim = gui_direct_check("Add set charge_mix_ndim", &state->use_charge_mix_ndim,
                                                      NULL, NULL, vbox);
  gui_direct_spin("charge_mix_ndim", &state->charge_mix_ndim, 0.0, 40.0, 1.0, NULL, NULL, vbox);
  state->check_use_charge_mix_rcut = gui_direct_check("Add set charge_mix_rcut", &state->use_charge_mix_rcut,
                                                      NULL, NULL, vbox);
  gui_direct_spin("charge_mix_rcut", &state->charge_mix_rcut, 0.0, 50.0, 0.25, NULL, NULL, vbox);
  state->check_use_emass = gui_direct_check("Add set emass", &state->use_emass,
                                            NULL, NULL, vbox);
  gui_direct_spin("emass", &state->emass, 0.0, 5000.0, 10.0, NULL, NULL, vbox);

  vbox = qbox_setup_section_new(run_box, "Cell And Stress");
  state->check_use_ref_cell = gui_direct_check("Add set ref_cell", &state->use_ref_cell,
                                               NULL, NULL, vbox);
  state->entry_ref_cell = gui_text_entry("ref_cell value", &state->ref_cell, TRUE, TRUE, vbox);
  state->check_use_vext = gui_direct_check("Add set vext", &state->use_vext,
                                           NULL, NULL, vbox);
  state->entry_vext = gui_text_entry("vext value", &state->vext, TRUE, TRUE, vbox);
  state->check_use_lock_cm = gui_direct_check("Add set lock_cm ON", &state->use_lock_cm,
                                              NULL, NULL, vbox);
  state->check_use_cell_lock = gui_direct_check("Add set cell_lock", &state->use_cell_lock,
                                                NULL, NULL, vbox);
  state->entry_cell_lock = gui_text_entry("cell_lock value", &state->cell_lock, TRUE, TRUE, vbox);
  state->check_use_cell_mass = gui_direct_check("Add set cell_mass", &state->use_cell_mass,
                                                NULL, NULL, vbox);
  gui_direct_spin("cell_mass", &state->cell_mass, 0.0, 100000.0, 10.0, NULL, NULL, vbox);
  state->check_use_cell_dyn = gui_direct_check("Add set cell_dyn", &state->use_cell_dyn,
                                               NULL, NULL, vbox);
  state->entry_cell_dyn = gui_text_entry("cell_dyn value", &state->cell_dyn, TRUE, TRUE, vbox);
  state->check_use_stress = gui_direct_check("Add set stress ON", &state->use_stress,
                                             NULL, NULL, vbox);
  state->check_use_ext_stress = gui_direct_check("Add set ext_stress", &state->use_ext_stress,
                                                 NULL, NULL, vbox);
  state->entry_ext_stress = gui_text_entry("ext_stress value", &state->ext_stress, TRUE, TRUE, vbox);
  state->check_use_stress_tol = gui_direct_check("Add set stress_tol", &state->use_stress_tol,
                                                 NULL, NULL, vbox);
  gui_direct_spin("stress_tol", &state->stress_tol, 0.0, 1.0, 0.00001, NULL, NULL, vbox);

  vbox = qbox_setup_section_new(run_box, "Dynamics And Thermostat");
  state->check_use_force_tol = gui_direct_check("Add set force_tol", &state->use_force_tol,
                                                NULL, NULL, vbox);
  gui_direct_spin("force_tol", &state->force_tol, 0.0, 1.0, 0.0001, NULL, NULL, vbox);
  state->check_use_atoms_dyn = gui_direct_check("Add set atoms_dyn", &state->use_atoms_dyn,
                                                NULL, NULL, vbox);
  state->entry_atoms_dyn = gui_text_entry("atoms_dyn value", &state->atoms_dyn, TRUE, TRUE, vbox);
  state->check_use_dt = gui_direct_check("Add set dt", &state->use_dt, NULL, NULL, vbox);
  gui_direct_spin("dt", &state->dt, 0.0, 500.0, 1.0, NULL, NULL, vbox);
  state->check_use_randomize_v = gui_direct_check("Add randomize_v", &state->use_randomize_v,
                                                  NULL, NULL, vbox);
  gui_direct_spin("randomize_v temperature", &state->randomize_v, 0.0, 5000.0, 25.0, NULL, NULL, vbox);
  state->check_use_thermostat = gui_direct_check("Add set thermostat", &state->use_thermostat,
                                                 NULL, NULL, vbox);
  state->entry_thermostat = gui_text_entry("thermostat value", &state->thermostat, TRUE, TRUE, vbox);
  state->check_use_th_temp = gui_direct_check("Add set th_temp", &state->use_th_temp,
                                              NULL, NULL, vbox);
  gui_direct_spin("th_temp", &state->th_temp, 0.0, 5000.0, 10.0, NULL, NULL, vbox);
  state->check_use_th_time = gui_direct_check("Add set th_time", &state->use_th_time,
                                              NULL, NULL, vbox);
  gui_direct_spin("th_time", &state->th_time, 0.0, 5000.0, 10.0, NULL, NULL, vbox);
  state->check_use_th_width = gui_direct_check("Add set th_width", &state->use_th_width,
                                               NULL, NULL, vbox);
  gui_direct_spin("th_width", &state->th_width, 0.0, 1000.0, 1.0, NULL, NULL, vbox);
  state->check_use_iter_cmd = gui_direct_check("Add set iter_cmd", &state->use_iter_cmd,
                                               NULL, NULL, vbox);
  state->entry_iter_cmd = gui_text_entry("iter_cmd value", &state->iter_cmd, TRUE, TRUE, vbox);
  state->check_use_iter_cmd_period = gui_direct_check("Add set iter_cmd_period", &state->use_iter_cmd_period,
                                                      NULL, NULL, vbox);
  gui_direct_spin("iter_cmd_period", &state->iter_cmd_period, 1.0, 1000.0, 1.0, NULL, NULL, vbox);

  vbox = qbox_setup_section_new(run_box, "Output And Reload");
  state->check_auto_convert_xyz = gui_direct_check("Auto convert Log to XYZ after run", &state->auto_convert_xyz,
                                                   NULL, NULL, vbox);
  state->check_load_xyz_after_convert = gui_direct_check("Load XYZ after conversion", &state->load_xyz_after_convert,
                                                         NULL, NULL, vbox);
  state->check_open_animation_after_xyz = gui_direct_check("Open Animation dialog for loaded XYZ", &state->open_animation_after_xyz,
                                                           NULL, NULL, vbox);
  state->check_load_saved_xml = gui_direct_check("Load saved XML after Qbox finishes", &state->load_saved_xml,
                                                 NULL, NULL, vbox);

  label = qbox_note_label_new("Write Input writes a runnable script with model export + defaults."
                              " Setup is grouped into resources, electronic controls, cell/stress,"
                              " and dynamics sections."
                              " It now covers common spin/charge, SCF mixing, cell control,"
                              " iter_cmd hooks, thermostat, and dynamics variables."
                              " Resource controls let you choose serial vs MPI plus OpenMP threads."
                              " Qbox run keeps .out and can auto-convert to .xyz trajectory."
                              " Optionally load the XYZ and open Animation automatically."
                              " Use the free-form command blocks below for less common Qbox commands.");
  gtk_box_pack_start(GTK_BOX(setup_box), label, FALSE, FALSE, 0);

  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page, gtk_label_new("Setup"));

  page = gtk_vbox_new(FALSE, PANEL_SPACING);
  gtk_container_set_border_width(GTK_CONTAINER(page), PANEL_SPACING);

  hbox = gtk_hbox_new(FALSE, PANEL_SPACING);
  gtk_box_pack_start(GTK_BOX(page), hbox, FALSE, FALSE, 0);
  gui_button("Use Demo Potentials", qbox_apply_demo_potentials, dialog, hbox, TT);
  gui_button("Clear Paths", qbox_clear_potentials, dialog, hbox, TT);

  label = qbox_note_label_new("Set one XML pseudopotential path per element in the active model."
                              " Auto-fill searches legacy demo files plus external/pseudos/qbox-xml-oncv-sr,"
                              " then external/pseudos/qbox-xml-oncv."
                              " You can override with GDIS_QBOX_POTENTIAL_DIR.");
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

  frame = gtk_frame_new("Advanced: Input Composition");
  gtk_box_pack_start(GTK_BOX(setup_box), frame, FALSE, FALSE, 0);
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

  frame = gtk_frame_new("Advanced: Command Templates");
  gtk_box_pack_start(GTK_BOX(setup_box), frame, FALSE, FALSE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(frame), PANEL_SPACING);
  vbox = gtk_vbox_new(FALSE, PANEL_SPACING);
  gtk_container_add(GTK_CONTAINER(frame), vbox);

  label = qbox_note_label_new("These buttons append official Qbox command snippets to Post/default-override Commands."
                              " Use them as a starting point, then edit the atom names, file names, or parameters.");
  gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

  hbox = gtk_hbox_new(FALSE, PANEL_SPACING);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
  qbox_template_button_new("Status", "status", QBOX_TEMPLATE_POST, dialog, hbox);
  qbox_template_button_new("List Atoms", "list_atoms", QBOX_TEMPLATE_POST, dialog, hbox);
  qbox_template_button_new("Distance", "distance atom1 atom2", QBOX_TEMPLATE_POST, dialog, hbox);
  qbox_template_button_new("Angle", "angle atom1 atom2 atom3", QBOX_TEMPLATE_POST, dialog, hbox);
  qbox_template_button_new("Torsion", "torsion atom1 atom2 atom3 atom4", QBOX_TEMPLATE_POST, dialog, hbox);

  hbox = gtk_hbox_new(FALSE, PANEL_SPACING);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
  qbox_template_button_new("Move", "move atom1 by 0.05 0 0\nrun 0 10", QBOX_TEMPLATE_POST, dialog, hbox);
  qbox_template_button_new("Constraint", "constraint define distance c1 atom1 atom2 2.0\nconstraint list", QBOX_TEMPLATE_POST, dialog, hbox);
  qbox_template_button_new("Plot XYZ", "plot qbox-frame.xyz", QBOX_TEMPLATE_POST, dialog, hbox);
  qbox_template_button_new("Plot Density", "plot -density density.cube", QBOX_TEMPLATE_POST, dialog, hbox);

  hbox = gtk_hbox_new(FALSE, PANEL_SPACING);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
  qbox_template_button_new("Plot WF", "plot -wf 1 orbital_1.cube", QBOX_TEMPLATE_POST, dialog, hbox);
  qbox_template_button_new("MLWF", "compute_mlwf", QBOX_TEMPLATE_POST, dialog, hbox);
  qbox_template_button_new("Spectrum", "spectrum 0.05 spectrum.dat", QBOX_TEMPLATE_POST, dialog, hbox);
  qbox_template_button_new("Response", "# response usually needs polarization enabled\nresponse 0.001 20 4", QBOX_TEMPLATE_POST, dialog, hbox);

  frame = gtk_frame_new("Advanced: Pre-default Commands");
  gtk_box_pack_start(GTK_BOX(setup_box), frame, TRUE, TRUE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(frame), PANEL_SPACING);
  vbox = gtk_vbox_new(FALSE, PANEL_SPACING);
  gtk_container_add(GTK_CONTAINER(frame), vbox);
  label = qbox_note_label_new("Commands inserted before default settings block.");
  gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
  swin = qbox_text_window_new(&state->pre_commands, &state->pre_buffer, &state->pre_buffer_handler);
  gtk_widget_set_size_request(swin, -1, 110);
  gtk_box_pack_start(GTK_BOX(vbox), swin, TRUE, TRUE, 0);

  frame = gtk_frame_new("Advanced: Post/default-override Commands");
  gtk_box_pack_start(GTK_BOX(setup_box), frame, TRUE, TRUE, 0);
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
  gtk_box_pack_start(GTK_BOX(setup_box), label, FALSE, FALSE, 0);

  button = gui_button("Write Input", qbox_write_input_cb, dialog, actions, FF);
#if GTK_MAJOR_VERSION >= 4
  qbox_compact_action_button(button);
#endif
  button = gui_stock_button(GTK_STOCK_EXECUTE, qbox_run_cb, dialog, actions);
#if GTK_MAJOR_VERSION >= 4
  qbox_compact_action_button(button);
#endif
  button = gui_stock_button(GTK_STOCK_CLOSE, dialog_destroy, dialog, actions);
#if GTK_MAJOR_VERSION >= 4
  qbox_compact_action_button(button);
#endif

  gtk_widget_show_all(window);

/* Debug automation hooks for GTK4 regression scripts. */
  if (g_getenv("GDIS_DEBUG_QBOX_AUTO_FILL"))
    qbox_apply_demo_potentials(NULL, dialog);
  if (g_getenv("GDIS_DEBUG_QBOX_AUTO_WRITE"))
    qbox_write_input_cb(NULL, dialog);
  if (g_getenv("GDIS_DEBUG_QBOX_AUTO_EXECUTE"))
    qbox_run_cb(NULL, dialog);
}
