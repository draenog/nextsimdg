/*!
 * @file ParaGrid_test.cpp
 *
 * @date Oct 27, 2022
 * @author Tim Spain <timothy.spain@nersc.no>
 */

#ifdef USE_MPI
#include <doctest/extensions/doctest_mpi.h>
#else
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#endif

#include "include/Configurator.hpp"
#include "include/ConfiguredModule.hpp"
#include "include/NZLevels.hpp"
#include "include/ParaGridIO.hpp"
#include "include/ParametricGrid.hpp"
#include "include/StructureModule.hpp"
#include "include/gridNames.hpp"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>

#include <ncAtt.h>
#include <ncFile.h>
#include <ncGroup.h>
#include <ncVar.h>

const std::string test_files_dir = TEST_FILES_DIR;
const std::string filename = test_files_dir + "/paraGrid_test.nc";
const std::string diagFile = test_files_dir + "/paraGrid_diag.nc";
#ifdef USE_MPI
const std::string partition_filename = test_files_dir + "/partition_metadata_3.nc";
#define TEST_MPI_SIZE 3
#endif
const std::string date_string = "2000-01-01T00:00:00Z";

static const int DG = 3;
static const int DGSTRESS = 6;
static const int CG = 2;

namespace Nextsim {

size_t c = 0;

TEST_SUITE_BEGIN("ParaGrid");
#ifdef USE_MPI
// Number of ranks should not be hardcoded here
MPI_TEST_CASE("Write and read a ModelState-based RectGrid restart file", TEST_MPI_SIZE)
#else
TEST_CASE("Write and read a ModelState-based ParaGrid restart file")
#endif
{
    Module::setImplementation<IStructure>("Nextsim::ParametricGrid");

    std::filesystem::remove(filename);

    ParametricGrid grid;
    ParaGridIO* pio = new ParaGridIO(grid);
    grid.setIO(pio);

    // Set the dimension lengths
    size_t nx = 5;
    size_t ny = 7;
    size_t nz = 3;
    NZLevels::set(nz);
    size_t nxcg = CG * nx + 1;
    size_t nycg = CG * ny + 1;

    double yFactor = 0.01;
    double xFactor = 0.0001;

    ModelArray::setDimension(ModelArray::Dimension::X, nx);
    ModelArray::setDimension(ModelArray::Dimension::Y, ny);
    ModelArray::setDimension(ModelArray::Dimension::Z, NZLevels::get());
    ModelArray::setDimension(ModelArray::Dimension::XVERTEX, nx + 1);
    ModelArray::setDimension(ModelArray::Dimension::YVERTEX, ny + 1);
    ModelArray::setDimension(ModelArray::Dimension::XCG, nxcg);
    ModelArray::setDimension(ModelArray::Dimension::YCG, nycg);

    ModelArray::setNComponents(ModelArray::Type::DG, DG);
    ModelArray::setNComponents(ModelArray::Type::DGSTRESS, DGSTRESS);
    ModelArray::setNComponents(ModelArray::Type::VERTEX, ModelArray::nCoords);

    HField fractional(ModelArray::Type::H);
    DGField fractionalDG(ModelArray::Type::DG);
    HField mask(ModelArray::Type::H);
    fractional.resize();
    fractionalDG.resize();
    for (size_t j = 0; j < ny; ++j) {
        for (size_t i = 0; i < nx; ++i) {
            fractional(i, j) = j * yFactor + i * xFactor;
            mask(i, j)
                = (i - nx / 2) * (i - nx / 2) + (j - ny / 2) * (j - ny / 2) > (nx * ny) ? 0 : 1;
            for (size_t d = 0; d < DG; ++d) {
                fractionalDG.components({ i, j })[d] = fractional(i, j) + d;
            }
        }
    }

    DGField hice = fractionalDG + 10;
    DGField cice = fractionalDG + 20;
    HField hsnow = fractional + 30;
    ZField tice(ModelArray::Type::Z);
    tice.resize();
    for (size_t i = 0; i < ModelArray::size(ModelArray::Type::H); ++i) {
        for (size_t k = 0; k < nz; ++k) {
            tice.zIndexAndLayer(i, k) = fractional[i] + 40 + k;
        }
    }

    VertexField coordinates(ModelArray::Type::VERTEX);
    coordinates.resize();
    // Planar coordinates
    double scale = 1e5;

    // Vertex coordinates
    for (size_t i = 0; i < ModelArray::definedDimensions.at(ModelArray::Dimension::XVERTEX).length;
         ++i) {
        for (size_t j = 0;
             j < ModelArray::definedDimensions.at(ModelArray::Dimension::YVERTEX).length; ++j) {
            double x = i - 0.5 - nx / 2;
            double y = j - 0.5 - ny / 2;
            coordinates.components({ i, j })[0] = x * scale;
            coordinates.components({ i, j })[1] = y * scale;
        }
    }

    REQUIRE(coordinates.components({ 3, 4 })[0] - coordinates.components({ 2, 4 })[0] == scale);
    REQUIRE(coordinates.components({ 3, 3 })[1] - coordinates.components({ 3, 2 })[1] == scale);

    HField x;
    HField y;
    x.resize();
    y.resize();
    // Element coordinates
    for (size_t j = 0; j < ModelArray::size(ModelArray::Dimension::Y); ++j) {
        double yy = scale * (j - ny / 2);
        for (size_t i = 0; i < ModelArray::size(ModelArray::Dimension::X); ++i) {
            double xx = scale * (i - nx / 2);
            x(i, j) = xx;
            y(i, j) = yy;
        }
    }

    HField gridAzimuth;
    double gridAzimuth0 = 45.;
    gridAzimuth = gridAzimuth0;

    ModelState state = { {
                             { maskName, mask },
                             { hiceName, hice },
                             { ciceName, cice },
                             { hsnowName, hsnow },
                             { ticeName, tice },
                         },
        {} };

    // A model state to set the coordinates in the metadata object
    ModelState coordState = { {
                                  { xName, x },
                                  { yName, y },
                                  { coordsName, coordinates },
                                  { gridAzimuthName, gridAzimuth },
                              },
        {} };

    ModelMetadata metadata;
    metadata.setTime(TimePoint(date_string));
    // The coordinates are passed through the metadata object as affix
    // coordinates is the correct way to add coordinates to a ModelState
    metadata.extractCoordinates(coordState);
    metadata.affixCoordinates(state);

// Write reference file
#ifdef USE_MPI
    // Create subcommunicator with only first rank
    metadata.setMpiMetadata(test_comm);
    int colour = MPI_UNDEFINED, key = 0;
    MPI_Comm rank0Comm;

    if(metadata.mpiMyRank == 0) {
        colour = 0;
    }
    MPI_Comm_split(test_comm, colour, key, &rank0Comm);

    // Write reference file serially on first MPI rank
    if (metadata.mpiMyRank == 0) {
        metadata.setMpiMetadata(rank0Comm);
        metadata.globalExtentX = nx;
        metadata.globalExtentY = ny;
        metadata.localCornerX = 0;
        metadata.localCornerY = 0;
        metadata.localExtentX = nx;
        metadata.localExtentY = ny;
        grid.dumpModelState(state, metadata, filename, true);
        pio->close(filename);
        MPI_Comm_free(&rank0Comm);
    }
    metadata.setMpiMetadata(test_comm);
    // Barrier to prevent not 0 ranks to reach filepath test first
    MPI_Barrier(test_comm);
#else
    grid.dumpModelState(state, metadata, filename, true);
#endif

    REQUIRE(std::filesystem::exists(std::filesystem::path(filename)));

    // Reset the array dimensions to make sure that the read function gets them correct
    ModelArray::setDimension(ModelArray::Dimension::X, 1);
    ModelArray::setDimension(ModelArray::Dimension::Y, 1);
    ModelArray::setDimension(ModelArray::Dimension::Z, 1);
    ModelArray::setDimension(ModelArray::Dimension::XVERTEX, 1);
    ModelArray::setDimension(ModelArray::Dimension::YVERTEX, 1);
    ModelArray::setDimension(ModelArray::Dimension::XCG, 1);
    ModelArray::setDimension(ModelArray::Dimension::YCG, 1);
    // In the full model numbers of DG components are set at compile time, so they are not reset
    REQUIRE(ModelArray::nComponents(ModelArray::Type::DG) == DG);
    REQUIRE(ModelArray::nComponents(ModelArray::Type::VERTEX) == ModelArray::nCoords);

    ParametricGrid gridIn;
    ParaGridIO* readIO = new ParaGridIO(gridIn);
    gridIn.setIO(readIO);

#ifdef USE_MPI
    ModelMetadata metadataIn(partition_filename, test_comm);
    metadataIn.setTime(TimePoint(date_string));
    ModelState ms = gridIn.getModelState(filename, metadataIn);
#else
    ModelState ms = gridIn.getModelState(filename);
#endif

    REQUIRE(ModelArray::dimensions(ModelArray::Type::Z)[0] == nx);
    REQUIRE(ModelArray::dimensions(ModelArray::Type::Z)[1] == ny);
    REQUIRE(ModelArray::dimensions(ModelArray::Type::Z)[2] == NZLevels::get());

    REQUIRE(ms.data.size() == state.data.size());

    ModelArray& ticeRef = ms.data.at(ticeName);
    REQUIRE(ModelArray::nDimensions(ModelArray::Type::Z) == 3);
    REQUIRE(ticeRef.getType() == ModelArray::Type::Z);
    REQUIRE(ticeRef.nDimensions() == 3);
    REQUIRE(ticeRef.dimensions()[0] == nx);
    REQUIRE(ticeRef.dimensions()[1] == ny);
    REQUIRE(ticeRef.dimensions()[2] == NZLevels::get());

    ModelArray& hiceRef = ms.data.at(hiceName);
    REQUIRE(hiceRef.nDimensions() == 2);
    REQUIRE(hiceRef.dimensions()[0] == nx);
    REQUIRE(hiceRef.dimensions()[1] == ny);
    REQUIRE(ModelArray::nComponents(ModelArray::Type::DG) == DG);
    REQUIRE(hiceRef.nComponents() == DG);

    REQUIRE(ticeRef(3, 4, 1) == tice(3, 4, 1));

    // Here we don't bother passing the coordinate arrays through a ModelMetadata object
    ModelArray& coordRef = ms.data.at(coordsName);
    REQUIRE(coordRef.nDimensions() == 2);
    REQUIRE(coordRef.nComponents() == 2);
    REQUIRE(coordRef.dimensions()[0] == nx + 1);
    REQUIRE(coordRef.dimensions()[1] == ny + 1);
    REQUIRE(coordRef.components({ 2, 3 })[0] - coordRef.components({ 1, 3 })[0] == scale);
    REQUIRE(coordRef.components({ 2, 3 })[1] - coordRef.components({ 2, 2 })[1] == scale);

    REQUIRE(ms.data.count(xName) > 0);
    ModelArray& xRef = ms.data.at(xName);
    REQUIRE(xRef(2, 3) == coordRef.components({ 2, 3 })[0] + scale / 2);

    REQUIRE(ms.data.count(yName) > 0);
    ModelArray& yRef = ms.data.at(yName);
    REQUIRE(yRef(2, 3) == coordRef.components({ 2, 3 })[1] + scale / 2);

    REQUIRE(ms.data.count(gridAzimuthName) > 0);
    REQUIRE(ms.data.at(gridAzimuthName)(0, 0) == gridAzimuth0);
//    std::filesystem::remove(filename);
}

//#ifdef USE_MPI
//MPI_TEST_CASE("Write a diagnostic ParaGrid file", TEST_MPI_SIZE)
//#else
//TEST_CASE("Write a diagnostic ParaGrid file")
//#endif
//{
//    Module::setImplementation<IStructure>("Nextsim::ParametricGrid");
//
//    REQUIRE(Module::getImplementation<IStructure>().structureType() == "parametric_rectangular");
//
//    std::filesystem::remove(diagFile);
//
//    ParametricGrid grid;
//    ParaGridIO* pio = new ParaGridIO(grid);
//    grid.setIO(pio);
//
//    // Set the dimension lengths
//    size_t nx = 30;
//    size_t ny = 20;
//    size_t nz = 3;
//    NZLevels::set(nz);
//    size_t nxcg = CG * nx + 1;
//    size_t nycg = CG * ny + 1;
//
//    double yFactor = 0.01;
//    double xFactor = 0.0001;
//
//    ModelArray::setDimension(ModelArray::Dimension::X, nx);
//    ModelArray::setDimension(ModelArray::Dimension::Y, ny);
//    ModelArray::setDimension(ModelArray::Dimension::Z, NZLevels::get());
//    ModelArray::setDimension(ModelArray::Dimension::XVERTEX, nx + 1);
//    ModelArray::setDimension(ModelArray::Dimension::YVERTEX, ny + 1);
//    ModelArray::setDimension(ModelArray::Dimension::XCG, nxcg);
//    ModelArray::setDimension(ModelArray::Dimension::YCG, nycg);
//
//    ModelArray::setNComponents(ModelArray::Type::DG, DG);
//    ModelArray::setNComponents(ModelArray::Type::DGSTRESS, DGSTRESS);
//    ModelArray::setNComponents(ModelArray::Type::VERTEX, ModelArray::nCoords);
//
//    HField fractional(ModelArray::Type::H);
//    DGField fractionalDG(ModelArray::Type::DG);
//    HField mask(ModelArray::Type::H);
//    fractional.resize();
//    fractionalDG.resize();
//    for (size_t j = 0; j < ny; ++j) {
//        for (size_t i = 0; i < nx; ++i) {
//            fractional(i, j) = j * yFactor + i * xFactor;
//            mask(i, j) = fractional(i, j);
//            //                = (i - nx / 2) * (i - nx / 2) + (j - ny / 2) * (j - ny / 2) > (nx *
//            //                ny) ? 0 : 1;
//            for (size_t d = 0; d < DG; ++d) {
//                fractionalDG.components({ i, j })[d] = fractional(i, j) + d;
//            }
//        }
//    }
//    double prec = 1e-9;
//    REQUIRE(fractional(2, 2) - fractional(1, 2) == doctest::Approx(xFactor).epsilon(prec));
//    REQUIRE(fractional(2, 2) - fractional(2, 1) == doctest::Approx(yFactor).epsilon(prec));
//
//    REQUIRE(fractionalDG(2, 2) - fractionalDG(1, 2) == doctest::Approx(xFactor).epsilon(prec));
//    REQUIRE(fractionalDG(2, 2) - fractionalDG(2, 1) == doctest::Approx(yFactor).epsilon(prec));
//
//    DGField hice = fractionalDG + 10;
//    DGField cice = fractionalDG + 20;
//
//    VertexField coordinates(ModelArray::Type::VERTEX);
//    coordinates.resize();
//    // Planar coordinates
//    double scale = 1e5;
//
//    for (size_t i = 0; i < ModelArray::definedDimensions.at(ModelArray::Dimension::XVERTEX).length;
//         ++i) {
//        for (size_t j = 0;
//             j < ModelArray::definedDimensions.at(ModelArray::Dimension::YVERTEX).length; ++j) {
//            double x = i - 0.5 - nx / 2;
//            double y = j - 0.5 - ny / 2;
//            coordinates.components({ i, j })[0] = x * scale;
//            coordinates.components({ i, j })[1] = y * scale;
//        }
//    }
//
//    REQUIRE(coordinates.components({ 2, 3 })[0] - coordinates.components({ 1, 3 })[0] == scale);
//    REQUIRE(coordinates.components({ 2, 3 })[1] - coordinates.components({ 2, 2 })[1] == scale);
//
//    HField x;
//    HField y;
//    x.resize();
//    y.resize();
//    // Element coordinates
//    for (size_t j = 0; j < ModelArray::size(ModelArray::Dimension::Y); ++j) {
//        double yy = scale * (j - ny / 2);
//        for (size_t i = 0; i < ModelArray::size(ModelArray::Dimension::X); ++i) {
//            double xx = scale * (i - nx / 2);
//            x(i, j) = xx;
//            y(i, j) = yy;
//        }
//    }
//
//    HField gridAzimuth;
//    double gridAzimuth0 = 45.;
//    gridAzimuth = gridAzimuth0;
//
//    ModelState state = { {
//                             { maskName, mask },
//                             { hiceName, hice },
//                             { ciceName, cice },
//                         },
//        {} };
//
//    // A model state to set the coordinates in the metadata object
//    ModelState coordState = { {
//                                  { xName, x },
//                                  { yName, y },
//                                  { coordsName, coordinates },
//                                  { gridAzimuthName, gridAzimuth },
//                              },
//        {} };
//
//    ModelMetadata metadata;
//    metadata.setTime(TimePoint(date_string));
//    // The coordinates are passed through the metadata object as affix
//    // coordinates is the correct way to add coordinates to a ModelState
//    metadata.extractCoordinates(coordState);
//    metadata.affixCoordinates(state);
//
//    grid.dumpModelState(state, metadata, diagFile, false);
//
//    for (int t = 1; t < 5; ++t) {
//        hice += 100;
//        cice += 100;
//        state = { {
//                      { hiceName, hice },
//                      { ciceName, cice },
//                  },
//            {} };
//        metadata.incrementTime(Duration(3600));
//
//        grid.dumpModelState(state, metadata, diagFile, false);
//    }
//    pio->close(diagFile);
//
//    // What do we have in the file?
//    netCDF::NcFile ncFile(diagFile, netCDF::NcFile::read);
//
//    REQUIRE(ncFile.getGroups().size() == 3);
//    netCDF::NcGroup structGrp(ncFile.getGroup(IStructure::structureNodeName()));
//    netCDF::NcGroup metaGrp(ncFile.getGroup(IStructure::metadataNodeName()));
//    netCDF::NcGroup dataGrp(ncFile.getGroup(IStructure::dataNodeName()));
//
//    std::string structureType;
//    structGrp.getAtt(grid.typeNodeName()).getValues(structureType);
//    REQUIRE(structureType == grid.structureType());
//
//    // TODO test metadata
//
//    // test data
//    REQUIRE(dataGrp.getVarCount() == 8);
//    netCDF::NcVar hiceVar = dataGrp.getVar(hiceName);
//    netCDF::NcVar ciceVar = dataGrp.getVar(ciceName);
//    netCDF::NcVar maskVar = dataGrp.getVar(maskName);
//    netCDF::NcVar timeVar = dataGrp.getVar(timeName);
//
//    // hice
//    REQUIRE(hiceVar.getDimCount() == 4);
//
//    // coordinates
//    REQUIRE(dataGrp.getVars().count(xName) > 0);
//    REQUIRE(dataGrp.getVars().count(yName) > 0);
//    REQUIRE(dataGrp.getVars().count(coordsName) > 0);
//    REQUIRE(dataGrp.getVars().count(gridAzimuthName) > 0);
//
//    ncFile.close();
//
////    std::filesystem::remove(diagFile);
//}
//
//#define TO_STR(s) TO_STRI(s)
//#define TO_STRI(s) #s
//#ifndef TEST_FILE_SOURCE
//#define TEST_FILE_SOURCE .
//#endif
//
//#ifdef USE_MPI
//MPI_TEST_CASE("Test array ordering", TEST_MPI_SIZE)
//#else
//TEST_CASE("Test array ordering")
//#endif
//{
//    std::string inputFilename = "ParaGridIO_input_test.nc";
//
//    Module::setImplementation<IStructure>("Nextsim::ParametricGrid");
//
//    REQUIRE(Module::getImplementation<IStructure>().structureType() == "parametric_rectangular");
//
//    size_t nx = 9;
//    size_t ny = 11;
//    NZLevels::set(1);
//
//    double xFactor = 10;
//
//    ModelArray::setDimension(ModelArray::Dimension::X, nx);
//    ModelArray::setDimension(ModelArray::Dimension::Y, ny);
//    ModelArray::setDimension(ModelArray::Dimension::Z, NZLevels::get());
//
//    HField index2d(ModelArray::Type::H);
//    index2d.resize();
//    std::string fieldName = "index2d";
//    std::set<std::string> fields = { fieldName };
//    TimePoint time;
//
//    ModelState state = ParaGridIO::readForcingTimeStatic(
//        fields, time, TO_STR(TEST_FILE_SOURCE) + std::string("/") + inputFilename);
//    REQUIRE(state.data.count(fieldName) > 0);
//    index2d = state.data.at(fieldName);
//    REQUIRE(index2d(3, 5) == 35);
//    // And that's all that's needed
//}
//
//#undef TO_STR
//#undef TO_STRI
//
//#ifdef USE_MPI
//MPI_TEST_CASE("Check an exception is thrown for an invalid file name", TEST_MPI_SIZE)
//#else
//TEST_CASE("Check an exception is thrown for an invalid file name")
//#endif
//{
//    ParametricGrid gridIn;
//    ParaGridIO* readIO = new ParaGridIO(gridIn);
//    gridIn.setIO(readIO);
//
//    ModelState state;
//
//    // MD5 hash of the current output of $ date
//    std::string longRandomFilename("a44f5cc1f7934a8ae8dd03a95308745d.nc");
//#ifdef USE_MPI
//    ModelMetadata metadataIn(partition_filename, test_comm);
//    metadataIn.setTime(TimePoint(date_string));
//    ModelState ms = gridIn.getModelState(filename, metadataIn);
//    REQUIRE_THROWS(state = gridIn.getModelState(longRandomFilename, metadataIn));
//    std::cout << "FOOOO\n";
//#else
//    REQUIRE_THROWS(state = gridIn.getModelState(longRandomFilename));
//#endif
//
//}
TEST_SUITE_END();

}
