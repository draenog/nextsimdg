/*!
 * @file NextsimPhysics.hpp
 * @date Sep 9, 2021
 * @author Tim Spain <timothy.spain@nersc.no>
 */

#include <memory>

#include "BaseElementData.hpp"
#include "Configured.hpp"

#ifndef SRC_INCLUDE_NEXTSIMPHYSICS_HPP
#define SRC_INCLUDE_NEXTSIMPHYSICS_HPP

namespace Nextsim {

class PrognosticData;
class PhysicsData;
class ExternalData;
class UnusedData;

class IIceAlbedo;
class IThermodynamics;
class IIceOceanHeatFlux;
class IConcentrationModel;

template <class Phys> class ElementData;

class NextsimPhysics;

class NextsimPhysics : public BaseElementData, Configured<NextsimPhysics> {
public:
    NextsimPhysics();

    void configure() override;
    enum {
        DRAGOCEANQ_KEY,
        DRAGOCEANT_KEY,
        DRAGICET_KEY,
        I0_KEY,
        MINC_KEY,
    };

    inline static void updateDerivedData(
        const PrognosticData& prog, const ExternalData& exter, PhysicsData& phys, const UnusedData&)
    {
        updateDerivedDataStatic(prog, exter, phys);
    }
    inline static void massFluxOpenWater(
        const UnusedData&, const UnusedData&, PhysicsData& phys, const UnusedData&)
    {
        massFluxOpenWaterStatic(phys);
    };
    inline static void momentumFluxOpenWater(
        const UnusedData&, const UnusedData&, PhysicsData& phys, const UnusedData&)
    {
        momentumFluxOpenWaterStatic(phys);
    };
    inline static void heatFluxOpenWater(
        const PrognosticData& prog, const ExternalData& exter, PhysicsData& phys, UnusedData&)
    {
        heatFluxOpenWaterStatic(prog, exter, phys);
    }
    inline static void massFluxIceAtmosphere(
        const PrognosticData& prog, const ExternalData& exter, PhysicsData& phys, UnusedData&)
    {
        massFluxIceAtmosphereStatic(prog, exter, phys);
    };
    inline static void heatFluxIceAtmosphere(
        const PrognosticData& prog, const ExternalData& exter, PhysicsData& phys, UnusedData&)
    {
        heatFluxIceAtmosphereStatic(prog, exter, phys);
    };
    inline static void massFluxIceOcean(const PrognosticData& prog, const ExternalData& exter,
        PhysicsData& phys, NextsimPhysics& nsphys)
    {
        massFluxIceOceanStatic(prog, exter, phys, nsphys);
    };
    //! The thickness of newly created ice in the current timestep
    inline double newIce() const { return m_newice; };

    void heatFluxIceOcean(const PrognosticData& prog, const ExternalData& exter, PhysicsData& phys);

    static void setDragOcean_q(double dragOcean_q);
    static void setDragOcean_t(double dragOcean_t);
    static void setDragIce_t(double dragIce_t);
    static void setI0(double I0);

    class SpecificHumidity {
    public:
        SpecificHumidity();
        double operator()(const double temperature, const double pressure) const;
        double operator()(
            const double temperature, const double pressure, const double salinity) const;

    protected:
        SpecificHumidity(double, double, double, double, double, double, double);
        double f(const double pressurePa, const double temperature) const;
        double est(const double temperature, const double salinity) const;
        const double m_a;
        const double m_b;
        const double m_c;
        const double m_d;
        const double m_bigA;
        const double m_bigB;
        const double m_bigC;
        const double m_alpha;
        const double m_beta;
    };
    class SpecificHumidityIce : public SpecificHumidity {
    public:
        SpecificHumidityIce();
        double operator()(const double temperature, const double pressure) const;
        double dq_dT(const double temperature, const double pressure) const;
    };

private:
    static void updateDerivedDataStatic(
        const PrognosticData& prog, const ExternalData& exter, PhysicsData& phys);

    static void massFluxOpenWaterStatic(PhysicsData& phys);
    static void momentumFluxOpenWaterStatic(PhysicsData& phys);
    static void heatFluxOpenWaterStatic(
        const PrognosticData& prog, const ExternalData& exter, PhysicsData& phys);

    static void massFluxIceAtmosphereStatic(
        const PrognosticData& prog, const ExternalData& exter, PhysicsData& phys);
    static void heatFluxIceAtmosphereStatic(
        const PrognosticData& prog, const ExternalData& exter, PhysicsData& phys);
    static void massFluxIceOceanStatic(const PrognosticData& prog, const ExternalData& exter,
        PhysicsData& phys, NextsimPhysics& nsphys);
    static void lateralGrowth(const PrognosticData& prog, const ExternalData& exter,
        PhysicsData& phys, NextsimPhysics& nsphys);

    static double dragOcean_q;
    static double dragOcean_m(double windSpeed);
    static double dragOcean_t;
    static double dragIce_t;

    // Private instance functions
    // Ice formation due to cooling below freezing
    void supercool(const PrognosticData& prog, const ExternalData& exter, PhysicsData& phys);

    // Ice-ocean heat flux
    double m_Qio; // Ice-ocean heat flux
    static IIceOceanHeatFlux* iceOceanHeatFluxImpl;
    // New ice created by cooling below freezing
    double m_newice;

    static IConcentrationModel* iConcentrationModelImpl;

public:
    static double I_0;

private:
    static double latentHeatWater(double temperature);
    static double latentHeatIce(double temperature);

    static double minc; // minimum ice concentration

    static SpecificHumidity specificHumidityWater;
    static SpecificHumidityIce specificHumidityIce;

    static std::unique_ptr<IIceAlbedo> iIceAlbedoImpl;
    static IThermodynamics* iThermo;
};

} /* namespace Nextsim */

#endif /* SRC_INCLUDE_NEXTSIMPHYSICS_HPP */
