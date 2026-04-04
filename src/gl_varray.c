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
#include <string.h>
#include <math.h>

#include "gdis.h"
#include "coords.h"
#include "matrix.h"
#include "numeric.h"
#include "opengl.h"
#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

/* externals */
extern struct sysenv_pak sysenv;


struct varray_pak
{
/* config */
gint gl_method;
gint periodic;
gint num_indices;
gint num_vertices;

/* data */
GLuint *indices;
/*
GLdouble *vertices;
GLdouble *new_vertices;
GLdouble *normals;
*/
GLfloat *template_interleaved;
GLfloat *interleaved;

GLdouble *colours;
};


/**********************/
/* creation primitive */
/**********************/
gpointer va_init(void)
{
struct varray_pak *va;

va = g_malloc(sizeof(struct varray_pak));

va->gl_method = 0;
va->periodic = FALSE;
va->num_indices = 0;
va->num_vertices = 0;

va->indices = NULL;
/*
va->vertices = NULL;
va->new_vertices = NULL;
va->normals = NULL;
*/
va->template_interleaved = NULL;
va->interleaved = NULL;

va->colours = NULL;

return(va);
}

/*************************/
/* destruction primitive */
/*************************/
void va_free(gpointer ptr_varray)
{
struct varray_pak *va = ptr_varray;

g_free(va->indices);
/*
g_free(va->vertices);
g_free(va->new_vertices);
g_free(va->normals);
*/

g_free(va->template_interleaved);
g_free(va->interleaved);

g_free(va->colours);
g_free(va);
}

static void va_clear_geometry(struct varray_pak *va)
{
g_return_if_fail(va != NULL);

g_free(va->indices);
va->indices = NULL;
g_free(va->template_interleaved);
va->template_interleaved = NULL;
g_free(va->interleaved);
va->interleaved = NULL;
va->num_indices = 0;
va->num_vertices = 0;
}


/* FIXME - these should really be guints */
gint cyclic_clamp(gint n, gint start, gint stop)
{
gint range, offset;

range = stop - start + 1;
offset = n - start;
offset /= range;

return(n - offset*range);
}

gint cyclic_inc(gint n, gint start, gint stop)
{
if (n < stop)
  return(n+1);
else
  return(start);
}

gint cyclic_dec(gint n, gint start, gint stop)
{
if (n > start)
  return(n-1);
else
  return(stop);
}

/* CURRENT */
#define DEBUG_VA_GENERATE_INDICES 0
void va_generate_indices(gint depth, struct varray_pak *va)
{
gint d, i, j, m, n;
gint div, mstart, nstart;
#ifdef UNUSED_BUT_SET
gint mdiv;
#endif

g_assert(va != NULL);

n = 18 * depth*depth;

#if DEBUG_VA_GENERATE_INDICES
printf("allocating for indices: %d\n", n);
#endif

va->num_indices = n;
va->indices = g_malloc(n * sizeof(GLuint));

i = 0;
n=1;
mstart = 0;
for (d=1 ; d<depth+1 ; d++)
  {
  nstart = n;
  div = d*6;

#ifdef UNUSED_BUT_SET
mdiv = 6*(d-1);
#endif

  m = mstart;

/* special case start */
  for (j=div ; j-- ; )
    {

*(va->indices+i++) = n;
#if DEBUG_VA_GENERATE_INDICES
printf(" %d", *(va->indices+i-1));
#endif

*(va->indices+i++) = cyclic_dec(n, nstart, nstart+div-1);
#if DEBUG_VA_GENERATE_INDICES
printf(" %d", *(va->indices+i-1));
#endif
n++;

*(va->indices+i++) = m++;
#if DEBUG_VA_GENERATE_INDICES
printf(" %d", *(va->indices+i-1));
#endif

    if ((j+1) % d == 0)
      m--;
    else
      {

/* missing triangle */
*(va->indices+i++) = cyclic_dec(m, mstart, nstart-1);
#if DEBUG_VA_GENERATE_INDICES
printf(" (%d", *(va->indices+i-1));
#endif
*(va->indices+i++) = cyclic_clamp(m, mstart, nstart-1);
#if DEBUG_VA_GENERATE_INDICES
printf(" %d", *(va->indices+i-1));
#endif
*(va->indices+i++) = cyclic_dec(n, nstart, nstart+div-1);
#if DEBUG_VA_GENERATE_INDICES
printf(" %d)", *(va->indices+i-1));
#endif
 
      }


    if (m >= nstart)
      {
      m -= nstart;
      m += mstart;
      }

    }

#if DEBUG_VA_GENERATE_INDICES
printf("\n");
#endif

  mstart = nstart;
  }

}

/*******************************/
/* sphere array initialization */
/*******************************/
#define DEBUG_MAKE_SPHERE 0
void va_make_sphere(gpointer ptr_varray)
{
gint i, j, k;
gint quality;
gint stacks, slices;
gint row0, row1;
gdouble phi, theta, sp, cp, st, ct;
struct varray_pak *va = ptr_varray;

g_return_if_fail(va != NULL);

quality = nearest_int(sysenv.render.sphere_quality);
if (quality < 0)
  quality = 0;

stacks = 6 + 2*quality;
slices = 2*stacks;

va_clear_geometry(va);

va->gl_method = GL_TRIANGLES;
va->num_vertices = (stacks + 1) * (slices + 1);
va->num_indices = stacks * slices * 6;
va->template_interleaved = g_malloc0(6 * va->num_vertices * sizeof(GLfloat));
va->interleaved = g_malloc0(6 * va->num_vertices * sizeof(GLfloat));
va->indices = g_malloc(va->num_indices * sizeof(GLuint));

k = 0;
for (i=0 ; i<=stacks ; i++)
  {
  phi = G_PI * (gdouble) i / (gdouble) stacks;
  sp = sin(phi);
  cp = cos(phi);

  for (j=0 ; j<=slices ; j++)
    {
    theta = 2.0 * G_PI * (gdouble) j / (gdouble) slices;
    st = sin(theta);
    ct = cos(theta);

    va->template_interleaved[6*k+0] = ct*sp;
    va->template_interleaved[6*k+1] = st*sp;
    va->template_interleaved[6*k+2] = -cp;
    va->template_interleaved[6*k+3] = va->template_interleaved[6*k+0];
    va->template_interleaved[6*k+4] = va->template_interleaved[6*k+1];
    va->template_interleaved[6*k+5] = va->template_interleaved[6*k+2];
    k++;
    }
  }

memcpy(va->interleaved, va->template_interleaved,
       6 * va->num_vertices * sizeof(GLfloat));

k = 0;
for (i=0 ; i<stacks ; i++)
  {
  row0 = i * (slices + 1);
  row1 = (i + 1) * (slices + 1);

  for (j=0 ; j<slices ; j++)
    {
    va->indices[k++] = row0 + j;
    va->indices[k++] = row1 + j;
    va->indices[k++] = row1 + j + 1;
    va->indices[k++] = row0 + j;
    va->indices[k++] = row1 + j + 1;
    va->indices[k++] = row0 + j + 1;
    }
  }

#if DEBUG_MAKE_SPHERE
printf("stacks = %d, slices = %d, vertices = %d, indices = %d\n",
       stacks, slices, va->num_vertices, va->num_indices);
#endif
}

/*********************************/
/* cylinder array initialization */
/*********************************/
void va_make_cylinder(gpointer ptr_varray)
{
gint i, k;
gint quality;
gint slices;
gint side_base, bottom_base, top_base;
gint bottom_center, top_center;
struct varray_pak *va = ptr_varray;

g_return_if_fail(va != NULL);

quality = nearest_int(sysenv.render.cylinder_quality);
if (quality < 4)
  quality = 4;
if (quality > 64)
  quality = 64;

slices = quality;

va_clear_geometry(va);

va->gl_method = GL_TRIANGLES;
va->num_vertices = 4 * (slices + 1) + 2;
va->num_indices = slices * 12;
va->template_interleaved = g_malloc0(6 * va->num_vertices * sizeof(GLfloat));
va->interleaved = g_malloc0(6 * va->num_vertices * sizeof(GLfloat));
va->indices = g_malloc(va->num_indices * sizeof(GLuint));

side_base = 0;
bottom_base = 2 * (slices + 1);
top_base = bottom_base + (slices + 1);
bottom_center = top_base + (slices + 1);
top_center = bottom_center + 1;

for (i=0 ; i<=slices ; i++)
  {
  gdouble theta;
  gdouble st, ct;

  theta = 2.0 * G_PI * (gdouble) i / (gdouble) slices;
  st = sin(theta);
  ct = cos(theta);

  k = side_base + 2*i;
  va->template_interleaved[6*k+0] = ct;
  va->template_interleaved[6*k+1] = st;
  va->template_interleaved[6*k+2] = 0.0f;
  va->template_interleaved[6*k+3] = ct;
  va->template_interleaved[6*k+4] = st;
  va->template_interleaved[6*k+5] = 0.0f;

  k++;
  va->template_interleaved[6*k+0] = ct;
  va->template_interleaved[6*k+1] = st;
  va->template_interleaved[6*k+2] = 1.0f;
  va->template_interleaved[6*k+3] = ct;
  va->template_interleaved[6*k+4] = st;
  va->template_interleaved[6*k+5] = 0.0f;

  k = bottom_base + i;
  va->template_interleaved[6*k+0] = ct;
  va->template_interleaved[6*k+1] = st;
  va->template_interleaved[6*k+2] = 0.0f;
  va->template_interleaved[6*k+3] = 0.0f;
  va->template_interleaved[6*k+4] = 0.0f;
  va->template_interleaved[6*k+5] = -1.0f;

  k = top_base + i;
  va->template_interleaved[6*k+0] = ct;
  va->template_interleaved[6*k+1] = st;
  va->template_interleaved[6*k+2] = 1.0f;
  va->template_interleaved[6*k+3] = 0.0f;
  va->template_interleaved[6*k+4] = 0.0f;
  va->template_interleaved[6*k+5] = 1.0f;
  }

va->template_interleaved[6*bottom_center+0] = 0.0f;
va->template_interleaved[6*bottom_center+1] = 0.0f;
va->template_interleaved[6*bottom_center+2] = 0.0f;
va->template_interleaved[6*bottom_center+3] = 0.0f;
va->template_interleaved[6*bottom_center+4] = 0.0f;
va->template_interleaved[6*bottom_center+5] = -1.0f;

va->template_interleaved[6*top_center+0] = 0.0f;
va->template_interleaved[6*top_center+1] = 0.0f;
va->template_interleaved[6*top_center+2] = 1.0f;
va->template_interleaved[6*top_center+3] = 0.0f;
va->template_interleaved[6*top_center+4] = 0.0f;
va->template_interleaved[6*top_center+5] = 1.0f;

memcpy(va->interleaved, va->template_interleaved,
       6 * va->num_vertices * sizeof(GLfloat));

k = 0;
for (i=0 ; i<slices ; i++)
  {
  gint row0, row1;

  row0 = side_base + 2*i;
  row1 = side_base + 2*(i+1);

  va->indices[k++] = row0;
  va->indices[k++] = row0 + 1;
  va->indices[k++] = row1 + 1;
  va->indices[k++] = row0;
  va->indices[k++] = row1 + 1;
  va->indices[k++] = row1;

  va->indices[k++] = bottom_center;
  va->indices[k++] = bottom_base + i + 1;
  va->indices[k++] = bottom_base + i;

  va->indices[k++] = top_center;
  va->indices[k++] = top_base + i;
  va->indices[k++] = top_base + i + 1;
  }
}

/****************************/
/* prepare sphere positions */
/****************************/
void va_prepare_sphere(gpointer ptr_varray, gdouble *x, gdouble r)
{
gint i;
GLfloat *interleaved;
struct varray_pak *va = ptr_varray;

/* setup vertex/normal arrays */
interleaved = va->interleaved;
g_return_if_fail(va->template_interleaved != NULL);

/* expand to required radius and move to desired location */
for (i=va->num_vertices ; i-- ; )
  {
  ARR3SET((interleaved+6*i), (va->template_interleaved+6*i));
  VEC3MUL((interleaved+6*i), r);
  ARR3ADD((interleaved+6*i), x);
  ARR3SET((interleaved+6*i+3), (va->template_interleaved+6*i+3));
  }
}

/******************************/
/* prepare cylinder positions */
/******************************/
void va_prepare_cylinder(gpointer ptr_varray, gdouble *v1, gdouble *v2, gdouble r)
{
gint i;
gdouble axis[3], p[3], q[3], pos[3], normal[3];
gdouble len;
GLfloat *interleaved;
GLfloat *base;
struct varray_pak *va = ptr_varray;

g_return_if_fail(va != NULL);
g_return_if_fail(v1 != NULL);
g_return_if_fail(v2 != NULL);
g_return_if_fail(va->template_interleaved != NULL);

interleaved = va->interleaved;

ARR3SET(axis, v2);
ARR3SUB(axis, v1);
len = VEC3MAG(axis);
if (len < FRACTION_TOLERANCE)
  {
  for (i=0 ; i<va->num_vertices ; i++)
    {
    ARR3SET((interleaved+6*i), v1);
    ARR3SET((interleaved+6*i+3), (va->template_interleaved+6*i+3));
    }
  return;
  }

normalize(axis, 3);

VEC3SET(p, 1.0, 1.0, 1.0);
crossprod(q, p, axis);
if (VEC3MAGSQ(q) < FRACTION_TOLERANCE)
  {
  VEC3SET(p, 0.0, 1.0, 0.0);
  crossprod(q, p, axis);
  if (VEC3MAGSQ(q) < FRACTION_TOLERANCE)
    {
    VEC3SET(p, 0.0, 0.0, 1.0);
    crossprod(q, p, axis);
    }
  }
crossprod(p, axis, q);
normalize(p, 3);
normalize(q, 3);

for (i=0 ; i<va->num_vertices ; i++)
  {
  base = va->template_interleaved + 6*i;
  pos[0] = v1[0] + r * (base[0] * p[0] + base[1] * q[0])
                 + len * base[2] * axis[0];
  pos[1] = v1[1] + r * (base[0] * p[1] + base[1] * q[1])
                 + len * base[2] * axis[1];
  pos[2] = v1[2] + r * (base[0] * p[2] + base[1] * q[2])
                 + len * base[2] * axis[2];

  normal[0] = base[3] * p[0] + base[4] * q[0] + base[5] * axis[0];
  normal[1] = base[3] * p[1] + base[4] * q[1] + base[5] * axis[1];
  normal[2] = base[3] * p[2] + base[4] * q[2] + base[5] * axis[2];
  if (VEC3MAGSQ(normal) > FRACTION_TOLERANCE)
    normalize(normal, 3);
  else
    ARR3SET(normal, axis);

  ARR3SET((interleaved+6*i), pos);
  ARR3SET((interleaved+6*i+3), normal);
  }
}

/*******************/
/* mesh accessors */
/*******************/
gint va_get_gl_method(gpointer ptr_varray)
{
struct varray_pak *va = ptr_varray;

g_return_val_if_fail(va != NULL, 0);

return(va->gl_method);
}

gint va_get_num_vertices(gpointer ptr_varray)
{
struct varray_pak *va = ptr_varray;

g_return_val_if_fail(va != NULL, 0);

return(va->num_vertices);
}

gint va_get_num_indices(gpointer ptr_varray)
{
struct varray_pak *va = ptr_varray;

g_return_val_if_fail(va != NULL, 0);

return(va->num_indices);
}

gconstpointer va_get_interleaved(gpointer ptr_varray)
{
struct varray_pak *va = ptr_varray;

g_return_val_if_fail(va != NULL, NULL);

return(va->interleaved);
}

gconstpointer va_get_indices(gpointer ptr_varray)
{
struct varray_pak *va = ptr_varray;

g_return_val_if_fail(va != NULL, NULL);

return(va->indices);
}

/*********************/
/* drawing primitive */
/*********************/
void va_draw_sphere(gpointer ptr_varray, gdouble *x, gdouble r)
{
GLfloat *interleaved;
struct varray_pak *va = ptr_varray;

va_prepare_sphere(ptr_varray, x, r);

interleaved = va->interleaved;

/* draw vertex array */
/*
glVertexPointer(3, GL_DOUBLE, 0, vertices);
if (va->normals)
  glNormalPointer(GL_DOUBLE, 0, va->normals);
*/

glVertexPointer(3, GL_FLOAT, 6*sizeof(GLfloat), &interleaved[0]);
glNormalPointer(GL_FLOAT, 6*sizeof(GLfloat), &interleaved[3]);


glDrawElements(va->gl_method, va->num_indices, GL_UNSIGNED_INT, va->indices);
}
