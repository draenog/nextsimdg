/*!
 * @file ThermoIce0.cpp
 *
 * @date Mar 17, 2022
 * @author Tim Spain <timothy.spain@nersc.no>
 */

#include "include/ThermoIce0.hpp"

#include "include/IceGrowth.hpp"
#include "include/ModelArray.hpp"
#include "include/constants.hpp"

namespace Nextsim {

double ThermoIce0::k_s;
double ThermoIce0::m_I0;

static const double k_sDefault = 0.3096;
static const double i0_default = 0.17;

const double ThermoIce0::freezingPointIce = -Water::mu * Ice::s;

ThermoIce0::ThermoIce0()
    : iIceAlbedoImpl(nullptr)
    , IIceThermodynamics()
    , snowMelt(ModelArray::Type::H)
    , topMelt(ModelArray::Type::H)
    , botMelt(ModelArray::Type::H)
    , qic(ModelArray::Type::H)
    , oldHi(getProtectedArray())
{
}

void ThermoIce0::update(const TimestepTime& tsTime)
{
    iIceAlbedoImpl->setTime(tsTime.start);
    overElements(std::bind(&ThermoIce0::calculateElement, this, std::placeholders::_1,
                     std::placeholders::_2),
        tsTime);
}

template <>
const std::map<int, std::string> Configured<ThermoIce0>::keyMap = {
    { ThermoIce0::KS_KEY, "thermoice0.ks" },
    { ThermoIce0::I0_KEY, "thermoice0.I_0" },
};

void ThermoIce0::configure()
{
    iIceAlbedoImpl = &Module::getImplementation<IIceAlbedo>();
    tryConfigure(iIceAlbedoImpl);

    k_s = Configured::getConfiguration(keyMap.at(KS_KEY), k_sDefault);
    m_I0 = Configured::getConfiguration(keyMap.at(I0_KEY), i0_default);
}

ModelState ThermoIce0::getStateRecursive(const OutputSpec& os) const
{
    ModelState state = { {},
        {
            { keyMap.at(KS_KEY), k_s },
        } };
    return os ? state : ModelState();
}

ThermoIce0::HelpMap& ThermoIce0::getHelpText(HelpMap& map, bool getAll)
{
    map["ThermoIce0"] = {
        { keyMap.at(KS_KEY), ConfigType::NUMERIC, { "0", "∞" }, std::to_string(k_sDefault),
            "W K⁻¹ m⁻¹", "Thermal conductivity of snow." },
        { keyMap.at(I0_KEY), ConfigType::NUMERIC, { "0", "∞" }, std::to_string(i0_default), "",
            "Transmissivity of ice." },
    };

    Module::getHelpRecursive<IIceAlbedo>(map, getAll);

    return map;
}
ThermoIce0::HelpMap& ThermoIce0::getHelpRecursive(HelpMap& map, bool getAll)
{
    return getHelpText(map, getAll);
}

void ThermoIce0::setData(const ModelState::DataMap& ms)
{
    IIceThermodynamics::setData(ms);

    snowMelt.resize();
    topMelt.resize();
    botMelt.resize();
    qic.resize();
}

void ThermoIce0::calculateElement(size_t i, const TimestepTime& tst)
{
    static const double beta = 0.4;
    static const double gamma = 1.065;

    static const double bulkLHFusionSnow = Water::Lf * Ice::rhoSnow;
    static const double bulkLHFusionIce = Water::Lf * Ice::rho;

    // Create a reference to the local updated Tice value here to avoid having
    // to write the array access expression out in full every time
    double& tice_i = tice.zIndexAndLayer(i, 0);

    const double k_lSlab = k_s * Ice::kappa / (k_s * hice[i] + Ice::kappa * hsnow[i]);
    qic[i] = k_lSlab * (tf[i] - tice_i) * gamma;
    double albedoValue = iIceAlbedoImpl->albedo(tice.zIndexAndLayer(i, 0), hsnow[i]);
    if ( hsnow[i] == 0. )
        albedoValue += beta*(1.-albedoValue)*m_I0;

    const double remainingFlux = qic[i] - (qia[i] + (1.-albedoValue) * qsw[i]);
    tice_i += remainingFlux / (k_lSlab + dQia_dt[i]);

    // Clamp the temperature of the ice to a maximum of the melting point
    // of ice or snow
    const double meltingLimit = (hsnow[i] > 0) ? 0 : freezingPointIce;
    tice_i = std::min(meltingLimit, tice_i);

    // Top melt. Melting rate is non-positive.
    const double snowMeltRate = std::min(-remainingFlux, 0.) / bulkLHFusionSnow;
    snowMelt[i] = snowMeltRate * tst.step;
    const double snowSublRate = sublim[i] / Ice::rhoSnow;
    const double nowSnow = hsnow[i] + (snowMeltRate - snowSublRate) * tst.step;
    // Use excess flux to melt ice. Non-positive value
    const double excessIceMelt = std::min(nowSnow, 0.) * bulkLHFusionSnow / bulkLHFusionIce;
    // With the excess flux noted, clamp the snow thickness to a minimum of zero.
    hsnow[i] = std::max(nowSnow, 0.);

    // Bottom melt or growth
    const double iceBottomChange = (qic[i] - qio[i]) * tst.step / bulkLHFusionIce;
    // Total thickness change
    deltaHi[i] = excessIceMelt + iceBottomChange;
    hice[i] += deltaHi[i];

    // Then add snowfall back on top if there's still ice
    if ( hice[i] > 0. )
        hsnow[i] += snowfall[i] * tst.step / Ice::rhoSnow;

    // Amount of melting (only) at the top and bottom of the ice
    topMelt[i] = std::min(excessIceMelt, 0.);
    botMelt[i] = std::min(iceBottomChange, 0.);
    // Snow to ice conversion
    const double iceDraught = (hice[i] * Ice::rho + hsnow[i] * Ice::rhoSnow) / Water::rhoOcean;

    if (doFlooding && iceDraught > hice[i]) {
        double snowDraught = iceDraught - hice[i];
        snowToIce[i] = snowDraught;
        hsnow[i] -= snowDraught * Ice::rho / Ice::rhoSnow;
        hice[i] = iceDraught;
    } else {
        snowToIce[i] = 0;
    }

    // Melt all ice if it is below minimum threshold
    if (hice[i] < IceGrowth::minimumIceThickness()) {
        if (deltaHi[i] < 0) {
            const double scaling = oldHi[i] / deltaHi[i];
            topMelt[i] *= scaling;
            botMelt[i] *= scaling;
        }

        // No snow was converted to ice
        snowToIce[i] = 0.;

        // Change in thickness is all of the old thickness
        deltaHi[i] = -oldHi[i];

        // The ice-ocean flux includes all the latent heat
        qio[i] += hice[i] * bulkLHFusionIce / tst.step + hsnow[i] * bulkLHFusionSnow / tst.step;

        // No ice, no snow and the surface temperature is the melting point of ice
        hice[i] = 0.;
        hsnow[i] = 0.;
        tice_i = 0.;
    }
}
} /* namespace Nextsim */
