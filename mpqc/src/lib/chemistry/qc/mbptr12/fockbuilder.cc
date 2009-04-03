//
// fockbuilder.cc
//
// Copyright (C) 2009 Edward Valeev
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

#ifdef __GNUG__
#pragma implementation
#endif

#include <cfloat>
#include<chemistry/qc/basis/symmint.h>
#include<chemistry/qc/basis/orthog.h>
#include<chemistry/qc/mbptr12/blas.h>
#include<chemistry/qc/mbptr12/fockbuilder.h>

using namespace sc;

namespace sc {
  namespace detail {

    RefSymmSCMatrix overlap(const Ref<GaussianBasisSet>& bas,
                            const Ref<Integral>& integral) {

      Ref<Integral> localints = integral->clone();
      localints->set_basis(bas);
      Ref<PetiteList> pl = localints->petite_list();

      // form skeleton overlap in AO basis
      RefSymmSCMatrix sao(bas->basisdim(), bas->matrixkit());
      sao.assign(0.0);
      Ref<SCElementOp> sc =
          new OneBodyIntOp(new SymmOneBodyIntIter(localints->overlap(), pl));
      sao.element_op(sc);
      sc = 0;

      // now symmetrize Sso
      RefSymmSCMatrix s(pl->SO_basisdim(), bas->so_matrixkit());
      pl->symmetrize(sao, s);

      return s;
    }

    RefSCMatrix overlap(const Ref<GaussianBasisSet>& brabas,
                        const Ref<GaussianBasisSet>& ketbas,
                        const Ref<Integral>& integral) {

      Ref<Integral> localints = integral->clone();
      Ref<GPetiteList2> pl12 = GPetiteListFactory::plist2(brabas,ketbas);
      localints->set_basis(brabas,ketbas);

      // form overlap in AO basis
      RefSCMatrix sao(brabas->basisdim(), ketbas->basisdim(), brabas->matrixkit());
      sao.assign(0.0);

      abort();
    }

    RefSymmSCMatrix nonrelativistic(const Ref<GaussianBasisSet>& bas,
                                    const Ref<Integral>& integral) {

      Ref<Integral> localints = integral->clone();
      localints->set_basis(bas);
      Ref<PetiteList> pl = localints->petite_list();

      // form skeleton Hcore in AO basis
      RefSymmSCMatrix hao(bas->basisdim(), bas->matrixkit());
      hao.assign(0.0);

      // kinetic energy
      Ref<SCElementOp> hc =
          new OneBodyIntOp(new SymmOneBodyIntIter(localints->kinetic(), pl));
      hao.element_op(hc);
      hc = 0;

      // molecular potential
      Ref<OneBodyInt> nuc = localints->nuclear();
      nuc->reinitialize();
      hc = new OneBodyIntOp(new SymmOneBodyIntIter(nuc, pl));
      hao.element_op(hc);
      hc = 0;

      // now symmetrize Hso
      RefSymmSCMatrix h(pl->SO_basisdim(), bas->so_matrixkit());
      pl->symmetrize(hao, h);

      return h;
    }

    RefSymmSCMatrix twobody_twocenter_coulomb(const Ref<GaussianBasisSet>& bas,
                                              const Ref<Integral>& integral) {

      Ref<Integral> localints = integral->clone();
      localints->set_basis(bas);
      Ref<PetiteList> pl = localints->petite_list();

      // form skeleton 2-center Coulomb in AO basis
      RefSymmSCMatrix ao(bas->basisdim(), bas->matrixkit());
      ao.assign(0.0);
      Ref<SCElementOp> sc =
          new TwoBodyTwoCenterIntOp(new SymmTwoBodyTwoCenterIntIter(localints->electron_repulsion2(),
                                                                    pl));
      ao.element_op(sc);
      sc = 0;

      // now symmetrize
      RefSymmSCMatrix result(pl->SO_basisdim(), bas->so_matrixkit());
      pl->symmetrize(ao, result);

      return result;
    }

    RefSymmSCMatrix pauli(const Ref<GaussianBasisSet>& bas,
                          const Ref<Integral>& integral) {
      const double c = 137.0359895; // speed of light in a vacuum in a.u.

      Ref<Integral> localints = integral->clone();
      localints->set_basis(bas);
      Ref<PetiteList> pl = localints->petite_list();

      // form skeleton Hcore in AO basis
      RefSymmSCMatrix hao(bas->basisdim(), bas->matrixkit());
      hao.assign(0.0);

      // mass-velocity
      Ref<OneBodyInt> mv = localints->p4();
      mv->reinitialize();
      Ref<SCElementOp> hc = new OneBodyIntOp(new SymmOneBodyIntIter(mv, pl));
      RefSymmSCMatrix mv_ao(bas->basisdim(), bas->matrixkit());
      mv_ao.assign(0.0);
      mv_ao.element_op(hc);
      mv_ao.scale((-1.0) / (8.0 * c * c));
      hao.accumulate(mv_ao);
      mv_ao = 0;
      hc = 0;

      // Darwin term
      RefSymmSCMatrix Darwin(bas->basisdim(), bas->matrixkit());
      Darwin.assign(0.0);
      {
        const double darwin_prefac = M_PI / (c * c * 2.0);

        GaussianBasisSet::ValueData* vdata1 =
            new GaussianBasisSet::ValueData(bas, localints);
        const int nbasis1 = bas->nbasis();
        double* values1 = new double[nbasis1];
        const Ref<Molecule>& molecule = bas->molecule();
        const int natom = molecule->natom();

        for (int iatom = 0; iatom < natom; iatom++) {

          SCVector3 R_iatom = molecule->r(iatom);
          // this puts values of basis functions evaluated at R_iatom into values1
          bas->values(R_iatom, vdata1, values1);
          const double prefac = darwin_prefac * molecule->charge(iatom);

          for (int ibasis = 0; ibasis < bas->nbasis(); ibasis++) {
            for (int jbasis = 0; jbasis <= ibasis; jbasis++) {

              const double d = values1[ibasis] * values1[jbasis] * prefac;
              Darwin.accumulate_element(ibasis, jbasis, d);
            }
          }
        }
        delete[] values1;
        delete vdata1;
      }
      hao.accumulate(Darwin);

      // now symmetrize Hso
      RefSymmSCMatrix h(pl->SO_basisdim(), bas->so_matrixkit());
      pl->symmetrize(hao, h);

      // add the nonrelativistic contribution
      RefSymmSCMatrix hnr = nonrelativistic(bas, integral);
      h->accumulate(hnr);

      return h;
    }

    namespace {

      void dk2_contrib(const RefSymmSCMatrix &h_pbas,
                       const RefDiagSCMatrix &E,
                       const RefDiagSCMatrix &K,
                       const RefDiagSCMatrix &p2,
                       const RefDiagSCMatrix &p2K2,
                       const RefDiagSCMatrix &p2K2_inv,
                       const RefSymmSCMatrix &AVA_pbas,
                       const RefSymmSCMatrix &BpVpB_pbas) {
        RefSCDimension pdim = AVA_pbas.dim();

        RefSymmSCMatrix AVA_prime = AVA_pbas.clone();
        RefSymmSCMatrix BpVpB_prime = BpVpB_pbas.clone();
        int npbas = pdim.n();
        for (int i=0; i<npbas; i++) {
          double Ei = E(i);
          for (int j=0; j<=i; j++) {
            double Ej = E(j);
            AVA_prime(i,j) = AVA_pbas(i,j)/(Ei+Ej);
            BpVpB_prime(i,j) = BpVpB_pbas(i,j)/(Ei+Ej);
          }
        }

        RefSCMatrix h_contrib;
        h_contrib
          =
          - 1.0 * BpVpB_prime * E * AVA_prime
          - 0.5 * BpVpB_prime * AVA_prime * E
          - 0.5 * AVA_prime * BpVpB_prime * E
          + 0.5 * AVA_prime * p2K2 * AVA_prime * E
          + 0.5 * BpVpB_prime * p2K2_inv * BpVpB_prime * E
          + 0.5 * AVA_prime * (p2K2 * E) * AVA_prime
          + 0.5 * BpVpB_prime * (p2K2_inv * E) * BpVpB_prime
          ;

        h_pbas.accumulate_symmetric_sum(h_contrib);
      }

      void dk3_contrib(const RefSymmSCMatrix &h_pbas,
                       const RefDiagSCMatrix &E,
                       const RefDiagSCMatrix &B,
                       const RefDiagSCMatrix &p2K2_inv,
                       const RefSCMatrix &so_to_p,
                       const RefSymmSCMatrix &pxVp) {
        RefSCDimension p_oso_dim = so_to_p.rowdim();
        Ref<SCMatrixKit> p_so_kit = so_to_p.kit();
        int noso = p_oso_dim.n();
        RefSymmSCMatrix pxVp_pbas(p_oso_dim, p_so_kit);
        pxVp_pbas.assign(0.0);
        pxVp_pbas.accumulate_transform(so_to_p, pxVp);

        RefSymmSCMatrix BpxVpB_prime(p_oso_dim, p_so_kit);
        for (int i=0; i<noso; i++) {
          double Ei = E(i);
          for (int j=0; j<=i; j++) {
            double Ej = E(j);
            BpxVpB_prime(i,j) = pxVp_pbas(i,j)*B(i)*B(j)/(Ei+Ej);
          }
        }

        RefSCDimension pdim = E.dim();

        RefSCMatrix h_contrib;
        h_contrib
          =
          - 0.5 * BpxVpB_prime * E * p2K2_inv * BpxVpB_prime
          - 0.5 * BpxVpB_prime * p2K2_inv * BpxVpB_prime * E
          ;

        h_pbas.accumulate_symmetric_sum(h_contrib);
      }

    } // anonymous namespace

    RefSymmSCMatrix dk(int dklev, const Ref<GaussianBasisSet>& bas,
                       const Ref<GaussianBasisSet>& p_bas,
                       const Ref<Integral>& integral) {

#define DK_DEBUG 0

      ExEnv::out0() << indent << "Including DK" << dklev
          << (dklev == 1 ? " (free particle projection)" : "")
          << (dklev == 2 ? " (Douglas-Kroll-Hess)" : "")
          << (dklev == 3 ? " (complete spin-free Douglas-Kroll)" : "")
          << " terms in the one body Hamiltonian." << std::endl;

      if (dklev > 2) {
        throw FeatureNotImplemented("dklev must be 0, 1, or 2", __FILE__, __LINE__);
      }

      Ref<Integral> localints = integral->clone();
      // The one electron integrals will be computed in the momentum basis.
      localints->set_basis(p_bas);

      Ref<PetiteList> p_pl = localints->petite_list();

      RefSCDimension p_so_dim = p_pl->SO_basisdim();
      RefSCDimension p_ao_dim = p_pl->AO_basisdim();
      Ref<SCMatrixKit> p_kit = p_bas->matrixkit();
      Ref<SCMatrixKit> p_so_kit = p_bas->so_matrixkit();

      // Compute the overlap in the momentum basis.
      RefSymmSCMatrix S_skel(p_ao_dim, p_kit);
      S_skel.assign(0.0);
      Ref<SCElementOp> hc =
          new OneBodyIntOp(new SymmOneBodyIntIter(localints->overlap(), p_pl));
      S_skel.element_op(hc);
      hc = 0;
      RefSymmSCMatrix S(p_so_dim, p_so_kit);
      p_pl->symmetrize(S_skel, S);

      ExEnv::out0() << indent << "The momentum basis is:" << std::endl;
      ExEnv::out0() << incindent;
      p_bas->print_brief(ExEnv::out0());
      ExEnv::out0() << decindent;

      ExEnv::out0() << indent << "Orthogonalizing the momentum basis"
          << std::endl;
      const int debug = 0;
      Ref<OverlapOrthog> p_orthog = new OverlapOrthog(OverlapOrthog::default_orthog_method(), S,
                                                      p_so_kit, OverlapOrthog::default_lindep_tol(),
                                                      debug);

      RefSCDimension p_oso_dim = p_orthog->orthog_dim();

      // form skeleton Hcore in the momentum basis
      RefSymmSCMatrix T_skel(p_ao_dim, p_kit);
      T_skel.assign(0.0);

      hc = new OneBodyIntOp(new SymmOneBodyIntIter(localints->kinetic(), p_pl));
      T_skel.element_op(hc);
      hc = 0;

      // finish constructing the kinetic energy integrals,
      // for which the skeleton is in hao
      RefSymmSCMatrix T(p_so_dim, p_so_kit);
      p_pl->symmetrize(T_skel, T);
      T_skel = 0;

      // Transform T into an orthogonal basis
      RefSymmSCMatrix T_oso(p_oso_dim, p_so_kit);
      T_oso.assign(0.0);
      T_oso.accumulate_transform(p_orthog->basis_to_orthog_basis(), T);

      // diagonalize the T integrals to get a momentum basis
      RefDiagSCMatrix Tval(p_oso_dim, p_so_kit);
      RefSCMatrix Tvec(p_oso_dim, p_oso_dim, p_so_kit);
      // Tvec * Tval * Tvec.t() = T_oso
      T_oso.diagonalize(Tval, Tvec);

      T_oso = 0;

#if DK_DEBUG
      T.print("T");
      Tval.print("Tval");
#endif

      // Compute the kinematic factors
      RefDiagSCMatrix A(p_oso_dim, p_so_kit);
      RefDiagSCMatrix B(p_oso_dim, p_so_kit);
      RefDiagSCMatrix E(p_oso_dim, p_so_kit);
      RefDiagSCMatrix K(p_oso_dim, p_so_kit);
      RefDiagSCMatrix p2(p_oso_dim, p_so_kit);
      RefDiagSCMatrix Emc2(p_oso_dim, p_so_kit);
      const double c = 137.0359895; // speed of light in a vacuum in a.u.
      int noso = p_oso_dim.n();
      for (int i = 0; i < noso; i++) {
        double T_val = Tval(i);
        // momentum basis sets with near linear dependencies may
        // have T_val approximately equal to zero.  These can be
        // negative, which will cause a SIGFPE in sqrt.
        if (T_val < DBL_EPSILON)
          T_val = 0.0;
        double p = sqrt(2.0 * T_val);
        double E_val = c * sqrt(p * p + c * c);
        double A_val = sqrt((E_val + c * c) / (2.0 * E_val));
        double K_val = c / (E_val + c * c);
        double B_val = A_val * K_val;
        double Emc2_val = c * c * p * p / (E_val + c * c); // = E - mc^2
        A( i) = A_val;
        B( i) = B_val;
        E( i) = E_val;
        K( i) = K_val;
        p2( i) = p * p;
        Emc2( i) = Emc2_val;
      }

#if DK_DEBUG
      A.print("A");
      B.print("B");
      E.print("E");
      K.print("K");
      Emc2.print("Emc2");
#endif

      // Construct the transform from the coordinate to the momentum
      // representation in the momentum basis
      RefSCMatrix so_to_p = Tvec.t() * p_orthog->basis_to_orthog_basis();

#if DK_DEBUG
      so_to_p.print("so_to_p");
#endif

      // compute the V integrals
      Ref<OneBodyInt> V_obi = localints->nuclear();
      V_obi->reinitialize();
      hc = new OneBodyIntOp(new SymmOneBodyIntIter(V_obi, p_pl));
      RefSymmSCMatrix V_skel(p_ao_dim, p_kit);
      V_skel.assign(0.0);
      V_skel.element_op(hc);
      V_obi = 0;
      hc = 0;
      RefSymmSCMatrix V(p_so_dim, p_so_kit);
      p_pl->symmetrize(V_skel, V);
      V_skel = 0;

#if DK_DEBUG
      V.print("V");
#endif

      // transform V to the momentum basis
      RefSymmSCMatrix V_pbas(p_oso_dim, p_so_kit);
      V_pbas.assign(0.0);
      V_pbas.accumulate_transform(so_to_p, V);

      // compute the p.Vp integrals
      Ref<OneBodyInt> pVp_obi = localints->p_dot_nuclear_p();
      hc = new OneBodyIntOp(new SymmOneBodyIntIter(pVp_obi, p_pl));
      RefSymmSCMatrix pVp_skel(p_ao_dim, p_kit);
      pVp_skel.assign(0.0);
      pVp_skel.element_op(hc);
#if DK_DEBUG
      const double *buf = pVp_obi->buffer();
      for (int I=0,Ii=0; I<p_bas->nshell(); I++) {
        for (int i=0; i<p_bas->shell(I).nfunction(); i++,Ii++) {
          for (int J=0,Jj=0; J<p_bas->nshell(); J++) {
            pVp_obi->compute_shell(I,J);
            int ij = i*p_bas->shell(J).nfunction();
            for (int j=0; j<p_bas->shell(J).nfunction(); j++,ij++,Jj++) {
              std::cout << "pVp["<<Ii<<"]["<<Jj<<"][0]= " << buf[ij]
              << std::endl;
            }
          }
        }
      }
#endif
      pVp_obi = 0;
      hc = 0;
      RefSymmSCMatrix pVp(p_so_dim, p_so_kit);
      p_pl->symmetrize(pVp_skel, pVp);
      pVp_skel = 0;

#if DK_DEBUG
      pVp.print("pVp");
      (-2.0*T).print("-2*T");
#endif

      // transform p.Vp to the momentum basis
      RefSymmSCMatrix pVp_pbas(p_oso_dim, p_so_kit);
      pVp_pbas.assign(0.0);
      pVp_pbas.accumulate_transform(so_to_p, pVp);

      RefSymmSCMatrix AVA_pbas(p_oso_dim, p_so_kit);
      RefSymmSCMatrix BpVpB_pbas(p_oso_dim, p_so_kit);
      for (int i = 0; i < noso; i++) {
        for (int j = 0; j <= i; j++) {
          AVA_pbas(i, j) = V_pbas(i, j) * A(i) * A(j);
          BpVpB_pbas(i, j) = pVp_pbas(i, j) * B(i) * B(j);
        }
      }

      V_pbas = 0;
      pVp_pbas = 0;

      // form the momentum basis hamiltonian
      RefSymmSCMatrix h_pbas(p_oso_dim, p_so_kit);
      h_pbas = AVA_pbas + BpVpB_pbas;

      // Add the kinetic energy
      for (int i = 0; i < noso; i++) {
        h_pbas(i, i) = h_pbas(i, i) + Emc2(i);
      }

      if (dklev > 1) {
        RefDiagSCMatrix p2K2 = p2 * K * K;
        RefDiagSCMatrix p2K2_inv = p2K2->clone();

        for (int i = 0; i < noso; i++) {
          double p2K2_val = p2K2(i);
          if (fabs(p2K2_val) > DBL_EPSILON)
            p2K2_inv( i) = 1.0 / p2K2_val;
          else
            p2K2_inv( i) = 0.0;
        }

        dk2_contrib(h_pbas, E, K, p2, p2K2, p2K2_inv, AVA_pbas, BpVpB_pbas);

        if (dklev > 2) {
          Ref<OneBodyInt> pxVp_obi = localints->p_cross_nuclear_p();
          Ref<SCElementOp3> hc3;
          hc3 = new OneBody3IntOp(new SymmOneBodyIntIter(pxVp_obi, p_pl));
          RefSymmSCMatrix pxVp_x_skel(p_ao_dim, p_kit);
          RefSymmSCMatrix pxVp_y_skel(p_ao_dim, p_kit);
          RefSymmSCMatrix pxVp_z_skel(p_ao_dim, p_kit);
          pxVp_x_skel.assign(0.0);
          pxVp_y_skel.assign(0.0);
          pxVp_z_skel.assign(0.0);
          pxVp_x_skel.element_op(hc3, pxVp_y_skel, pxVp_z_skel);
          RefSymmSCMatrix pxVp_x(p_so_dim, p_so_kit);
          RefSymmSCMatrix pxVp_y(p_so_dim, p_so_kit);
          RefSymmSCMatrix pxVp_z(p_so_dim, p_so_kit);
          p_pl->symmetrize(pxVp_x_skel, pxVp_x);
          p_pl->symmetrize(pxVp_y_skel, pxVp_y);
          p_pl->symmetrize(pxVp_z_skel, pxVp_z);
          pxVp_x_skel = 0;
          pxVp_y_skel = 0;
          pxVp_z_skel = 0;

          dk3_contrib(h_pbas, E, B, p2K2_inv, so_to_p, pxVp_x);
          dk3_contrib(h_pbas, E, B, p2K2_inv, so_to_p, pxVp_y);
          dk3_contrib(h_pbas, E, B, p2K2_inv, so_to_p, pxVp_z);
        }

      }

#if DK_DEBUG
      h_pbas.print("h_pbas");
#endif

      AVA_pbas = 0;
      BpVpB_pbas = 0;
      A = 0;
      B = 0;
      E = 0;
      K = 0;
      Emc2 = 0;

      // Construct the transform from the momentum representation to the
      // coordinate representation in the momentum basis
      RefSCMatrix p_to_so = p_orthog->basis_to_orthog_basis_inverse() * Tvec;

      // Construct the transform from the momentum basis to the
      // coordinate basis.
      localints->set_basis(bas, p_bas);
      Ref<PetiteList> pl = localints->petite_list();
      RefSCMatrix S_ao_p(pl->AO_basisdim(), p_ao_dim, p_kit);
      S_ao_p.assign(0.0);
      hc = new OneBodyIntOp(localints->overlap());
      S_ao_p.element_op(hc);
      hc = 0;
      // convert s_ao_p into the so ao and so p basis
      RefSCMatrix blocked_S_ao_p(pl->AO_basisdim(), p_pl->AO_basisdim(),
                                 p_so_kit);
      blocked_S_ao_p->convert(S_ao_p);
      RefSCMatrix S_ao_p_so_l = pl->sotoao() * blocked_S_ao_p;
      RefSCMatrix S_ao_p_so = S_ao_p_so_l * p_pl->aotoso();
      S_ao_p_so_l = 0;

      // transform h_pbas back to the so basis
      RefSymmSCMatrix h_dk_so(pl->SO_basisdim(), bas->so_matrixkit());
      h_dk_so.assign(0.0);
      h_dk_so.accumulate_transform(S_ao_p_so * p_orthog->overlap_inverse()
          * p_to_so, h_pbas);

      // Compute the overlap in bas
      localints->set_basis(bas);
      RefSymmSCMatrix S_bas;
      {
        Ref<SCMatrixKit> kit = bas->matrixkit();
        Ref<SCMatrixKit> so_kit = bas->so_matrixkit();
        RefSCDimension so_dim = pl->SO_basisdim();
        RefSCDimension ao_dim = pl->AO_basisdim();
        RefSymmSCMatrix S_skel(ao_dim, kit);
        S_skel.assign(0.0);
        hc
            = new OneBodyIntOp(
                               new SymmOneBodyIntIter(localints->overlap(), pl));
        S_skel.element_op(hc);
        hc = 0;
        S_bas = so_kit->symmmatrix(so_dim);
        pl->symmetrize(S_skel, S_bas);
      }

      localints->set_basis(bas);

#if DK_DEBUG
      {
        RefSCMatrix tmp = S_ao_p_so * p_orthog->overlap_inverse() * S_ao_p_so.t();
        tmp.print("S(OBS,pbasis) * S(pbasis)^-1 * S(pbasis,OBS)");
        ExEnv::out0() << indent << " trace = " << tmp.trace() << endl;
        S_bas.print("S(OBS)");

        ExEnv::out0() << indent << " trace = " << S_bas.trace() << endl;

        ExEnv::out0() << indent << "nso = " << pl->SO_basisdim()->n() << endl;
      }
#endif

      // Check to see if the momentum basis spans the coordinate basis.  The
      // following approach seems reasonable, but a more careful mathematical
      // analysis would be desirable.
      const double S_ao_projected_trace = (S_ao_p_so
          * p_orthog->overlap_inverse() * S_ao_p_so.t()).trace()
          / pl->SO_basisdim()->n();
      const double S_ao_trace = S_bas.trace() / pl->SO_basisdim()->n();
      const double completeness_diagnostic = S_ao_projected_trace / S_ao_trace;
      ExEnv::out0() << indent
          << "Tr(basis overlap projected into momentum basis)/Tr(basis overlap) = "
          << completeness_diagnostic << std::endl;
      if (fabs(1.0 - completeness_diagnostic) > OverlapOrthog::default_lindep_tol()) {
        ExEnv::out0() << indent
            << "WARNING: the momentum basis does not span the orbital basis"
            << std::endl;
      }

#if DK_DEBUG
      S_ao_p_so.print("S_ao_p_so");
      p_to_so.print("p_to_so");
      //(p_to_so*so_to_p).print("p_to_so*so_to_p");
      (S_ao_p_so*S.gi()*p_to_so).print("S_ao_p_so*S.gi()*p_to_so");
#endif

#if DK_DEBUG
      (T+V).print("T+V");
      h_dk_so.print("h_dk_so");
#endif

      return h_dk_so;

    }

    RefSCMatrix coulomb(const Ref<TwoBodyFourCenterMOIntsRuntime>& ints_rtime,
                        const Ref<OrbitalSpace>& occ_space,
                        const Ref<OrbitalSpace>& bra_space,
                        const Ref<OrbitalSpace>& ket_space) {

      Ref<MessageGrp> msg = MessageGrp::get_default_messagegrp();

      Timer tim_coulomb("coulomb");

      int me = msg->me();
      int nproc = msg->n();
      ExEnv::out0() << endl << indent
               << "Entered Coulomb matrix evaluator" << endl;
      ExEnv::out0() << incindent;

      // Only need 1/r12 integrals. In principle, almost any Descr will do. For now ask for ERI
      const std::string tform_key = ParsedTwoBodyFourCenterIntKey::key(occ_space->id(),bra_space->id(),
                                                             occ_space->id(),ket_space->id(),
                                                             std::string("ERI"),
                                                             std::string(TwoBodyIntLayout::b1k1_b2k2));
      Ref<TwoBodyMOIntsTransform> mnxy_tform = ints_rtime->get(tform_key);
      Ref<R12IntsAcc> mnxy_acc = mnxy_tform->ints_acc();

      const int nocc = occ_space->rank();
      const int nbra = bra_space->rank();
      const int nket = ket_space->rank();
      const int nbraket = nbra*nket;

      ExEnv::out0() << indent << "Begin computation of Coulomb matrix" << endl;

      // Compute the number of tasks that have full access to the integrals
      // and split the work among them
      vector<int> proc_with_ints;
      int nproc_with_ints = mnxy_acc->tasks_with_access(proc_with_ints);

      //////////////////////////////////////////////////////////////
      //
      // Evaluation of the coulomb matrix proceeds as follows:
      //
      //    loop over batches of mm, 0<=m<nocc
      //      load (mmxy)=(xm|my) into memory
      //
      //      loop over xy, 0<=x<nbra, 0<=y<nket
      //        compute K[x][y] += (mmxy)
      //      end xy loop
      //    end mm loop
      //
      /////////////////////////////////////////////////////////////////////////////////

      double* J_xy = new double[nbraket];
      memset(J_xy,0,nbraket*sizeof(double));
      Timer tim_mo_ints_retrieve;
      tim_mo_ints_retrieve.set_default("MO ints retrieve");
      if (mnxy_acc->has_access(me)) {

        for(int m=0; m<nocc; m++) {

          const int mm = m*nocc+m;
          const int mm_proc = mm%nproc_with_ints;
          if (mm_proc != proc_with_ints[me])
            continue;

          // Get (|1/r12|) integrals
          tim_mo_ints_retrieve.enter_default();
          const double *mmxy_buf_eri = mnxy_acc->retrieve_pair_block(m,m,TwoBodyOper::eri);
          tim_mo_ints_retrieve.exit_default();

          const double one = 1.0;
          const int unit_stride = 1;
          F77_DAXPY(&nbraket,&one,mmxy_buf_eri,&unit_stride,J_xy,&unit_stride);

          mnxy_acc->release_pair_block(m,m,TwoBodyOper::eri);
        }
      }

      ExEnv::out0() << indent << "End of computation of Coulomb matrix" << endl;

      msg->sum(J_xy,nbraket);

      RefSCMatrix J(bra_space->coefs()->coldim(), ket_space->coefs()->coldim(), bra_space->coefs()->kit());
      J.assign(J_xy);
      delete[] J_xy;

      ExEnv::out0() << decindent;
      ExEnv::out0() << indent << "Exited Coulomb matrix evaluator" << endl;
      tim_coulomb.exit();

      return J;
    }

    RefSCMatrix exchange(const Ref<TwoBodyFourCenterMOIntsRuntime>& ints_rtime,
                         const Ref<OrbitalSpace>& occ_space,
                         const Ref<OrbitalSpace>& bra_space,
                         const Ref<OrbitalSpace>& ket_space) {

      Ref<MessageGrp> msg = MessageGrp::get_default_messagegrp();

      Timer tim_exchange("exchange");

      int me = msg->me();
      int nproc = msg->n();
      ExEnv::out0() << endl << indent
               << "Entered exchange matrix evaluator" << endl;
      ExEnv::out0() << incindent;

      // Only need 1/r12 integrals. In principle, almost any Descr will do. For now ask for ERI
      const std::string tform_key = ParsedTwoBodyFourCenterIntKey::key(occ_space->id(),occ_space->id(),
                                                             bra_space->id(),ket_space->id(),
                                                             std::string("ERI"),
                                                             std::string(TwoBodyIntLayout::b1b2_k1k2));
      Ref<TwoBodyMOIntsTransform> mxny_tform = ints_rtime->get(tform_key);
      Ref<R12IntsAcc> mnxy_acc = mxny_tform->ints_acc();

      const int nocc = occ_space->rank();
      const int nbra = bra_space->rank();
      const int nket = ket_space->rank();
      const int nbraket = nbra*nket;

      ExEnv::out0() << indent << "Begin computation of exchange matrix" << endl;

      // Compute the number of tasks that have full access to the integrals
      // and split the work among them
      vector<int> proc_with_ints;
      int nproc_with_ints = mnxy_acc->tasks_with_access(proc_with_ints);

      //////////////////////////////////////////////////////////////
      //
      // Evaluation of the exchange matrix proceeds as follows:
      //
      //    loop over batches of mm, 0<=m<nocc
      //      load (mmxy)=(xm|my) into memory
      //
      //      loop over xy, 0<=x<nbra, 0<=y<nket
      //        compute K[x][y] += (mmxy)
      //      end xy loop
      //    end mm loop
      //
      /////////////////////////////////////////////////////////////////////////////////

      double* K_xy = new double[nbraket];
      memset(K_xy,0,nbraket*sizeof(double));
      Timer tim_mo_ints_retrieve;
      tim_mo_ints_retrieve.set_default("MO ints retrieve");
      if (mnxy_acc->has_access(me)) {

        for(int m=0; m<nocc; m++) {

          const int mm = m*nocc+m;
          const int mm_proc = mm%nproc_with_ints;
          if (mm_proc != proc_with_ints[me])
            continue;

          // Get (|1/r12|) integrals
          tim_mo_ints_retrieve.enter_default();
          const double *mmxy_buf_eri = mnxy_acc->retrieve_pair_block(m,m,TwoBodyOper::eri);
          tim_mo_ints_retrieve.exit_default();

          const double one = 1.0;
          const int unit_stride = 1;
          F77_DAXPY(&nbraket,&one,mmxy_buf_eri,&unit_stride,K_xy,&unit_stride);

          mnxy_acc->release_pair_block(m,m,TwoBodyOper::eri);
        }
      }

      ExEnv::out0() << indent << "End of computation of exchange matrix" << endl;

      msg->sum(K_xy,nbraket);

      RefSCMatrix K(bra_space->coefs()->coldim(), ket_space->coefs()->coldim(), bra_space->coefs()->kit());
      K.assign(K_xy);
      delete[] K_xy;

      ExEnv::out0() << decindent;
      ExEnv::out0() << indent << "Exited exchange matrix evaluator" << endl;
      tim_exchange.exit();

      return K;
    }

}} // namespace sc::detail

/////////////////////////////////////////////////////////////////////////////

// Local Variables:
// mode: c++
// c-file-style: "CLJ-CONDENSED"
// End: