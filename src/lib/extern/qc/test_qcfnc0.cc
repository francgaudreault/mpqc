//
// test_qcfnc0.cc
//
// Copyright (C) 2012 Edward Valeev
//
// Author: Edward Valeev <evaleev@vt.edu>
// Maintainer: EV
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

#include <iostream>
#include <iomanip>
#include <extern/qc/qc_fnc.h>

int
main(int argc, char** argv) {

  const int natoms = 3;
  double *Z = new double[natoms];
  double *geom = new double[natoms*3];
  const int use_symmetry = 1;
  Z[0] = 8.0; geom[0] = 0.0; geom[1] = 0.0; geom[2] = 0.0;
  Z[1] = 1.0; geom[3] = 0.0; geom[4] = 1.0; geom[5] = 1.0;
  Z[2] = 1.0; geom[6] = 0.0; geom[7] =-1.0; geom[8] = 1.0;

  init_molecule_(natoms, Z, geom, use_symmetry);

  init_basis_set_("cc-pVDZ",7);
  const int nshell = basis_set_nshell_();
  const int* shell2nfunction = basis_set_shell_to_nfunction_();
  const int* shell2function = basis_set_shell_to_function_();

  init_integrals_();

  init_overlap_integrals_();
  const double* buffer = overlap_integrals_buffer_();
  std::cout << "overlap integrals:" << std::endl;
  for(int s1=0; s1<nshell; s1++) {
    const int bf1_offset = shell2function[s1];
    const int nbf1 = shell2nfunction[s1];

    for(int s2=0; s2<nshell; s2++) {
      const int bf2_offset = shell2function[s2];
      const int nbf2 = shell2nfunction[s2];

      compute_overlap_shell_(s1, s2);

      int bf12 = 0;
      for(int bf1=0; bf1<nbf1; ++bf1) {
        for(int bf2=0; bf2<nbf2; ++bf2, ++bf12) {
          std::cout << bf1+bf1_offset << " " << bf2+bf2_offset << " "
              << std::setprecision(15) << buffer[bf12] << std::endl;
        }
      }
    }
  }
  done_overlap_integrals_();

  init_hcore_integrals_();
  buffer = hcore_integrals_buffer_();
  std::cout << "hcore integrals:" << std::endl;
  for(int s1=0; s1<nshell; s1++) {
    const int bf1_offset = shell2function[s1];
    const int nbf1 = shell2nfunction[s1];

    for(int s2=0; s2<nshell; s2++) {
      const int bf2_offset = shell2function[s2];
      const int nbf2 = shell2nfunction[s2];

      compute_hcore_shell_(s1, s2);

      int bf12 = 0;
      for(int bf1=0; bf1<nbf1; ++bf1) {
        for(int bf2=0; bf2<nbf2; ++bf2, ++bf12) {
          std::cout << bf1+bf1_offset << " " << bf2+bf2_offset << " "
              << std::setprecision(15) << buffer[bf12] << std::endl;
        }
      }
    }
  }
  done_hcore_integrals_();

  init_twoecoulomb_integrals_();
  buffer = twoecoulomb_integrals_buffer_();
  std::cout << "two-e Coulomb integrals:" << std::endl;
  for(int s1=0; s1<nshell; s1++) {
    const int bf1_offset = shell2function[s1];
    const int nbf1 = shell2nfunction[s1];

    for(int s2=0; s2<nshell; s2++) {
      const int bf2_offset = shell2function[s2];
      const int nbf2 = shell2nfunction[s2];

      for(int s3=0; s3<nshell; s3++) {
        const int bf3_offset = shell2function[s3];
        const int nbf3 = shell2nfunction[s3];

        for(int s4=0; s4<nshell; s4++) {
          const int bf4_offset = shell2function[s4];
          const int nbf4 = shell2nfunction[s4];

          compute_twoecoulomb_shell_(s1, s2, s3, s4);

          int bf1234 = 0;
          for(int bf1=0; bf1<nbf1; ++bf1) {
            for(int bf2=0; bf2<nbf2; ++bf2) {
              for(int bf3=0; bf3<nbf3; ++bf3) {
                for(int bf4=0; bf4<nbf4; ++bf4, ++bf1234) {
                  std::cout << bf1+bf1_offset << " " << bf2+bf2_offset << " "
                      << bf3+bf3_offset << " " << bf4+bf4_offset << " "
                      << std::setprecision(15) << buffer[bf1234] << std::endl;
                }
              }
            }
          }

        }
      }
    }
  }
  done_twoecoulomb_integrals_();

  delete[] geom;
  delete[] Z;
  done_integrals_();

  return 0;
}
