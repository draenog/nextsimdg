/*
 * @file IIceOceanHeatFlux.hpp
 *
 * @date Oct 19, 2021
 * @author Tim Spain <timothy.spain@nersc.no>
 */

#ifndef SRC_INCLUDE_IICEOCEANHEATFLUX_HPP_
#define SRC_INCLUDE_IICEOCEANHEATFLUX_HPP_

namespace Nextsim {
class PrognosticData;
class ExternalData;
class PhysicsData;
class NextsimPhysics;

class IIceOceanHeatFlux {
public:
    virtual ~IIceOceanHeatFlux() = default;
    virtual double flux(const PrognosticData&, const ExternalData&, const PhysicsData&, const NextsimPhysics&)
        = 0;
};
}
#endif /* SRC_INCLUDE_IICEOCEANHEATFLUX_HPP_ */
