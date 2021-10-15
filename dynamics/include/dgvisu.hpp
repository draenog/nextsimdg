/*----------------------------   visu.h     ---------------------------*/
/*      $Id:$                 */
#ifndef __visu_H
#define __visu_H
/*----------------------------   visu.h     ---------------------------*/

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "dgvector.hpp"
#include "mesh.hpp"

namespace Nextsim {

class VTK {
public:
    //! Puts together the file name by adding the index n with fixed width
    static std::string compose_vtkname(const std::string& fname, int n)
    {
        std::ostringstream ss;
        ss << fname << "." << std::setw(5) << std::setfill('0') << n << ".vtk";
        return ss.str();
    }

    static inline double cuttol(double v, double tol)
    {
        if ((v < tol) && (v > -tol))
            return 0;
        return v;
    }

    ////////////////////////////////////////////////// Simple output dg(0) +
    /// cg(1)

    template <int DGdegree>
    static void write_cellvector(const std::string& fname,
        const CellVector<DGdegree>& v,
        const Mesh& mesh)
    {
        std::ofstream OUT(fname.c_str());
        if (!OUT.is_open()) {
            std::cerr << "Failed to open '" << fname << "'." << std::endl;
            assert(0);
        }

        // Structure Points
        OUT << "# vtk DataFile Version 2.0" << std::endl
            << "Structured output generated by Nextsim" << std::endl
            << "ASCII" << std::endl
            << "DATASET STRUCTURED_POINTS" << std::endl
            << "DIMENSIONS " << mesh.nx + 1 << " " << mesh.ny + 1 << " 1"
            << std::endl
            << "ORIGIN 0 0 0" << std::endl
            << "SPACING " << mesh.h << " " << mesh.h << " " << mesh.h << std::endl;
        OUT << "POINT_DATA " << (mesh.nx + 1) * (mesh.ny + 1) << std::endl
            << "SCALARS interpolate DOUBLE 1" << std::endl
            << "LOOKUP_TABLE default" << std::endl;

        Eigen::VectorXd interpolate((mesh.nx + 1) * (mesh.ny + 1));
        Eigen::VectorXi count((mesh.nx + 1) * (mesh.ny + 1));

        interpolate.setZero();
        count.setZero();

        size_t ii = 0;
        for (size_t iy = 0; iy < mesh.ny; ++iy)
            for (size_t ix = 0; ix < mesh.nx; ++ix, ++ii) {
                size_t i0 = iy * (mesh.nx + 1) + ix;
                count(i0)++;
                count(i0 + 1)++;
                count(i0 + (mesh.nx + 1))++;
                count(i0 + (mesh.nx + 1) + 1)++;

                interpolate(i0) += v(ii, 0);
                interpolate(i0 + 1) += v(ii, 0);
                interpolate(i0 + (mesh.nx + 1)) += v(ii, 0);
                interpolate(i0 + (mesh.nx + 1) + 1) += v(ii, 0);

                if (DGdegree == 1) {
                    interpolate(i0) += -0.5 * v(ii, 1);
                    interpolate(i0 + 1) += 0.5 * v(ii, 1);
                    interpolate(i0 + (mesh.nx + 1)) += -0.5 * v(ii, 1);
                    interpolate(i0 + (mesh.nx + 1) + 1) += 0.5 * v(ii, 1);

                    interpolate(i0) += -0.5 * v(ii, 2);
                    interpolate(i0 + 1) += -0.5 * v(ii, 2);
                    interpolate(i0 + (mesh.nx + 1)) += 0.5 * v(ii, 2);
                    interpolate(i0 + (mesh.nx + 1) + 1) += 0.5 * v(ii, 2);
                }
            }
        for (int i = 0; i < interpolate.size(); ++i)
            OUT << cuttol(interpolate(i) / count(i), 1.e-20) << std::endl;

        OUT << "CELL_DATA " << mesh.nx * mesh.ny << std::endl
            << "SCALARS average DOUBLE 1" << std::endl
            << "LOOKUP_TABLE default" << std::endl;
        ii = 0;
        for (size_t iy = 0; iy < mesh.ny; ++iy)
            for (size_t ix = 0; ix < mesh.nx; ++ix, ++ii)
                OUT << cuttol(v(ii, 0), 1.e-20) << std::endl;
        OUT.close();
    }

    template <int DGdegree>
    static void write_cellvector(const std::string& fname,
        int n,
        const CellVector<DGdegree>& v,
        const Mesh& mesh)
    {
        write_cellvector(compose_vtkname(fname, n), v, mesh);
    }

    ////////////////////////////////////////////////// dG(1) output

    template <int DGdegree>
    static void write_dg(const std::string& fname,
        const CellVector<DGdegree>& v,
        const Mesh& mesh)
    {

        std::ofstream OUT(fname.c_str());
        assert(OUT.is_open());

        // Structure Points
        OUT << "# vtk DataFile Version 2.0" << std::endl
            << "dg(" << DGdegree << ") output generated by Nextsim" << std::endl
            << "ASCII" << std::endl
            << "DATASET UNSTRUCTURED_GRID" << std::endl
            << "FIELD FieldData 1" << std::endl
            << "TIME 1 1 double" << std::endl
            << 0.0 << std::endl;
        OUT << "POINTS " << 4 * mesh.nx * mesh.ny << " DOUBLE" << std::endl;
        for (size_t iy = 0; iy < mesh.ny; ++iy)
            for (size_t ix = 0; ix < mesh.nx; ++ix) {
                OUT << mesh.h * ix << "\t" << mesh.h * iy << "\t0" << std::endl;
                OUT << mesh.h * (ix + 1) << "\t" << mesh.h * iy << "\t0" << std::endl;
                OUT << mesh.h * (ix + 1) << "\t" << mesh.h * (iy + 1) << "\t0"
                    << std::endl;
                OUT << mesh.h * ix << "\t" << mesh.h * (iy + 1) << "\t0" << std::endl;
            }
        OUT << "CELLS " << mesh.nx * mesh.ny << " " << 5 * mesh.nx * mesh.ny
            << std::endl;
        size_t ii = 0;
        for (size_t iy = 0; iy < mesh.ny; ++iy)
            for (size_t ix = 0; ix < mesh.nx; ++ix, ++ii)
                OUT << "4 " << 4 * ii << " " << 4 * ii + 1 << " " << 4 * ii + 2 << " "
                    << 4 * ii + 3 << std::endl;

        OUT << "CELL_TYPES " << mesh.nx * mesh.ny << std::endl;
        for (ii = 0; ii < mesh.nx * mesh.ny; ++ii)
            OUT << "9 ";
        OUT << std::endl;

        OUT << "POINT_DATA " << 4 * mesh.nx * mesh.ny << std::endl
            << "SCALARS dg" << DGdegree << " DOUBLE " << std::endl
            << "LOOKUP_TABLE default" << std::endl;

        ii = 0;
        for (size_t iy = 0; iy < mesh.ny; ++iy)
            for (size_t ix = 0; ix < mesh.nx; ++ix, ++ii) {
                std::array<double, 4> interpolate = {
                    v(ii, 0), v(ii, 0), v(ii, 0), v(ii, 0)
                };

                if (DGdegree >= 1) {
                    interpolate[0] += -0.5 * v(ii, 1);
                    interpolate[1] += 0.5 * v(ii, 1);
                    interpolate[2] += 0.5 * v(ii, 1);
                    interpolate[3] += -0.5 * v(ii, 1);

                    interpolate[0] += -0.5 * v(ii, 2);
                    interpolate[1] += -0.5 * v(ii, 2);
                    interpolate[2] += 0.5 * v(ii, 2);
                    interpolate[3] += 0.5 * v(ii, 2);
                }
                if (DGdegree >= 2) {
                    interpolate[0] += 1. / 6. * v(ii, 3);
                    interpolate[1] += 1. / 6. * v(ii, 3);
                    interpolate[2] += 1. / 6. * v(ii, 3);
                    interpolate[3] += 1. / 6. * v(ii, 3);

                    interpolate[0] += 1. / 6. * v(ii, 4);
                    interpolate[1] += 1. / 6. * v(ii, 4);
                    interpolate[2] += 1. / 6. * v(ii, 4);
                    interpolate[3] += 1. / 6. * v(ii, 4);

                    interpolate[0] += 1. / 4. * v(ii, 5);
                    interpolate[1] += -1. / 4. * v(ii, 5);
                    interpolate[2] += 1. / 4. * v(ii, 5);
                    interpolate[3] += -1. / 4. * v(ii, 5);
                }
                for (auto it : interpolate)
                    OUT << it << std::endl;
            }
        OUT.close();
    }

    template <int DGdegree>
    static void write_dg(const std::string& fname,
        int n,
        const CellVector<DGdegree>& v,
        const Mesh& mesh)
    {
        write_dg(compose_vtkname(fname, n), v, mesh);
    }
};

} // namespace Nextsim

/*----------------------------   visu.h     ---------------------------*/
/* end of #ifndef __visu_H */
#endif
/*----------------------------   visu.h     ---------------------------*/
