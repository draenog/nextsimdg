/*!
 * @file SimpleOutput.cpp
 *
 * @date May 25, 2022
 * @author Tim Spain <timothy.spain@nersc.no>
 */

#include "include/Logged.hpp"
#include "include/SimpleOutput.hpp"
#include "include/StructureFactory.hpp"

#include <iostream>
#include <sstream>

namespace Nextsim {

void SimpleOutput::outputState(const ModelState& state, const ModelMetadata& meta)
{
    std::stringstream startStream;
    startStream << meta.time();
    std::string timeFileName = m_filePrefix + "." + startStream.str() + ".nc";
    Logged::info("Outputting " + std::to_string(state.size()) + " fields to " + timeFileName + "\n");
//    std::cout << "Outputting " << state.size() << " fields to " << timeFileName << std::endl;

    StructureFactory::fileFromState(state, meta, timeFileName);
}
} /* namespace Nextsim */
