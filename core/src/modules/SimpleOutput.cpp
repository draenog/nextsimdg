/*!
 * @file SimpleOutput.cpp
 *
 * @date May 25, 2022
 * @author Tim Spain <timothy.spain@nersc.no>
 */

#include "include/SimpleOutput.hpp"
#include "include/StructureFactory.hpp"

#include <iostream>
#include <sstream>

namespace Nextsim {

void SimpleOutput::outputState(const ModelState& state, const ModelMetadata& meta) const
{
    std::stringstream startStream;
    startStream << meta.time();
    std::string timeFileName = m_filePrefix + "." + startStream.str() + ".nc";
    std::cout << "Outputting " << state.size() << " fields to " << timeFileName << std::endl;

    StructureFactory::fileFromState(state, meta, timeFileName);
}
} /* namespace Nextsim */
