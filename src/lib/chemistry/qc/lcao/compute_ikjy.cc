//
// compute_ikjy.cc
//
// Copyright (C) 2004 Edward Valeev
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

#include <stdexcept>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>

#include <mpqc_config.h>
#include <util/misc/formio.h>
#include <util/misc/regtime.h>
#include <util/class/class.h>
#include <util/state/state.h>
#include <util/state/state_text.h>
#include <util/state/state_bin.h>
#include <math/scmat/matrix.h>
#include <chemistry/qc/mbpt/bzerofast.h>
#include <chemistry/qc/lcao/transform_ikjy.h>
#include <math/scmat/blas.h>
#include <chemistry/qc/lcao/transform_123inds.h>
#include <util/misc/print.h>

using namespace std;
using namespace sc;

#define SINGLE_THREAD_E123   0
#define PRINT3Q 0
#define PRINT4Q 0
#define PRINT_NUM_TE_TYPES 1
#define ALL_TASKS_ON_SAME_NODE 1
#define CHECK_INTS_SYMM 1

/*-------------------------------------
  Based on MBPT2::compute_mp2_energy()
 -------------------------------------*/
void
TwoBodyMOIntsTransform_ikjy::compute()
{
  blasint rank1 = space1_->rank();
  blasint rank2 = space2_->rank();
  blasint rank3 = space3_->rank();
  blasint rank4 = space4_->rank();

  init_acc();
  // if all integrals are already available -- do nothing
  if (restart_orbital_ == rank1)
    return;

  Ref<Integral> integral = factory_->integral();
  Ref<GaussianBasisSet> bs1 = space1_->basis();
  Ref<GaussianBasisSet> bs2 = space2_->basis();
  Ref<GaussianBasisSet> bs3 = space3_->basis();
  Ref<GaussianBasisSet> bs4 = space4_->basis();
  blasint nbasis1 = bs1->nbasis();
  blasint nbasis2 = bs2->nbasis();
  blasint nbasis3 = bs3->nbasis();
  blasint nbasis4 = bs4->nbasis();
  int nshell1 = bs1->nshell();
  int nshell2 = bs2->nshell();
  int nshell3 = bs3->nshell();
  int nshell4 = bs4->nshell();
  int nfuncmax1 = bs1->max_nfunction_in_shell();
  int nfuncmax2 = bs2->max_nfunction_in_shell();
  int nfuncmax3 = bs3->max_nfunction_in_shell();
  int nfuncmax4 = bs4->max_nfunction_in_shell();
  enum te_types {eri=0, r12=1, r12t1=2};
  const size_t memgrp_blocksize = memgrp_blksize();

  // log2 of the erep tolerance
  // (erep < 2^tol => discard)
  const int tol = (int) (-10.0/log10(2.0));  // discard ints smaller than 10^-20

  int aoint_computed = 0;

  std::string tim_label("tbint_tform_");
  tim_label += type(); tim_label += " "; tim_label += name();
  Timer tim(tim_label);

  print_header();

  int me = msg_->me();
  int nproc = msg_->n();
  const int restart_orb = restart_orbital();
  int nijmax = compute_nij(batchsize_,rank3,nproc,me);
  // allocate memory for MemoryGrp
  const size_t localmem = nijmax * num_te_types() * memgrp_blksize();
  alloc_mem(localmem);

  vector<unsigned int> orbsym1 = space1_->orbsym();
  vector<unsigned int> orbsym2 = space2_->orbsym();
  vector<unsigned int> orbsym3 = space3_->orbsym();
  vector<unsigned int> orbsym4 = space4_->orbsym();
  double** vector4 = new double*[nbasis4];
  vector4[0] = new double[rank4*nbasis4];
  for(int i=1; i<nbasis4; i++) vector4[i] = vector4[i-1] + rank4;
  space4_->coefs().convert(vector4);

  /////////////////////////////////////
  //  Begin transformation loops
  /////////////////////////////////////

  // debug print
  if (debug() >= DefaultPrintThresholds::fine) {
    ExEnv::outn() << indent
		  << scprintf("node %i, begin loop over i-batches",me) << endl;
  }
  // end of debug print

  // Initialize the integrals
  integral->set_storage(max_memory_ - localmem);
  integral->set_basis(space1_->basis(),space2_->basis(),space3_->basis(),space4_->basis());
  Ref<TwoBodyInt>* tbints = new Ref<TwoBodyInt>[thr_->nthread()];
  for (int i=0; i<thr_->nthread(); i++) {
    tbints[i] = tbintdescr_->inteval();
  }
  if (debug() >= DefaultPrintThresholds::diagnostics)
    ExEnv::out0() << indent << scprintf("Memory used for integral storage:       %i Bytes",
      integral->storage_used()) << endl;

  Ref<ThreadLock> lock = thr_->new_lock();
  TwoBodyMOIntsTransform_123Inds** e123thread = new TwoBodyMOIntsTransform_123Inds*[thr_->nthread()];
  for (int i=0; i<thr_->nthread(); i++) {
    e123thread[i] = new TwoBodyMOIntsTransform_123Inds(this,i,thr_->nthread(),lock,tbints[i],
                                                       this->log2_epsilon(),debug());
  }

  /*-----------------------------------

    Start the integrals transformation

   -----------------------------------*/
  Timer tim_passes("mp2-r12/a passes");
  if (me == 0 && top_mole_ && top_mole_->if_to_checkpoint()) {
    StateOutBin stateout(top_mole_->checkpoint_file());
    SavableState::save_state(top_mole_,stateout);
    ExEnv::out0() << indent << "Checkpointed the wave function" << endl;
  }

  for (int pass=0; pass<npass_; pass++) {

    ExEnv::out0() << indent << "Beginning pass " << pass+1 << endl;

    int ni = batchsize_;
    int i_offset = restart_orb + pass*ni;
    if (pass == npass_ - 1)
      ni = rank1 - batchsize_*(npass_-1);

    // Compute number of of i,j pairs on each node during current pass for
    // two-el integrals
    int nij = compute_nij(ni,rank3,nproc,me);;

    // debug print
    if (debug() >= DefaultPrintThresholds::fine)
      ExEnv::outn() << indent << "node " << me << ", nij = " << nij << endl;
    // end of debug print

    // Allocate and initialize some arrays
    // (done here to avoid having these arrays
    // overlap with arrays allocated later)

    // Allocate (and initialize) some arrays

    double* integral_ijsx = (double*) mem_->localdata();
    //bzerofast(integral_ijsx, (num_te_types()*nij*memgrp_blocksize));
    memset(integral_ijsx, 0, num_te_types()*nij*memgrp_blocksize);
    integral_ijsx = 0;
    mem_->sync();
    ExEnv::out0() << indent
		  << scprintf("Begin loop over shells (ints, 1+2+3 q.t.)") << endl;

    // Do the two electron integrals and the first three quarter transformations
    Timer tim123("ints+1qt+2qt+3qt");
    shell_pair_data()->init();
    for (int i=0; i<thr_->nthread(); i++) {
      e123thread[i]->set_i_offset(i_offset);
      e123thread[i]->set_ni(ni);
      thr_->add_thread(i,e123thread[i]);
#     if SINGLE_THREAD_E123
      e123thread[i]->run();
#     endif
    }
#   if !SINGLE_THREAD_E123
    thr_->start_threads();
    thr_->wait_threads();
#   endif
    tim123.exit();
    ExEnv::out0() << indent << "End of loop over shells" << endl;

    mem_->sync();  // Make sure ijsx is complete on each node before continuing
    integral_ijsx = (double*) mem_->localdata();

#if PRINT3Q
    {
      // each task take its turn to write to the file, in case all tasks live on the same node
      for (int proc = 0; proc < nproc; ++proc) {
        if (me == proc) { // my turn to write

          string filename = type() + "." + name_ + ".3q.dat";
          ios_base::openmode mode = ios_base::trunc;
          if (pass > 0 || ALL_TASKS_ON_SAME_NODE)
            mode = ios_base::app;
          ofstream ints_file(filename.c_str(), mode);

          const int ntetypes = std::min((int) num_te_types(),
                                        PRINT_NUM_TE_TYPES);
          for (int te_type = 0; te_type < ntetypes; te_type++) {
            for (int i = 0; i < ni; i++) {
              for (int j = 0; j < rank3; j++) {
                int ij = i * rank3 + j;
                int ij_local = ij / nproc;
                if (ij % nproc != me)
                  continue;
                for (int x = 0; x < rank2; x++) {
                  const double* ijsx_ints =
                      (const double*) ((size_t) integral_ijsx + (ij_local
                          * num_te_types() + te_type) * memgrp_blocksize);
                  for (int s = 0; s < nbasis4; s++) {
                    double value = ijsx_ints[s * rank2 + x];
                    ints_file << scprintf("3Q: type = %d |(%d %d|%d %d)| = %12.8f\n",
                                          te_type, i + i_offset, x, j, s, fabs(value));
                  }
                }
              }
            }
          }

          ints_file.close();
        }
        msg_->sync();
      }
    }
#endif

    // Fourth quarter transform
    ExEnv::out0() << indent << "Begin fourth q.t." << endl;
    Timer tim4("4. q.t.");
    // Begin fourth quarter transformation;
    // generate (ix|jy) stored as ijxy

    double* ijxy_ints = new double[rank2*rank4];
    const size_t xy_size = rank2*rank4*sizeof(double);
    for (int i = 0; i<ni; i++) {
      for (int j = 0; j<rank3; j++) {
        int ij = i*rank3+j;
        int ij_local = ij/nproc;
        if (ij%nproc == me) {

          for(int te_type=0; te_type<num_te_types(); te_type++) {
            const double *sx_ptr = (const double*) ((size_t)integral_ijsx + (ij_local*num_te_types()+te_type)*memgrp_blocksize);

            // fourth quarter transform
            // yx = ys * sx
            const char notransp = 'n';
            const char transp = 't';
            const double one = 1.0;
            const double zero = 0.0;
            F77_DGEMM(&notransp,&transp,&rank4,&rank2,&nbasis4,&one,vector4[0],&rank4,
                      sx_ptr,&rank2,&zero,ijxy_ints,&rank4);

            // copy the result back to integrals_ijsx
            memcpy((void*)sx_ptr,(const void*)ijxy_ints,xy_size);
          }
        }
      }
    }
    delete[] ijxy_ints;
    tim4.exit();
    ExEnv::out0() << indent << "End of fourth q.t." << endl;

    integral_ijsx = 0;
    double* integral_ijxy = (double*) mem_->localdata();

    // Zero out nonsymmetric integrals -- Pitzer theorem in action
    {
      for (int i = 0; i<ni; i++) {
        for (int j = 0; j<rank3; j++) {
          int ij = i*rank3+j;
          int ij_local = ij/nproc;
          if (ij%nproc == me) {
            const int ij_sym = orbsym1[i+i_offset] ^ orbsym3[j];
            for(int te_type=0; te_type<num_te_types(); te_type++) {
              double* ijxy_ptr = (double *)((size_t)integral_ijxy + (ij_local*num_te_types()+te_type)*memgrp_blocksize);
              for (int x = 0; x<rank2; x++) {
                const int ijx_sym = ij_sym ^ orbsym2[x];
                for (int y = 0; y<rank4; y++, ijxy_ptr++) {
                  if (ijx_sym ^ orbsym4[y]) {
                    *ijxy_ptr = 0.0;
                  }
                }
              }
            }
          }
        }
      }
    }
    // Sync up tasks before integrals are committed
    msg_->sync();

#if PRINT4Q
    {

      // each task take its turn to write to the file, in case all tasks live on the same node
      for (int proc = 0; proc < nproc; ++proc) {
        if (me == proc) { // my turn to write

          string filename = type() + "." + name_ + ".4q.dat";
          ios_base::openmode mode = ios_base::trunc;
          if (pass > 0 || ALL_TASKS_ON_SAME_NODE)
            mode = ios_base::app;
          ofstream ints_file(filename.c_str(), mode);

          const int ntetypes = std::min((int) num_te_types(),
                                        PRINT_NUM_TE_TYPES);
          for (int te_type = 0; te_type < ntetypes; te_type++) {
            for (int i = 0; i < ni; i++) {
              for (int j = 0; j < rank3; j++) {
                int ij = i * rank3 + j;
                int ij_local = ij / nproc;
                if (ij % nproc != me)
                  continue;
                for (int x = 0; x < rank2; x++) {
                  const double* ijxy_ints =
                      (const double*) ((size_t) integral_ijxy + (ij_local
                          * num_te_types() + te_type) * memgrp_blocksize);
                  for (int y = 0; y < rank4; y++) {
                    double value = ijxy_ints[x * rank4 + y];
                    ints_file << scprintf("4Q: type = %d |(%d %d|%d %d)| = %12.8f\n",
                                          te_type, i + i_offset, x, j, y, fabs(value));
                  }
                }
              }
            }
          }

          ints_file.close();
        }
        msg_->sync(); // all tasks wait the end of each turn
      }
    }
#endif

    // Push locally stored integrals to an accumulator
    Timer tim_intstore("MO ints store");
    ints_acc_->activate();
    detail::store_memorygrp(ints_acc_,mem_,restart_orbital_,ni,memgrp_blocksize);
    if (ints_acc_->data_persistent()) ints_acc_->deactivate();
    // if didn't throw can safely update the counter
    restart_orbital_ += ni;
    tim_intstore.exit();
    mem_->sync();

    if (me == 0 && top_mole_ && top_mole_->if_to_checkpoint()) {
      StateOutBin stateout(top_mole_->checkpoint_file());
      SavableState::save_state(top_mole_,stateout);
      ExEnv::out0() << indent << "Checkpointed the wave function" << endl;
    }

  } // end of loop over passes
  tim_passes.exit();

  for (int i=0; i<thr_->nthread(); i++) {
    delete e123thread[i];
  }
  delete[] e123thread;
  delete[] tbints; tbints = 0;
  delete[] vector4[0]; delete[] vector4;

  tim.exit();

  if (me == 0 && top_mole_ && top_mole_->if_to_checkpoint()) {
    StateOutBin stateout(top_mole_->checkpoint_file());
    SavableState::save_state(top_mole_,stateout);
    ExEnv::out0() << indent << "Checkpointed the wave function" << endl;
  }

  print_footer();

#if CHECK_INTS_SYMM
  ExEnv::out0() << indent << "Detecting non-totally-symmetric integrals ... ";
  check_int_symm();
  ExEnv::out0() << "none" << endl;
#endif

  // memory used by MemoryGrp can now be purged unless ints_acc_ uses it
  if (ints_acc_->data_persistent()) dealloc_mem();

}


////////////////////////////////////////////////////////////////////////////

// Local Variables:
// mode: c++
// c-file-style: "CLJ-CONDENSED"
// End:
