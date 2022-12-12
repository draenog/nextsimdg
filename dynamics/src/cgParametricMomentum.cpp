/*!
 * @file ParametricMomentum.cpp
 * @date 1 Mar 2022
 * @author Thomas Richter <thomas.richter@ovgu.de>
 */

#include "cgParametricMomentum.hpp"
#include "Interpolations.hpp"
#include "MEB.hpp"
#include "ParametricTools.hpp"

#include "mevp.hpp"

#include "stopwatch.hpp"

namespace Nextsim {

#define DGSTRESS(CG) ( (CG==1?3:(CG==2?8:-1) ) )
extern Timer GlobalTimer;

////////////////////////////////////////////////// Strain (ParametricMesh)

template <int CG>
void CGParametricMomentum<CG>::ProjectCGVelocityToDGStrain()
{
    assert(static_cast<long int>((CG * smesh.nx + 1) * (CG * smesh.ny + 1)) == vx.rows());
    assert(static_cast<long int>((CG * smesh.nx + 1) * (CG * smesh.ny + 1)) == vy.rows());
    assert(static_cast<long int>(smesh.nx * smesh.ny) == E11.rows());
    assert(static_cast<long int>(smesh.nx * smesh.ny) == E12.rows());
    assert(static_cast<long int>(smesh.nx * smesh.ny) == E22.rows());

    const int cgshift = CG * smesh.nx + 1; //!< Index shift for each row

    if (precompute_matrices == 0) {
        // parallelize over the rows
#pragma omp parallel for
        for (size_t row = 0; row < smesh.ny; ++row) {
            int dgi = smesh.nx * row; //!< Index of dg vector
            int cgi = CG * cgshift * row; //!< Lower left index of cg vector

            for (size_t col = 0; col < smesh.nx; ++col, ++dgi, cgi += CG) {

	      if (smesh.landmask[dgi]==0) // only on ice!
		continue;

                // get the 4/9 local x/y - velocity coefficients on the element
	      Eigen::Matrix<double, CGDOFS(CG), 1> vx_local, vy_local;
                if (CG == 1) {
                    vx_local << vx(cgi), vx(cgi + 1), vx(cgi + cgshift), vx(cgi + 1 + cgshift);
                    vy_local << vy(cgi), vy(cgi + 1), vy(cgi + cgshift), vy(cgi + 1 + cgshift);
                } else if (CG == 2) {
                    vx_local << vx(cgi), vx(cgi + 1), vx(cgi + 2), vx(cgi + cgshift), vx(cgi + 1 + cgshift),
                        vx(cgi + 2 + cgshift), vx(cgi + 2 * cgshift), vx(cgi + 1 + 2 * cgshift),
                        vx(cgi + 2 + 2 * cgshift);

                    vy_local << vy(cgi), vy(cgi + 1), vy(cgi + 2), vy(cgi + cgshift), vy(cgi + 1 + cgshift),
                        vy(cgi + 2 + cgshift), vy(cgi + 2 * cgshift), vy(cgi + 1 + 2 * cgshift),
                        vy(cgi + 2 + 2 * cgshift);
                } else
                    abort();

                // get the reference-Gradient of the velocity in the GP
                // (vx_i * d_x/y phi_i(q))
                const Eigen::Matrix<Nextsim::FloatType, 1, (CG == 2 ? 9 : 4)> DX_VX_g = PHIx<CG, (CG == 2 ? 3 : 2)>.transpose() * vx_local;
                const Eigen::Matrix<Nextsim::FloatType, 1, (CG == 2 ? 9 : 4)> DY_VX_g = PHIy<CG, (CG == 2 ? 3 : 2)>.transpose() * vx_local;
                const Eigen::Matrix<Nextsim::FloatType, 1, (CG == 2 ? 9 : 4)> DX_VY_g = PHIx<CG, (CG == 2 ? 3 : 2)>.transpose() * vy_local;
                const Eigen::Matrix<Nextsim::FloatType, 1, (CG == 2 ? 9 : 4)> DY_VY_g = PHIy<CG, (CG == 2 ? 3 : 2)>.transpose() * vy_local;
                const Eigen::Matrix<Nextsim::FloatType, 2, (CG == 2 ? 9 : 4)> dxT = (ParametricTools::dxT<(CG == 2 ? 3 : 2)>(smesh, dgi).array().rowwise() * GAUSSWEIGHTS<(CG == 2 ? 3 : 2)>.array()).matrix();
                const Eigen::Matrix<Nextsim::FloatType, 2, (CG == 2 ? 9 : 4)> dyT = (ParametricTools::dyT<(CG == 2 ? 3 : 2)>(smesh, dgi).array().rowwise() * GAUSSWEIGHTS<(CG == 2 ? 3 : 2)>.array()).matrix();

                // gradient of transformation
                //      [ dxT1, dyT1 ]     //            [ dyT2, -dxT2 ]
                // dT = 		       // J dT^{-T}=
                //      [ dxT2, dyT2 ]     //            [ -dyT1, dxT1 ]
                //

                const Eigen::Matrix<Nextsim::FloatType, 1, (CG == 2 ? 9 : 4)> J_dx_vx_g = dyT.row(1).array() * DX_VX_g.array() - dxT.row(1).array() * DY_VX_g.array();
                const Eigen::Matrix<Nextsim::FloatType, 1, (CG == 2 ? 9 : 4)> J_dy_vx_g = dxT.row(0).array() * DY_VX_g.array() - dyT.row(0).array() * DX_VX_g.array();
                const Eigen::Matrix<Nextsim::FloatType, 1, (CG == 2 ? 9 : 4)> J_dx_vy_g = dyT.row(1).array() * DX_VY_g.array() - dxT.row(1).array() * DY_VY_g.array();
                const Eigen::Matrix<Nextsim::FloatType, 1, (CG == 2 ? 9 : 4)> J_dy_vy_g = dxT.row(0).array() * DY_VY_g.array() - dyT.row(0).array() * DX_VY_g.array();

                const Eigen::Matrix<Nextsim::FloatType, DGSTRESS(CG), DGSTRESS(CG)> imass = ParametricTools::massMatrix<DGSTRESS(CG)>(smesh, dgi).inverse();

                E11.row(dgi) = imass * (PSI<DGSTRESS(CG), (CG == 2 ? 3 : 2)> * J_dx_vx_g.transpose());
                E22.row(dgi) = imass * (PSI<DGSTRESS(CG), (CG == 2 ? 3 : 2)> * J_dy_vy_g.transpose());
                E12.row(dgi) = 0.5 * imass * (PSI<DGSTRESS(CG), (CG == 2 ? 3 : 2)> * (J_dx_vy_g.transpose() + J_dy_vx_g.transpose()));
            }
        }
    } else if (precompute_matrices == 1) {
        // parallelize over the rows
#pragma omp parallel for
        for (size_t row = 0; row < smesh.ny; ++row) {
            int dgi = smesh.nx * row; //!< Index of dg vector
            int cgi = CG * cgshift * row; //!< Lower left index of cg vector

            for (size_t col = 0; col < smesh.nx; ++col, ++dgi, cgi += CG) {

	      if (smesh.landmask[dgi]==0) // only on ice!
		continue;
	      
	      
                // get the 4/9 local x/y - velocity coefficients on the element
                Eigen::Matrix<double, CGDOFS(CG), 1> vx_local, vy_local;
                if (CG == 1) {
                    vx_local << vx(cgi), vx(cgi + 1), vx(cgi + cgshift), vx(cgi + 1 + cgshift);
                    vy_local << vy(cgi), vy(cgi + 1), vy(cgi + cgshift), vy(cgi + 1 + cgshift);
                } else if (CG == 2) {
                    vx_local << vx(cgi), vx(cgi + 1), vx(cgi + 2), vx(cgi + cgshift), vx(cgi + 1 + cgshift),
                        vx(cgi + 2 + cgshift), vx(cgi + 2 * cgshift), vx(cgi + 1 + 2 * cgshift),
                        vx(cgi + 2 + 2 * cgshift);

                    vy_local << vy(cgi), vy(cgi + 1), vy(cgi + 2), vy(cgi + cgshift), vy(cgi + 1 + cgshift),
                        vy(cgi + 2 + cgshift), vy(cgi + 2 * cgshift), vy(cgi + 1 + 2 * cgshift),
                        vy(cgi + 2 + 2 * cgshift);
                } else
                    abort();

                E11.row(dgi) = ptrans.iMgradX[dgi] * vx_local;
                E22.row(dgi) = ptrans.iMgradY[dgi] * vy_local;
                E12.row(dgi) = 0.5 * (ptrans.iMgradX[dgi] * vy_local + ptrans.iMgradY[dgi] * vx_local);
            }
        }
    }
}

////////////////////////////////////////////////// STRESS Tensor
// Sasip-Mesh Interface
template <int CG>
void CGParametricMomentum<CG>::AddStressTensor(const double scale, CGVector<CG>& tx,
    CGVector<CG>& ty) const
{
    // parallelization in tripes
    for (size_t p = 0; p < 2; ++p)
#pragma omp parallel for schedule(static)
        for (size_t cy = 0; cy < smesh.ny; ++cy) //!< loop over all cells of the mesh
        {
            if (cy % 2 == p) {
                size_t c = smesh.nx * cy;
                for (size_t cx = 0; cx < smesh.nx; ++cx, ++c) //!< loop over all cells of the mesh
		  if (smesh.landmask[c]==1) // only on ice!
                    AddStressTensorCell(scale, c, cx, cy, tx, ty);
            }
        }
}

template <int CG>
void CGParametricMomentum<CG>::DirichletZero(CGVector<CG>& v)
{
  // the four segments bottom, right, top, left, are each processed in parallel
  for (size_t seg=0;seg<4;++seg)
    {
#pragma omp parallel for
        for (size_t i = 0; i < smesh.dirichlet[seg].size(); ++i) {

	  const size_t eid = smesh.dirichlet[seg][i];
	  const size_t ix = eid % smesh.nx; // compute 'coordinate' of element
	  const size_t iy = eid / smesh.nx;

	  if (seg == 0) // bottom
	    for (size_t j = 0; j < CG + 1; ++j)
	      v(iy * CG * (CG * smesh.nx + 1) + CG * ix + j, 0) = 0.0;
	  else if (seg == 1) // right
	    for (size_t j = 0; j < CG + 1; ++j)
	      v(iy * CG * (CG * smesh.nx + 1) + CG * ix + CG + (CG * smesh.nx + 1) * j, 0) = 0.0;
	  else if (seg == 2) // top
	    for (size_t j = 0; j < CG + 1; ++j)
                    v((iy + 1) * CG * (CG * smesh.nx + 1) + CG * ix + j, 0) = 0.0;
	  else if (seg == 3) // left
                for (size_t j = 0; j < CG + 1; ++j)
		  v(iy * CG * (CG * smesh.nx + 1) + CG * ix + (CG * smesh.nx + 1) * j, 0) = 0.0;
	  else {
	    std::cerr << "That should not have happened!" << std::endl;
	    abort();
            }
        }
    }
}

// --------------------------------------------------

template <int CG>
template <int DG>
void CGParametricMomentum<CG>::prepareIteration(const DGVector<DG>& H, const DGVector<DG>& A)
{
    // copy old velocity
    vx_mevp = vx;
    vy_mevp = vy;

    // interpolate ice height and concentration to local cg Variables
    Interpolations::DG2CG(smesh, cg_A, A);
    Interpolations::DG2CG(smesh, cg_H, H);
    // limit A to [0,1] and H to [0, ...)
    cg_A = cg_A.cwiseMin(1.0);
    cg_A = cg_A.cwiseMax(0.0);
    cg_H = cg_H.cwiseMax(1.e-4);
}

template <int CG>
template <int DG>
void CGParametricMomentum<CG>::mEVPStep(const VPParameters& params,
    const size_t NT_evp, const double alpha, const double beta,
    double dt_adv,
    const DGVector<DG>& H, const DGVector<DG>& A)
{

      Nextsim::GlobalTimer.start("time loop - mevp - strain");
    //! Compute Strain Rate
    // momentum.ProjectCGVelocityToDG1Strain(ptrans_stress, E11, E12, E22);
    ProjectCGVelocityToDGStrain();
    Nextsim::GlobalTimer.stop("time loop - mevp - strain");

    Nextsim::GlobalTimer.start("time loop - mevp - stress");
    if (precompute_matrices == 0) // computations on the fly
      Nextsim::mEVP::StressUpdateHighOrder<CG, DGSTRESS(CG), DG>(params, smesh, S11, S12, S22, E11, E12, E22, H, A, alpha, beta);
    else // --------------------- // use precomputed
        Nextsim::mEVP::StressUpdateHighOrder(params, ptrans, smesh, S11, S12, S22, E11, E12, E22, H, A, alpha, beta);
    Nextsim::GlobalTimer.stop("time loop - mevp - stress");

    Nextsim::GlobalTimer.start("time loop - mevp - update");
    //! Update
    Nextsim::GlobalTimer.start("time loop - mevp - update1");

    //	    update by a loop.. implicit parts and h-dependent
#pragma omp parallel for
    for (int i = 0; i < vx.rows(); ++i) {
      double absatm = sqrt(ax(i)*ax(i)+ay(i)*ay(i));
      double absocn = sqrt(SQR(vx(i)-ox(i)) + SQR(vy(i)-oy(i)));

      vx(i) = (1.0
            / (params.rho_ice * cg_H(i) / dt_adv * (1.0 + beta) // implicit parts
                + cg_A(i) * params.F_ocean
                    * absocn ) // implicit parts
            * (params.rho_ice * cg_H(i) / dt_adv
                    * (beta * vx(i) + vx_mevp(i))
                + // pseudo-timestepping
                cg_A(i)
                    * (params.F_atm * absatm * ax(i) + // atm forcing
                        params.F_ocean * absocn
                            * ox(i)) // ocean forcing
                + params.rho_ice * cg_H(i) * params.fc
                    * (vy(i) - oy(i)) // cor + surface
                ));
        vy(i) = (1.0
            / (params.rho_ice * cg_H(i) / dt_adv * (1.0 + beta) // implicit parts
                + cg_A(i) * params.F_ocean
                    * absocn ) // implicit parts
            * (params.rho_ice * cg_H(i) / dt_adv
                    * (beta * vy(i) + vy_mevp(i))
                + // pseudo-timestepping
                cg_A(i)
                    * (params.F_atm * absatm * ay(i) + // atm forcing
                        params.F_ocean * absocn
                            * oy(i)) // ocean forcing
                + params.rho_ice * cg_H(i) * params.fc
                    * (ox(i) - vx(i))));
    }
    Nextsim::GlobalTimer.stop("time loop - mevp - update1");

    Nextsim::GlobalTimer.start("time loop - mevp - update2");
    // Implicit etwas ineffizient
#pragma omp parallel for
    for (int i = 0; i < tmpx.rows(); ++i)
        tmpx(i) = tmpy(i) = 0;

    Nextsim::GlobalTimer.start("time loop - mevp - update2 -stress");
    // AddStressTensor(ptrans_stress, -1.0, tmpx, tmpy, S11, S12, S22);
    AddStressTensor(-1.0, tmpx, tmpy);
    Nextsim::GlobalTimer.stop("time loop - mevp - update2 -stress");

#pragma omp parallel for
    for (int i = 0; i < vx.rows(); ++i) {
      double absocn = sqrt(SQR(vx(i)-ox(i)) + SQR(vy(i)-oy(i)));

        vx(i) += (1.0
                     / (params.rho_ice * cg_H(i) / dt_adv * (1.0 + beta) // implicit parts
                         + cg_A(i) * params.F_ocean
                             * absocn ) // implicit parts
                     * tmpx(i))
            / lumpedcgmass(i);

        vy(i) += (1.0
                     / (params.rho_ice * cg_H(i) / dt_adv * (1.0 + beta) // implicit parts
                         + cg_A(i) * params.F_ocean
                             * absocn) // implicit parts
                     * tmpy(i))
            / lumpedcgmass(i);
    }
    Nextsim::GlobalTimer.stop("time loop - mevp - update2");
    Nextsim::GlobalTimer.stop("time loop - mevp - update");

    Nextsim::GlobalTimer.start("time loop - mevp - bound.");
    assert(smesh.periodic.size() == 0);
    //    PeriodicBoundary();
    DirichletZero();

    
    // Landmask
    const size_t inrow = CG*smesh.nx+1;
#pragma omp parallel for
    for (size_t eid=0;eid<smesh.nelements;++eid)
      if (smesh.landmask[eid]==0)
	{
	  const size_t ex = eid%smesh.nx;
	  const size_t ey = eid/smesh.nx;
	  for (int jy=0;jy<CG+1;++jy)
	    for (int jx=0;jx<CG+1;++jx)
	      {
		vx(inrow*(CG*ey+jy)+CG*ex+jx)=0.0;
		vy(inrow*(CG*ey+jy)+CG*ex+jx)=0.0;
	      }
	}
    
    Nextsim::GlobalTimer.stop("time loop - mevp - bound.");
}
// --------------------------------------------------
template <int CG>
template <int DG>
void CGParametricMomentum<CG>::prepareIteration(const DGVector<DG>& H,
    const DGVector<DG>& A, const DGVector<DG>& D)
{

    // set the average sub-iteration velocity to zero
    avg_vx.setZero();
    avg_vy.setZero();   

    // interpolate ice height and concentration to local cg Variables
    Interpolations::DG2CG(smesh, cg_A, A);
    Interpolations::DG2CG(smesh, cg_H, H);
    Interpolations::DG2CG(smesh, cg_D, D);

    // limit A and D to [0,1] and H to [0, ...)
    cg_A = cg_A.cwiseMin(1.0);
    cg_A = cg_A.cwiseMax(0.0);
    cg_H = cg_H.cwiseMax(1.e-4);
    cg_D = cg_D.cwiseMin(1.0);
    cg_D = cg_D.cwiseMax(0.0);
}

template <int CG>
template <int DG>
void CGParametricMomentum<CG>::MEBStep(const MEBParameters& params,
    const size_t NT_meb, double dt_adv, const DGVector<DG>& H, const DGVector<DG>& A,
    DGVector<DG>& D)
{

    double dt_mom = dt_adv / NT_meb;

    Nextsim::GlobalTimer.start("time loop - meb - strain");
    //! Compute Strain Rate
    ProjectCGVelocityToDGStrain();
    Nextsim::GlobalTimer.stop("time loop - meb - strain");

    Nextsim::GlobalTimer.start("time loop - meb - stress");
    // TODO compute stress update with precomputed transformations
    Nextsim::MEB::StressUpdateHighOrder<CG, DGSTRESS(CG), DG>(params, smesh, S11, S12, S22, E11, E12, E22, H, A, D, dt_mom);
    // Nextsim::MEB::StressUpdateHighOrder(params, ptrans, smesh, S11, S12, S22, E11, E12, E22, H, A, D, dt_mom);
    Nextsim::GlobalTimer.stop("time loop - meb - stress");

    Nextsim::GlobalTimer.start("time loop - meb - update");
    //! Update
    Nextsim::GlobalTimer.start("time loop - meb - update1");

    //	    update by a loop.. implicit parts and h-dependent
#pragma omp parallel for
    for (int i = 0; i < vx.rows(); ++i) {
        vx(i) = (1.0
            / (params.rho_ice * cg_H(i) / dt_mom // implicit parts
                + cg_A(i) * params.F_ocean
                    * fabs(ox(i) - vx(i))) // implicit parts
            * (params.rho_ice * cg_H(i) / dt_mom * vx(i)
                + cg_A(i) * (params.F_atm * fabs(ax(i)) * ax(i) + // atm forcing
                      params.F_ocean * fabs(ox(i) - vx(i)) * ox(i)) // ocean forcing
                + params.rho_ice * cg_H(i) * params.fc
                    * (vy(i) - oy(i)) // cor + surface
                ));
        vy(i) = (1.0
            / (params.rho_ice * cg_H(i) / dt_mom // implicit parts
                + cg_A(i) * params.F_ocean
                    * fabs(oy(i) - vy(i))) // implicit parts
            * (params.rho_ice * cg_H(i) / dt_mom * vy(i)
                + cg_A(i) * (params.F_atm * fabs(ay(i)) * ay(i) + // atm forcing
                      params.F_ocean * fabs(oy(i) - vy(i)) * oy(i)) // ocean forcing
                + params.rho_ice * cg_H(i) * params.fc
                    * (ox(i) - vx(i))));
    }
    Nextsim::GlobalTimer.stop("time loop - meb - update1");

    Nextsim::GlobalTimer.start("time loop - meb - update2");
    // Implicit etwas ineffizient
#pragma omp parallel for
    for (int i = 0; i < tmpx.rows(); ++i)
        tmpx(i) = tmpy(i) = 0;

    Nextsim::GlobalTimer.start("time loop - meb - update2 -stress");
    // AddStressTensor(ptrans_stress, -1.0, tmpx, tmpy, S11, S12, S22);
    AddStressTensor(-1.0, tmpx, tmpy);
    Nextsim::GlobalTimer.stop("time loop - meb - update2 -stress");

#pragma omp parallel for
    for (int i = 0; i < vx.rows(); ++i) {
        vx(i) += (1.0
                     / (params.rho_ice * cg_H(i) / dt_mom // implicit parts
                         + cg_A(i) * params.F_ocean
                             * fabs(ox(i) - vx(i))) // implicit parts
                     * tmpx(i))
            / lumpedcgmass(i);
        ;

        vy(i) += (1.0
                     / (params.rho_ice * cg_H(i) / dt_mom // implicit parts
                         + cg_A(i) * params.F_ocean
                             * fabs(oy(i) - vy(i))) // implicit parts
                     * tmpy(i))
            / lumpedcgmass(i);
        ;
    }
    Nextsim::GlobalTimer.stop("time loop - meb - update2");
    Nextsim::GlobalTimer.stop("time loop - meb - update");

    Nextsim::GlobalTimer.start("time loop - meb - bound.");
    DirichletZero();
    Nextsim::GlobalTimer.stop("time loop - meb - bound.");

    avg_vx += vx/NT_meb;
    avg_vy += vy/NT_meb;   
}
// --------------------------------------------------

template class CGParametricMomentum<1>;
template class CGParametricMomentum<2>;

template void CGParametricMomentum<1>::prepareIteration(const DGVector<1>& H, const DGVector<1>& A);
template void CGParametricMomentum<1>::prepareIteration(const DGVector<3>& H, const DGVector<3>& A);
template void CGParametricMomentum<1>::prepareIteration(const DGVector<6>& H, const DGVector<6>& A);
template void CGParametricMomentum<2>::prepareIteration(const DGVector<1>& H, const DGVector<1>& A);
template void CGParametricMomentum<2>::prepareIteration(const DGVector<3>& H, const DGVector<3>& A);
template void CGParametricMomentum<2>::prepareIteration(const DGVector<6>& H, const DGVector<6>& A);

template void CGParametricMomentum<1>::prepareIteration(const DGVector<1>& H, const DGVector<1>& A, const DGVector<1>& D);
template void CGParametricMomentum<1>::prepareIteration(const DGVector<3>& H, const DGVector<3>& A, const DGVector<3>& D);
template void CGParametricMomentum<1>::prepareIteration(const DGVector<6>& H, const DGVector<6>& A, const DGVector<6>& D);
template void CGParametricMomentum<2>::prepareIteration(const DGVector<1>& H, const DGVector<1>& A, const DGVector<1>& D);
template void CGParametricMomentum<2>::prepareIteration(const DGVector<3>& H, const DGVector<3>& A, const DGVector<3>& D);
template void CGParametricMomentum<2>::prepareIteration(const DGVector<6>& H, const DGVector<6>& A, const DGVector<6>& D);

// --------------------------------------------------

template void CGParametricMomentum<1>::mEVPStep(const VPParameters& params,
    size_t NT_evp, double alpha, double beta,
    double dt_adv,
    const DGVector<1>& H, const DGVector<1>& A);
template void CGParametricMomentum<1>::mEVPStep(const VPParameters& params,
    size_t NT_evp, double alpha, double beta,
    double dt_adv,
    const DGVector<3>& H, const DGVector<3>& A);
template void CGParametricMomentum<1>::mEVPStep(const VPParameters& params,
    size_t NT_evp, double alpha, double beta,
    double dt_adv,
    const DGVector<6>& H, const DGVector<6>& A);



template void CGParametricMomentum<2>::mEVPStep(const VPParameters& params,
    size_t NT_evp, double alpha, double beta,
    double dt_adv,
    const DGVector<1>& H, const DGVector<1>& A);
template void CGParametricMomentum<2>::mEVPStep(const VPParameters& params,
    size_t NT_evp, double alpha, double beta,
    double dt_adv,
    const DGVector<3>& H, const DGVector<3>& A);
template void CGParametricMomentum<2>::mEVPStep(const VPParameters& params,
    size_t NT_evp, double alpha, double beta,
    double dt_adv,
    const DGVector<6>& H, const DGVector<6>& A);

// --------------------------------------------------

template void CGParametricMomentum<1>::MEBStep(const MEBParameters& params,
    size_t NT_evp, double dt_adv,
    const DGVector<1>& H, const DGVector<1>& A, DGVector<1>& D);
template void CGParametricMomentum<1>::MEBStep(const MEBParameters& params,
    size_t NT_evp, double dt_adv,
    const DGVector<3>& H, const DGVector<3>& A, DGVector<3>& D);
template void CGParametricMomentum<1>::MEBStep(const MEBParameters& params,
    size_t NT_evp, double dt_adv,
    const DGVector<6>& H, const DGVector<6>& A, DGVector<6>& D);


template void CGParametricMomentum<2>::MEBStep(const MEBParameters& params,
    size_t NT_evp, double dt_adv,
    const DGVector<1>& H, const DGVector<1>& A, DGVector<1>& D);
template void CGParametricMomentum<2>::MEBStep(const MEBParameters& params,
    size_t NT_evp, double dt_adv,
    const DGVector<3>& H, const DGVector<3>& A, DGVector<3>& D);
template void CGParametricMomentum<2>::MEBStep(const MEBParameters& params,
    size_t NT_evp, double dt_adv,
    const DGVector<6>& H, const DGVector<6>& A, DGVector<6>& D);


} /* namespace Nextsim */
