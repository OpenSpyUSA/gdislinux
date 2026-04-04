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
#include <math.h>

#include "gdis.h"
#include "coords.h"
#include "file.h"
#include "matrix.h"
#include "model.h"
#include "parse.h"
#include "interface.h"

extern struct sysenv_pak sysenv;
extern struct elem_pak elements[];

struct qbox_species_pak
{
  gchar *name;
  gint atom_code;
};

enum
{
  QBOX_XML_NONE,
  QBOX_XML_POSITION,
  QBOX_XML_VELOCITY,
  QBOX_XML_FORCE,
  QBOX_XML_ENERGY
};

static void qbox_species_free(gpointer data)
{
  struct qbox_species_pak *species = data;

  if (!species)
    return;

  g_free(species->name);
  g_free(species);
}

static void qbox_strip_inline_comment(gchar *line)
{
  gchar *hash;

  g_return_if_fail(line != NULL);

  hash = strchr(line, '#');
  if (hash)
    *hash = '\0';
}

static gint qbox_guess_element_from_text(const gchar *text)
{
  gint i, code, num_tokens;
  gchar *tmp;
  gchar **buff;

  if (!text || !strlen(text))
    return(0);

  code = elem_test(text);
  if (code)
    return(code);

  for (i=1 ; i<sysenv.num_elements ; i++)
    {
    if (g_ascii_strcasecmp(text, elements[i].name) == 0)
      return(i);
    }

  tmp = g_strdup(text);
  for (i=0 ; i<strlen(tmp) ; i++)
    {
    if (!g_ascii_isalpha(tmp[i]))
      tmp[i] = ' ';
    }

  buff = tokenize(tmp, &num_tokens);
  g_free(tmp);

  for (i=0 ; i<num_tokens ; i++)
    {
    code = elem_test(buff[i]);
    if (!code)
      {
      gint j;

      for (j=1 ; j<sysenv.num_elements ; j++)
        {
        if (g_ascii_strcasecmp(buff[i], elements[j].name) == 0)
          {
          code = j;
          break;
          }
        }
      }
    if (code)
      break;
    }

  g_strfreev(buff);
  return(code);
}

static struct qbox_species_pak *qbox_species_get(GHashTable *table, const gchar *name)
{
  g_return_val_if_fail(table != NULL, NULL);
  g_return_val_if_fail(name != NULL, NULL);

  return(g_hash_table_lookup(table, name));
}

static gint qbox_species_add(GHashTable *table, const gchar *name, const gchar *uri)
{
  struct qbox_species_pak *species;
  gint atom_code;

  g_return_val_if_fail(table != NULL, 0);
  g_return_val_if_fail(name != NULL, 0);

  species = qbox_species_get(table, name);
  if (species)
    return(species->atom_code);

  atom_code = qbox_guess_element_from_text(name);
  if (!atom_code && uri)
    atom_code = qbox_guess_element_from_text(uri);

  species = g_malloc0(sizeof(struct qbox_species_pak));
  species->name = g_strdup(name);
  species->atom_code = atom_code;
  g_hash_table_insert(table, g_strdup(name), species);

  return(atom_code);
}

static gint qbox_parse_vector_string(const gchar *text, gdouble *x)
{
  gchar **buff;
  gint num_tokens, i;

  g_return_val_if_fail(text != NULL, 1);
  g_return_val_if_fail(x != NULL, 1);

  buff = tokenize(text, &num_tokens);
  if (num_tokens < 3)
    {
    g_strfreev(buff);
    return(1);
    }

  for (i=0 ; i<3 ; i++)
    x[i] = str_to_float(buff[i]);

  g_strfreev(buff);
  return(0);
}

static void qbox_model_reset_atomset(struct model_pak *model)
{
  g_return_if_fail(model != NULL);

  free_core_list(model);
  model->fractional = FALSE;
}

static void qbox_set_cell_from_cart(gdouble *a, gdouble *b, gdouble *c,
                                    gdouble scale, struct model_pak *model)
{
  g_return_if_fail(a != NULL);
  g_return_if_fail(b != NULL);
  g_return_if_fail(c != NULL);
  g_return_if_fail(model != NULL);

  model->periodic = 3;
  model->latmat[0] = a[0] * scale;
  model->latmat[1] = a[1] * scale;
  model->latmat[2] = a[2] * scale;
  model->latmat[3] = b[0] * scale;
  model->latmat[4] = b[1] * scale;
  model->latmat[5] = b[2] * scale;
  model->latmat[6] = c[0] * scale;
  model->latmat[7] = c[1] * scale;
  model->latmat[8] = c[2] * scale;
}

static gchar *qbox_xml_attr_dup(const gchar *line, const gchar *name)
{
  const gchar *start, *end;
  gchar quote;

  g_return_val_if_fail(line != NULL, NULL);
  g_return_val_if_fail(name != NULL, NULL);

  start = g_strstr_len(line, -1, name);
  if (!start)
    return(NULL);

  start += strlen(name);
  while (*start && g_ascii_isspace(*start))
    start++;

  quote = *start;
  if (quote == '\'' || quote == '"')
    {
    start++;
    end = strchr(start, quote);
    }
  else
    {
    end = start;
    while (*end && !g_ascii_isspace(*end) && *end != '>' && *end != '/')
      end++;
    }

  if (!end)
    return(NULL);

  return(g_strndup(start, end-start));
}

static gchar *qbox_xml_tag_text_dup(const gchar *line, const gchar *tag)
{
  gchar *start_tag, *end_tag;
  const gchar *start, *end;

  g_return_val_if_fail(line != NULL, NULL);
  g_return_val_if_fail(tag != NULL, NULL);

  start_tag = g_strdup_printf("<%s", tag);
  start = g_strstr_len(line, -1, start_tag);
  g_free(start_tag);
  if (!start)
    return(NULL);

  start = strchr(start, '>');
  if (!start)
    return(NULL);
  start++;

  end_tag = g_strdup_printf("</%s>", tag);
  end = g_strstr_len(start, -1, end_tag);
  g_free(end_tag);
  if (!end)
    return(NULL);

  return(g_strstrip(g_strndup(start, end-start)));
}

static void qbox_add_atom(struct model_pak *model, GHashTable *species_table,
                          const gchar *atom_name, const gchar *species_name,
                          gdouble *x_bohr)
{
  struct core_pak *core;
  struct qbox_species_pak *species;
  const gchar *label;

  g_return_if_fail(model != NULL);
  g_return_if_fail(species_table != NULL);
  g_return_if_fail(x_bohr != NULL);

  species = NULL;
  if (species_name)
    species = qbox_species_get(species_table, species_name);

  label = "X";
  if (species && species->atom_code > 0)
    label = elements[species->atom_code].symbol;
  else if (species_name)
    label = species_name;
  else if (atom_name)
    label = atom_name;

  core = new_core((gchar *) label, model);
  model->cores = g_slist_append(model->cores, core);

  if (species_name)
    {
    g_free(core->atom_type);
    core->atom_type = g_strdup(species_name);
    }

  core->x[0] = x_bohr[0] * BOHR_TO_ANGS;
  core->x[1] = x_bohr[1] * BOHR_TO_ANGS;
  core->x[2] = x_bohr[2] * BOHR_TO_ANGS;
}

static gint qbox_finish_read(gchar *filename, struct model_pak *model)
{
  g_return_val_if_fail(filename != NULL, 1);
  g_return_val_if_fail(model != NULL, 1);

  if (!model->cores)
    return(1);

  strcpy(model->filename, filename);
  g_free(model->basename);
  model->basename = parse_strip(filename);

  if (!model->title)
    model->title = g_strdup(model->basename);

  model->coord_units = ANGSTROM;
  model_prep(model);

  return(0);
}

gint write_qbox(gchar *filename, struct model_pak *model)
{
  GSList *list, *species_list;
  GHashTable *species_table;
  struct core_pak *core;
  struct elem_pak elem;
  gdouble x[3], min[3], max[3], cell[3], shift[3];
  FILE *fp;
  gboolean first;

  g_return_val_if_fail(filename != NULL, 1);
  g_return_val_if_fail(model != NULL, 1);

  fp = fopen(filename, "wt");
  if (!fp)
    return(2);

  species_table = g_hash_table_new_full(&g_str_hash, &hash_strcmp, &g_free, NULL);
  species_list = NULL;

  first = TRUE;
  for (list=model->cores ; list ; list=g_slist_next(list))
    {
    core = list->data;
    if (core->status & DELETED)
      continue;

    if (get_elem_data(core->atom_code, &elem, model))
      continue;

    if (!g_hash_table_lookup(species_table, elem.symbol))
      {
      g_hash_table_insert(species_table, elem.symbol, elem.symbol);
      species_list = g_slist_append(species_list, elem.symbol);
      }

    ARR3SET(x, core->x);
    if (model->periodic)
      vecmat(model->latmat, x);

    if (first)
      {
      ARR3SET(min, x);
      ARR3SET(max, x);
      first = FALSE;
      }
    else
      {
      min[0] = MIN(min[0], x[0]);
      min[1] = MIN(min[1], x[1]);
      min[2] = MIN(min[2], x[2]);
      max[0] = MAX(max[0], x[0]);
      max[1] = MAX(max[1], x[1]);
      max[2] = MAX(max[2], x[2]);
      }
    }

  if (first)
    {
    g_slist_free(species_list);
    g_hash_table_destroy(species_table);
    fclose(fp);
    return(3);
    }

  fprintf(fp, "# Qbox input generated by GDIS\n");
  fprintf(fp, "# Update the species URIs below to point at valid Qbox pseudopotential files.\n");

  if (model->periodic == 3)
    {
    fprintf(fp, "set cell");
    fprintf(fp, " %14.9f %14.9f %14.9f",
            model->latmat[0] / BOHR_TO_ANGS,
            model->latmat[1] / BOHR_TO_ANGS,
            model->latmat[2] / BOHR_TO_ANGS);
    fprintf(fp, " %14.9f %14.9f %14.9f",
            model->latmat[3] / BOHR_TO_ANGS,
            model->latmat[4] / BOHR_TO_ANGS,
            model->latmat[5] / BOHR_TO_ANGS);
    fprintf(fp, " %14.9f %14.9f %14.9f\n",
            model->latmat[6] / BOHR_TO_ANGS,
            model->latmat[7] / BOHR_TO_ANGS,
            model->latmat[8] / BOHR_TO_ANGS);
    VEC3SET(shift, 0.0, 0.0, 0.0);
    }
  else
    {
    cell[0] = MAX(max[0] - min[0] + 16.0, 12.0);
    cell[1] = MAX(max[1] - min[1] + 16.0, 12.0);
    cell[2] = MAX(max[2] - min[2] + 16.0, 12.0);
    shift[0] = 8.0 - min[0];
    shift[1] = 8.0 - min[1];
    shift[2] = 8.0 - min[2];

    fprintf(fp, "# Non-periodic model exported in a vacuum box.\n");
    fprintf(fp, "set cell %14.9f 0.0 0.0 0.0 %14.9f 0.0 0.0 0.0 %14.9f\n",
            cell[0] / BOHR_TO_ANGS,
            cell[1] / BOHR_TO_ANGS,
            cell[2] / BOHR_TO_ANGS);
    }

  for (list=species_list ; list ; list=g_slist_next(list))
    {
    gchar *symbol = list->data;

    fprintf(fp, "species %-4s %s.xml\n", symbol, symbol);
    }

  for (list=model->cores ; list ; list=g_slist_next(list))
    {
    core = list->data;
    if (core->status & DELETED)
      continue;
    if (get_elem_data(core->atom_code, &elem, model))
      continue;

    ARR3SET(x, core->x);
    if (model->periodic)
      vecmat(model->latmat, x);
    ARR3ADD(x, shift);

    fprintf(fp, "atom %-8s %-4s %14.9f %14.9f %14.9f\n",
            core->atom_label ? core->atom_label : elem.symbol,
            elem.symbol,
            x[0] / BOHR_TO_ANGS,
            x[1] / BOHR_TO_ANGS,
            x[2] / BOHR_TO_ANGS);
    }

  g_slist_free(species_list);
  g_hash_table_destroy(species_table);
  fclose(fp);

  return(0);
}

gint read_qbox(gchar *filename, struct model_pak *model)
{
  FILE *fp;
  gchar *line;
  GHashTable *species_table;

  g_return_val_if_fail(filename != NULL, 1);
  g_return_val_if_fail(model != NULL, 1);

  fp = fopen(filename, "rt");
  if (!fp)
    return(1);

  species_table = g_hash_table_new_full(&g_str_hash, &hash_strcmp,
                                        &g_free, &qbox_species_free);

  while ((line = file_read_line(fp)))
    {
    gchar **buff;
    gint num_tokens;

    qbox_strip_inline_comment(line);
    g_strstrip(line);
    if (!strlen(line))
      {
      g_free(line);
      continue;
      }

    buff = tokenize(line, &num_tokens);
    if (!buff)
      {
      g_free(line);
      continue;
      }

    if (num_tokens > 2 && g_ascii_strcasecmp(buff[0], "species") == 0)
      {
      qbox_species_add(species_table, buff[1], buff[2]);
      }
    else if (num_tokens > 10 && g_ascii_strcasecmp(buff[0], "set") == 0
             && g_ascii_strcasecmp(buff[1], "cell") == 0)
      {
      gdouble a[3], b[3], c[3];

      a[0] = str_to_float(buff[2]);
      a[1] = str_to_float(buff[3]);
      a[2] = str_to_float(buff[4]);
      b[0] = str_to_float(buff[5]);
      b[1] = str_to_float(buff[6]);
      b[2] = str_to_float(buff[7]);
      c[0] = str_to_float(buff[8]);
      c[1] = str_to_float(buff[9]);
      c[2] = str_to_float(buff[10]);
      qbox_set_cell_from_cart(a, b, c, BOHR_TO_ANGS, model);
      }
    else if (num_tokens > 5 && g_ascii_strcasecmp(buff[0], "atom") == 0)
      {
      gdouble x[3];

      x[0] = str_to_float(buff[3]);
      x[1] = str_to_float(buff[4]);
      x[2] = str_to_float(buff[5]);
      qbox_add_atom(model, species_table, buff[1], buff[2], x);
      }

    g_strfreev(buff);
    g_free(line);
    }

  fclose(fp);
  g_hash_table_destroy(species_table);

  return(qbox_finish_read(filename, model));
}

gint read_qbox_out(gchar *filename, struct model_pak *model)
{
  FILE *fp;
  gchar *line;
  GHashTable *species_table;
  struct core_pak *current_core;
  gint xml_state;
  gdouble latest_energy, max_force;
  gboolean have_energy, have_force;

  g_return_val_if_fail(filename != NULL, 1);
  g_return_val_if_fail(model != NULL, 1);

  fp = fopen(filename, "rt");
  if (!fp)
    return(1);

  species_table = g_hash_table_new_full(&g_str_hash, &hash_strcmp,
                                        &g_free, &qbox_species_free);
  current_core = NULL;
  xml_state = QBOX_XML_NONE;
  latest_energy = 0.0;
  max_force = 0.0;
  have_energy = FALSE;
  have_force = FALSE;

  while ((line = file_read_line(fp)))
    {
    if (g_strrstr(line, "<atomset"))
      {
      qbox_model_reset_atomset(model);
      current_core = NULL;
      xml_state = QBOX_XML_NONE;
      }
    else if (g_strrstr(line, "<unit_cell"))
      {
      gchar *a_text, *b_text, *c_text;
      gdouble a[3], b[3], c[3];

      a_text = qbox_xml_attr_dup(line, "a=");
      b_text = qbox_xml_attr_dup(line, "b=");
      c_text = qbox_xml_attr_dup(line, "c=");
      if (a_text && b_text && c_text
          && !qbox_parse_vector_string(a_text, a)
          && !qbox_parse_vector_string(b_text, b)
          && !qbox_parse_vector_string(c_text, c))
        qbox_set_cell_from_cart(a, b, c, BOHR_TO_ANGS, model);
      g_free(a_text);
      g_free(b_text);
      g_free(c_text);
      }
    else if (g_strrstr(line, "<atom "))
      {
      gchar *species_name, *atom_name;
      gint atom_code;
      const gchar *label;

      species_name = qbox_xml_attr_dup(line, "species=");
      atom_name = qbox_xml_attr_dup(line, "name=");
      atom_code = qbox_species_add(species_table, species_name ? species_name : "X", species_name);
      label = atom_code ? elements[atom_code].symbol : "X";

      current_core = new_core((gchar *) label, model);
      model->cores = g_slist_append(model->cores, current_core);

      if (species_name)
        {
        g_free(current_core->atom_type);
        current_core->atom_type = g_strdup(species_name);
        }
      if (atom_name)
        {
        g_free(current_core->atom_label);
        current_core->atom_label = g_strdup(atom_name);
        current_core->atom_code = atom_code ? atom_code : current_core->atom_code;
        }

      g_free(species_name);
      g_free(atom_name);
      }
    else if (g_strrstr(line, "</atom>"))
      {
      current_core = NULL;
      xml_state = QBOX_XML_NONE;
      }

    if (current_core && g_strrstr(line, "<position"))
      xml_state = QBOX_XML_POSITION;
    else if (current_core && g_strrstr(line, "<velocity"))
      xml_state = QBOX_XML_VELOCITY;
    else if (g_strrstr(line, "<force"))
      xml_state = QBOX_XML_FORCE;
    else if (g_strrstr(line, "<etotal"))
      xml_state = QBOX_XML_ENERGY;

    if (xml_state == QBOX_XML_POSITION || xml_state == QBOX_XML_VELOCITY
        || xml_state == QBOX_XML_FORCE || xml_state == QBOX_XML_ENERGY)
      {
      gchar *text;

      text = NULL;
      if (xml_state == QBOX_XML_POSITION)
        text = qbox_xml_tag_text_dup(line, "position");
      if (xml_state == QBOX_XML_VELOCITY)
        text = qbox_xml_tag_text_dup(line, "velocity");
      if (xml_state == QBOX_XML_FORCE)
        text = qbox_xml_tag_text_dup(line, "force");
      if (xml_state == QBOX_XML_ENERGY)
        text = qbox_xml_tag_text_dup(line, "etotal");

      if (text)
        {
        gdouble x[3];

        switch (xml_state)
          {
          case QBOX_XML_POSITION:
            if (current_core && !qbox_parse_vector_string(text, x))
              {
              current_core->x[0] = x[0] * BOHR_TO_ANGS;
              current_core->x[1] = x[1] * BOHR_TO_ANGS;
              current_core->x[2] = x[2] * BOHR_TO_ANGS;
              }
            break;

          case QBOX_XML_VELOCITY:
            if (current_core && !qbox_parse_vector_string(text, x))
              {
              current_core->v[0] = x[0] * BOHR_TO_ANGS;
              current_core->v[1] = x[1] * BOHR_TO_ANGS;
              current_core->v[2] = x[2] * BOHR_TO_ANGS;
              }
            break;

          case QBOX_XML_FORCE:
            if (!qbox_parse_vector_string(text, x))
              {
              gdouble force = VEC3MAG(x);
              if (force > max_force)
                max_force = force;
              have_force = TRUE;
              }
            break;

          case QBOX_XML_ENERGY:
            latest_energy = str_to_float(text);
            have_energy = TRUE;
            break;
          }

        g_free(text);

        if (g_strrstr(line, "</position>") || g_strrstr(line, "</velocity>")
            || g_strrstr(line, "</force>") || g_strrstr(line, "</etotal>"))
          xml_state = QBOX_XML_NONE;
        }
      }

    g_free(line);
    }

  fclose(fp);
  g_hash_table_destroy(species_table);

  if (qbox_finish_read(filename, model))
    return(1);

  if (have_energy)
    {
    gchar *text;

    text = g_strdup_printf("%f Hartree", latest_energy);
    property_add_ranked(3, "Energy", text, model);
    g_free(text);
    }
  if (have_force)
    {
    gchar *text;

    text = g_strdup_printf("%f Hartree/Bohr", max_force);
    property_add_ranked(4, "Max force", text, model);
    g_free(text);
    }

  return(0);
}
