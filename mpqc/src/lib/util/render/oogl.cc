//
// oogl.cc
//
// Copyright (C) 1996 Limit Point Systems, Inc.
//
// Author: Curtis Janssen <cljanss@limitpt.com>
// Maintainer: LPS
//
// This file is part of the SC Toolkit.
//
// The SC Toolkit is free software; you can redistribute it and/or modify
// it under the terms of the GNU Library General Public License as published by
// the Free Software Foundation; either version 2, or (at your option)
// any later version.
//
// The SC Toolkit is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public License
// along with the SC Toolkit; see the file COPYING.LIB.  If not, write to
// the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
//
// The U.S. Government is granted a limited license as per AL 91-7.
//

#include <util/misc/formio.h>
#include <util/keyval/keyval.h>
#include <util/render/oogl.h>
#include <util/render/object.h>
#include <util/render/sphere.h>
#include <util/render/polygons.h>
#include <util/render/polylines.h>
#include <util/render/material.h>

#define CLASSNAME OOGLRender
#define PARENTS public Render
#define HAVE_KEYVAL_CTOR
#include <util/class/classi.h>
void *
OOGLRender::_castdown(const ClassDesc*cd)
{
  void* casts[1];
  casts[0] = Render::_castdown(cd);
  return do_castdowns(casts,cd);
}

OOGLRender::OOGLRender(const char * filename)
{
  filename_ = strcpy(new char[strlen(filename)+1], filename);
  oogl_spheres_ = 0;
  fp_ = 0;
  clear();
}

OOGLRender::OOGLRender(FILE * fp)
{
  oogl_spheres_ = 0;
  filename_ = 0;
  fp_ = fp;
  clear();
}

OOGLRender::OOGLRender(const RefKeyVal& keyval):
  Render(keyval)
{
  filename_ = keyval->pcharvalue("filename");
  oogl_spheres_ = keyval->booleanvalue("oogl_spheres");
  if (!filename_) fp_ = stdout;
  else fp_ = 0;
  clear();
}

OOGLRender::~OOGLRender()
{
  if (filename_) {
      delete[] filename_;
      if (fp_) fclose(fp_);
    }
}

void
OOGLRender::clear()
{
  if (filename_) {
      if (fp_) {
          fclose(fp_);
        }
      fp_ = fopen(filename_, "w");
      if (!fp_) {
          cerr << scprintf("OOGLRender: couldn't open \"%s\"\n", filename_);
          abort();
        }
    }
}

void
OOGLRender::render(const RefRenderedObject& object)
{
  fprintf(fp_, "{\n");
  if (object->name()) {
      fprintf(fp_, "define %s\n", object->name());
    }
  if (object->transform().nonnull()) {
      fprintf(fp_, "= INST\n");
      fprintf(fp_, "transform {\n");
      for (int i=0; i<4; i++) {
          for (int j=0; j<4; j++) {
              fprintf(fp_, " %10.4f", object->transform()->transform()[j][i]);
            }
          fprintf(fp_,"\n");
        }
      fprintf(fp_,"}\n");
      fprintf(fp_,"geom {\n");
    }
  if (object->material().nonnull()
      ||object->appearance().nonnull()) {
      fprintf(fp_, "appearance {\n");
      if (object->material().nonnull()) {
          fprintf(fp_, "material {\n");
          if (object->material()->ambient().is_set()) {
              if (object->material()->ambient().overrides()) fprintf(fp_, "*");
              fprintf(fp_, "ambient %10.4f %10.4f %10.4f\n",
                      object->material()->ambient().value().red(),
                      object->material()->ambient().value().green(),
                      object->material()->ambient().value().blue());
            }
          if (object->material()->diffuse().is_set()) {
              if (object->material()->diffuse().overrides()) fprintf(fp_, "*");
              fprintf(fp_, "diffuse %10.4f %10.4f %10.4f\n",
                      object->material()->diffuse().value().red(),
                      object->material()->diffuse().value().green(),
                      object->material()->diffuse().value().blue());
            }
          fprintf(fp_, "}\n");
        }
      fprintf(fp_, "}\n");
    }

  Render::render(object);

  if (object->transform().nonnull()) {
      fprintf(fp_,"}\n");
    }
  fprintf(fp_, "}\n");
}

void
OOGLRender::set(const RefRenderedObjectSet& set)
{
  fprintf(fp_,"LIST\n");
  for (int i=0; i<set->n(); i++) {
      render(set->element(i));
    }
}

void
OOGLRender::sphere(const RefRenderedSphere& sphere)
{
  if (oogl_spheres_) {
      fprintf(fp_," = SPHERE 1.0 0.0 0.0 0.0\n");
    }
  else {
      Render::sphere(sphere);
    }
}

void
OOGLRender::polygons(const RefRenderedPolygons& poly)
{
  if (poly->have_vertex_rgb()) {
      fprintf(fp_, " = COFF\n");
    }
  else {
      fprintf(fp_," = OFF\n");
    }
  fprintf(fp_, "%d %d 0\n", poly->nvertex(), poly->nface());
  int i;
  for (i=0; i<poly->nvertex(); i++) {
      fprintf(fp_, " %10.4f %10.4f %10.4f",
              poly->vertex(i,0),
              poly->vertex(i,1),
              poly->vertex(i,2));
      if (poly->have_vertex_rgb()) {
          // The 1.0 is alpha
          fprintf(fp_, " %10.4f %10.4f %10.4f 1.0",
                  poly->vertex_rgb(i,0),
                  poly->vertex_rgb(i,1),
                  poly->vertex_rgb(i,2));
        }
      fprintf(fp_, "\n");
    }
  for (i=0; i<poly->nface(); i++) {
      fprintf(fp_, " %d", poly->nvertex_in_face(i));
      for (int j=0; j<poly->nvertex_in_face(i); j++) {
          fprintf(fp_, " %d", poly->face(i,j));
        }
      fprintf(fp_, "\n");
    }
}

void
OOGLRender::polylines(const RefRenderedPolylines& poly)
{
  int i;

  int nvertex= 0;
  for (i=0; i<poly->npolyline(); i++) nvertex += poly->nvertex_in_polyline(i);
  fprintf(fp_, " = VECT\n");
  fprintf(fp_, "%d %d %d\n",
          poly->npolyline(),
          nvertex,
          (poly->have_vertex_rgb()? nvertex:0));
  for (i=0; i<poly->npolyline(); i++) {
      fprintf(fp_, " %d", poly->nvertex_in_polyline(i));
    }
  fprintf(fp_, "\n");
  if (poly->have_vertex_rgb()) {
      for (i=0; i<poly->npolyline(); i++) {
          fprintf(fp_, " %d", poly->nvertex_in_polyline(i));
        }
    }
  else {
      for (i=0; i<poly->npolyline(); i++) {
          fprintf(fp_, " 0");
        }
    }
  fprintf(fp_, "\n");
  for (i=0; i<poly->npolyline(); i++) {
      for (int j=0; j<poly->nvertex_in_polyline(i); j++) {
          int ivertex = poly->polyline(i,j);
          fprintf(fp_, " %10.4f %10.4f %10.4f",
                  poly->vertex(ivertex,0),
                  poly->vertex(ivertex,1),
                  poly->vertex(ivertex,2));
        }
    }
  fprintf(fp_, "\n");
  if (poly->have_vertex_rgb()) {
      for (i=0; i<poly->npolyline(); i++) {
          for (int j=0; j<poly->nvertex_in_polyline(i); j++) {
              int ivertex = poly->polyline(i,j);
              fprintf(fp_, " %10.4f %10.4f %10.4f 1.0",
                      poly->vertex_rgb(ivertex,0),
                      poly->vertex_rgb(ivertex,1),
                      poly->vertex_rgb(ivertex,2));
            }
        }
      fprintf(fp_, "\n");
    }
}

/////////////////////////////////////////////////////////////////////////////

// Local Variables:
// mode: c++
// c-file-style: "CLJ"
// End:

