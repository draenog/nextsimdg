/*!
 * @file IIceAlbedoModule.cpp
 *
 * @date Feb 21, 2022
 * @author Tim Spain <timothy.spain@nersc.no>
 */

#include "include/IIceAlbedoModule.hpp"

#include "include/CCSMIceAlbedo.hpp"
#include "include/SMU2IceAlbedo.hpp"
#include "include/SMUIceAlbedo.hpp"

#include <string>

namespace Module {
const std::string SMUICEALBEDO = "SMUIceAlbedo";
const std::string SMU2ICEALBEDO = "SMU2IceAlbedo";
const std::string CCSMICEALBEDO = "CCSMIceAlbedo";

template <>
Module<Nextsim::IIceAlbedo>::map Module<Nextsim::IIceAlbedo>::functionMap = {
    { SMUICEALBEDO, newImpl<Nextsim::IIceAlbedo, Nextsim::SMUIceAlbedo> },
    { SMU2ICEALBEDO, newImpl<Nextsim::IIceAlbedo, Nextsim::SMU2IceAlbedo> },
    { CCSMICEALBEDO, newImpl<Nextsim::IIceAlbedo, Nextsim::CCSMIceAlbedo> },
};

template <>
Module<Nextsim::IIceAlbedo>::fn Module<Nextsim::IIceAlbedo>::spf = functionMap.at(SMUICEALBEDO);
template <>
std::unique_ptr<Nextsim::IIceAlbedo> Module<Nextsim::IIceAlbedo>::staticInstance
    = std::move(Module<Nextsim::IIceAlbedo>::spf());

template <> std::string Module<Nextsim::IIceAlbedo>::moduleName() { return "IIceAlbedo"; }

template <> Nextsim::IIceAlbedo& getImplementation<Nextsim::IIceAlbedo>()
{
    return getImplTemplate<Nextsim::IIceAlbedo, IIceAlbedoModule>();
}
template <> void setImplementation<Nextsim::IIceAlbedo>(const std::string& implName)
{
    setImplTemplate<IIceAlbedoModule>(implName);
}
template <> std::unique_ptr<Nextsim::IIceAlbedo> getInstance()
{
    return getInstTemplate<Nextsim::IIceAlbedo, IIceAlbedoModule>();
}
} /* namespace Module */