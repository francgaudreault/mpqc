//
// ffo.h
//
// Copyright (C) 1996 Limit Point Systems, Inc.
//
// Author: Ida Nielsen <ibniels@ca.sandia.gov>
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

#ifndef _chemistry_qc_mbpt_ffo_h
#define _chemistry_qc_mbpt_ffo_h

#include <math/dmt/matrix.h>
#include <chemistry/qc/dmtscf/scf_dmt.h>
#include <chemistry/qc/dmtsym/sym_dmt.h>

int
mbpt_ffo(dmt_matrix S,
         scf_struct_t *_scf_info, sym_struct_t *_sym_info, centers_t *_centers,
         dmt_matrix SCF_VEC, dmt_matrix FOCK, dmt_matrix FOCKO);

#endif
