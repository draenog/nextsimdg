#include "ParametricMesh.hpp"

#include <fstream>
#include <iostream>

namespace Nextsim {

void ParametricMesh::readmesh(std::string fname)
{
    std::ifstream IN(fname.c_str());
    if (IN.fail()) {
        std::cerr << "ParametricMesh :: Could not open mesh file " << fname << std::endl;
        abort();
    }

    std::string status;
    IN >> status;

    if (status != "ParametricMesh") {
        std::cerr << "ParametricMesh :: Wrong file format" << fname << "\t" << status
                  << std::endl
                  << "'ParametricMesh' expected" << std::endl;
        abort();
    }

    IN >> status;

    if (status != "1.0") {
        std::cerr << "ParametricMesh :: Wrong file format version" << fname << std::endl;
        abort();
    }

    IN >> nx >> ny;

    if ((nx < 1) || (ny < 1)) {
        std::cerr << "ParametricMesh :: Wrong mesh dimensions (nx,ny) << " << nx << " " << ny << std::endl;
        abort();
    }

    // set number of elements & nodes
    nelements = nx * ny;
    nnodes = (nx + 1) * (ny + 1);
    vertices.resize(nnodes, 2);

    for (size_t i = 0; i < nnodes; ++i) {
        IN >> vertices(i, 0) >> vertices(i, 1);
        if (IN.eof()) {
            std::cerr << "ParametricMesh :: Unexpected eof << " << fname << std::endl;
            abort();
        }
    }

    IN.close();

    if (statuslog > 0) {
        std::cout << "ParametricMesh :: read mesh file " << fname << std::endl
                  << "             nx,ny = " << nx << " , " << ny << std::endl
                  << "             " << nelements << " elements,  " << nnodes << " nodes" << std::endl;
    }
}

/*!
 * returns minimum mesh size.
 *
 */
double ParametricMesh::hmin() const
{
    double hmin = 1.e99;
    for (size_t i = 0; i < nelements; ++i)
        hmin = std::min(hmin, h(i));
    return hmin;
}

/*!
 * returns are of domain
 */
double ParametricMesh::area() const
{
    double a = 0;
    for (size_t i = 0; i < nelements; ++i)
        a += area(i);
    return a;
}
}
