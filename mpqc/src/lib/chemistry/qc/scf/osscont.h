//
// osscont.h --- definition of the open shell singlet fock contribution class
//
// Copyright (C) 1996 Limit Point Systems, Inc.
//
// Author: Edward Seidl <seidl@janed.com>
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

#ifndef _chemistry_qc_scf_osscont_h
#define _chemistry_qc_scf_osscont_h

#ifdef __GNUC__
#pragma interface
#endif

#include <chemistry/qc/scf/scf.h>

///////////////////////////////////////////////////////////////////////////

class LocalOSSContribution {
  private:
    double * const gmat;
    double * const gmata;
    double * const gmatb;

    double * const pmat;
    double * const pmata;
    double * const pmatb;

    double bound;
  public:
    LocalOSSContribution(double *g, double *p, double *ga, double *pa,
                         double *gb, double *pb) :
      gmat(g), pmat(p), gmata(ga), pmata(pa), gmatb(gb), pmatb(pb),
      bound(0.0) {}
    ~LocalOSSContribution() {}

    void set_bound(double b) { bound = b; }

    inline void cont1(int ij, int kl, double val) {
      gmat[ij] += val*pmat[kl];
      gmat[kl] += val*pmat[ij];
    }
    
    inline void cont2(int ij, int kl, double val) {
      val *= 0.25;
      gmat[ij] -= val*pmat[kl];
      gmat[kl] -= val*pmat[ij];

      gmata[ij] += val*pmata[kl];
      gmata[kl] += val*pmata[ij];

      gmatb[ij] += val*pmatb[kl];
      gmatb[kl] += val*pmatb[ij];

      val *= -3.0;
      gmatb[ij] += val*pmata[kl];
      gmatb[kl] += val*pmata[ij];

      gmata[ij] += val*pmatb[kl];
      gmata[kl] += val*pmatb[ij];
    }
    
    inline void cont3(int ij, int kl, double val) {
      val *= 0.5;
      gmat[ij] -= val*pmat[kl];
      gmat[kl] -= val*pmat[ij];

      gmata[ij] += val*pmata[kl];
      gmata[kl] += val*pmata[ij];

      gmatb[ij] += val*pmatb[kl];
      gmatb[kl] += val*pmatb[ij];

      val *= -3.0;
      gmata[ij] += val*pmatb[kl];
      gmata[kl] += val*pmatb[ij];

      gmatb[ij] += val*pmata[kl];
      gmatb[kl] += val*pmata[ij];
    }
    
    inline void cont4(int ij, int kl, double val) {
      gmat[ij] += 0.75*val*pmat[kl];
      gmat[kl] += 0.75*val*pmat[ij];

      gmata[ij] += 0.25*val*pmata[kl];
      gmata[kl] += 0.25*val*pmata[ij];

      gmatb[ij] += 0.25*val*pmatb[kl];
      gmatb[kl] += 0.25*val*pmatb[ij];

      gmata[ij] -= 0.75*val*pmatb[kl];
      gmata[kl] -= 0.75*val*pmatb[ij];

      gmatb[ij] -= 0.75*val*pmata[kl];
      gmatb[kl] -= 0.75*val*pmata[ij];
    }
    
    inline void cont5(int ij, int kl, double val) {
      val *= 0.5;
      gmat[ij] += val*pmat[kl];
      gmat[kl] += val*pmat[ij];

      gmata[ij] += val*pmata[kl];
      gmata[kl] += val*pmata[ij];

      gmatb[ij] += val*pmatb[kl];
      gmatb[kl] += val*pmatb[ij];

      val *= -3.0;
      gmata[ij] += val*pmatb[kl];
      gmata[kl] += val*pmatb[ij];

      gmatb[ij] += val*pmata[kl];
      gmatb[kl] += val*pmata[ij];
    }
};

class LocalOSSGradContribution {
  private:
    double * const pmat;
    double * const pmata;
    double * const pmatb;

  public:
    LocalOSSGradContribution(double *p, double *pa, double *pb) :
      pmat(p), pmata(pa), pmatb(pb) {}
    ~LocalOSSGradContribution() {}

    inline double cont1(int ij, int kl) {
      return pmat[ij]*pmat[kl] +
        0.5*(pmata[ij]*pmat[kl] + pmat[ij]*pmata[kl] +
             pmatb[ij]*pmat[kl] + pmat[ij]*pmatb[kl]) +
        0.25*(pmata[ij]*pmata[kl] + pmatb[ij]*pmatb[kl] +
              pmata[ij]*pmatb[kl] + pmatb[ij]*pmata[kl]);
    }

    inline double cont2(int ij, int kl) {
      return pmat[ij]*pmat[kl] +
        0.5*(pmata[ij]*pmat[kl] + pmat[ij]*pmata[kl] +
             pmatb[ij]*pmat[kl] + pmat[ij]*pmatb[kl] +
             pmata[ij]*pmata[kl] + pmatb[ij]*pmatb[kl] -
             pmata[ij]*pmatb[kl] - pmatb[ij]*pmata[kl]);
    }
};

#endif

// Local Variables:
// mode: c++
// c-file-style: "ETS"
// End:
