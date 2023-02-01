//
//  Units.h
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#ifndef epanet_rtx_units_h
#define epanet_rtx_units_h

#include "rtxMacros.h"
#include <string>
#include <map>

// convenience defines ------------ unit= conversion,   dimension (m,l,t,current,temp,amount,intensity)
#define RTX_NO_UNITS                RTX::Units(0)
#define RTX_DIMENSIONLESS           RTX::Units(1)
#define RTX_HERTZ                   RTX::Units(1,            0,0,-1)
#define RTX_RPM                     RTX::Units(0.016666666666667,0,0,-1)
#define RTX_PERCENT                 RTX::Units(0.01)
// Pressure
#define RTX_PSI                     RTX::Units(6894.75728,   1,-1,-2)
#define RTX_PASCAL                  RTX::Units(1,            1,-1,-2)
#define RTX_KILOPASCAL              RTX::Units(1000,         1,-1,-2)
#define RTX_BAR                     RTX::Units(100000,       1,-1,-2)
// distance
#define RTX_FOOT                    RTX::Units(.3048,        0,1,0)
#define RTX_INCH                    RTX::Units(.0254,        0,1,0)
#define RTX_METER                   RTX::Units(1,            0,1,0)
#define RTX_CENTIMETER              RTX::Units(.01,          0,1,0)
// volume
#define RTX_CUBIC_METER             RTX::Units(1,            0,3,0)
#define RTX_GALLON                  RTX::Units(.00378541,    0,3,0)
#define RTX_MILLION_GALLON          RTX::Units(3785.41178,   0,3,0)
#define RTX_LITER                   RTX::Units(.001,         0,3,0)
#define RTX_CUBIC_FOOT              RTX::Units(.0283168466,  0,3,0)
//flow
#define RTX_CUBIC_METER_PER_SECOND  RTX::Units(1,            0,3,-1)
#define RTX_THOUSAND_CUBIC_METER_PER_DAY RTX::Units(.0115740741,0,3,-1)
#define RTX_CUBIC_FOOT_PER_SECOND   RTX::Units(.0283168466,  0,3,-1)
#define RTX_GALLON_PER_SECOND       RTX::Units(.00378541178, 0,3,-1)
#define RTX_GALLON_PER_MINUTE       RTX::Units(.00006309020, 0,3,-1)
#define RTX_GALLON_PER_DAY          RTX::Units(43.812638888E-9, 0,3,-1)
#define RTX_MILLION_GALLON_PER_DAY  RTX::Units(.0438126364,  0,3,-1)
#define RTX_LITER_PER_SECOND        RTX::Units(.001,         0,3,-1)
#define RTX_LITER_PER_MINUTE        RTX::Units(.00001666667, 0,3,-1)
#define RTX_MILLION_LITER_PER_DAY   RTX::Units(.0115740741,  0,3,-1)
#define RTX_CUBIC_METER_PER_HOUR    RTX::Units(.000277777778,0,3,-1)
#define RTX_CUBIC_METER_PER_DAY     RTX::Units(.000011574074,0,3,-1)
#define RTX_ACRE_FOOT_PER_DAY       RTX::Units(.0142764102,  0,3,-1)
#define RTX_IMPERIAL_MILLION_GALLON_PER_DAY RTX::Units(.0526168042,0,3,-1)
// time
#define RTX_SECOND                  RTX::Units(1,            0,0,1)
#define RTX_MINUTE                  RTX::Units(60,           0,0,1)
#define RTX_HOUR                    RTX::Units(3600,         0,0,1)
#define RTX_DAY                     RTX::Units(86400,        0,0,1)
// mass
#define RTX_MICROGRAM               RTX::Units(.000000001,   1,0,0)
#define RTX_MILLIGRAM               RTX::Units(.000001,      1,0,0)
#define RTX_GRAM                    RTX::Units(.001,         1,0,0)
#define RTX_KILOGRAM                RTX::Units(1,            1,0,0)
// concentration
#define RTX_MICROGRAMS_PER_LITER    RTX::Units(.000001,      1,-3,0)
#define RTX_MILLIGRAMS_PER_LITER    RTX::Units(.001,         1,-3,0)
// conductance
#define RTX_MICROSIEMENS_PER_CM     RTX::Units(.0001,        -1,-3,3,2)
// velocity
#define RTX_METER_PER_SECOND        RTX::Units(1,            0,1,-1)
#define RTX_FOOT_PER_SECOND         RTX::Units(.3048,        0,1,-1)
#define RTX_FOOT_PER_HOUR           RTX::Units(84.6666667E-6,0,1,-1)
// acceleration
#define RTX_METER_PER_SECOND_SECOND RTX::Units(1,            0,1,-2)
#define RTX_FOOT_PER_SECOND_SECOND  RTX::Units(.3048,        0,1,-2)
#define RTX_FOOT_PER_HOUR_HOUR      RTX::Units(25.5185185E-9,0,1,-2)

// temperature
#define RTX_DEGREE_KELVIN           RTX::Units(1,            0,0,0,0,1,0,0)
#define RTX_DEGREE_RANKINE          RTX::Units(5./9.,        0,0,0,0,1,0,0) //  K = R * 5/9
#define RTX_DEGREE_CELSIUS          RTX::Units(1,            0,0,0,0,1,0,0,273.15)
#define RTX_DEGREE_FARENHEIT        RTX::Units(5./9.,        0,0,0,0,1,0,0,459.67)

// power and energy
#define RTX_KILOWATT_HOUR          RTX::Units(3600000,       1,2,-2)
#define RTX_JOULE                  RTX::Units(1,             1,2,-2)
#define RTX_MEGAJOULE              RTX::Units(1000000,       1,2,-2)
#define RTX_WATT                   RTX::Units(1,             1,2,-3)
#define RTX_KILOWATT               RTX::Units(1000,          1,2,-3)

#define RTX_VOLT                   RTX::Units(1,             1,2,-3,-1) // (kg * m2) / ( A * s3)
#define RTX_AMP                    RTX::Units(1,             0,0,0,1)

// energy density
#define RTX_ENERGY_DENSITY        RTX::Units(3600000,        1,-1,-2)
#define RTX_ENERGY_DENSITY_MG     RTX::Units(951.01983668876,1,-1,-2)

// area
#define RTX_SQ_FOOT               RTX::Units(0.092903,       0,2)
#define RTX_SQ_METER              RTX::Units(1,              0,2)
#define RTX_SQ_INCH               RTX::Units(0.00064516,     0,2)
#define RTX_SQ_CENTIMETER         RTX::Units(0.0001,         0,2)

namespace RTX {
  /*!
   \class Units
   \brief Keep track of dimensions and units of measure.
   
   In general, any unit can be expressed as a set of mutually independent exponent factors of the
   quantities listed below. We shall support all of the seven (7) exponents, even though we expect to
   never use the candela, but why limit the future, right?
   
   - length = meter (m)
   - mass = kilogram (kg)
   - time = second (s)
   - electric current = ampere (A)
   - thermodynamic temperature = kelvin (K)
   - amount of substance = mole (mol)
   - luminous intensity = candela (cd)
   
  */
  /*!
   \fn Units::Units(double conversion, int mass, int length, int time, int current, int temperature, int amount, int intensity)
   \brief Create a new Units object with the specified dimensions and conversion factor.
   
   The default dimensional exponents are all zero (0), which means dimensionless. See Units.h for some predefined units of measure.
   
   */
  
  class Units : public RTX_object {
  public:
    RTX_BASE_PROPS(Units);
    const static std::map<std::string, Units> unitStrings;
    
    Units(double conversion = 1., int kilogram = 0, int meter = 0, int second = 0, int ampere = 0, int kelvin = 0, int mole = 0, int candela = 0, double offset = 0);
    Units(const std::string& type);
    Units operator*(const Units& unit) const;
    Units operator*(const double factor) const;
    Units operator/(const Units& unit) const;
    Units operator^(const double power) const;
    bool operator==(const Units& unit) const;
    bool operator!=(const Units& unit) const;
    
    const bool isInvalid() const;
    const bool isSameDimensionAs(const Units& unit) const;
    const bool isDimensionless() const;
    const double conversion() const;
    const double offset() const;
    static double convertValue(double value, const Units& fromUnits, const Units& toUnits);
    static Units unitOfType(const std::string& unitString);
    const std::string to_string() const;
    const std::string rawUnitString(bool ignoreZeroDimensions = true) const;
    
    virtual std::ostream& toStream(std::ostream &stream) const;
    
  private:
    int _m;
    int _kg;
    int _s;
    int _A;
    int _K;
    int _mol;
    int _cd;
    double _conversion;
    double _offset;
  };
  
  std::ostream& operator<< (std::ostream &out, Units &unit);

} // namespace



#endif
