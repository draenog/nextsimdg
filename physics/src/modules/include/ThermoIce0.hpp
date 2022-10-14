/*!
 * @file ThermoIce0.hpp
 *
 * @date Mar 17, 2022
 * @author Tim Spain <timothy.spain@nersc.no>
 */

#ifndef THERMOICE0HPP
#define THERMOICE0HPP

#include "include/Configured.hpp"
#include "include/IIceThermodynamics.hpp"
#include "include/ModelArrayRef.hpp"
#include "include/IIceAlbedo.hpp"
#include "include/IIceAlbedoModule.hpp"
namespace Nextsim {

//! A class implementing IIceThermodynamics as the ThermoIce0 model.
class ThermoIce0 : public IIceThermodynamics, public Configured<ThermoIce0> {
public:
    ThermoIce0();
    virtual ~ThermoIce0() = default;

    enum {
        KS_KEY,
        I0_KEY,
    };
    void configure() override;

    ModelState getStateRecursive(const OutputSpec& os) const override;

    static HelpMap& getHelpText(HelpMap& map, bool getAll);
    static HelpMap& getHelpRecursive(HelpMap& map, bool getAll);

    void setData(const ModelState::DataMap&) override;
    void update(const TimestepTime& tsTime) override;

private:
    void calculateElement(size_t i, const TimestepTime& tst);

    HField snowMelt;
    HField topMelt;
    HField botMelt;
    HField qic;
    ModelArrayRef<ProtectedArray::HTRUE_ICE, MARConstBackingStore> oldHi;

    static double k_s;
    static double m_I0;
    static const double freezingPointIce;

    bool doFlooding = true; // TODO: read from configuration

    IIceAlbedo* iIceAlbedoImpl;
};

} /* namespace Nextsim */

#endif /* THERMOICE0HPP */
