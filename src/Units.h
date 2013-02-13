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

// convenience defines ------------ unit= conversion,   dimension (m,l,t,current,temp,amount,intensity)
#define RTX_DIMENSIONLESS           Units(1)
// Pressure
#define RTX_PSI                     Units(0.0001450378911491,1,-1,-2)
#define RTX_PASCAL                  Units(1,            1,-1,-2)
#define RTX_KILOPASCAL              Units(.001,         1,-1,-2)
// distance
#define RTX_FOOT                    Units(.3048,        0,1,0)
#define RTX_INCH                    Units(.0254,        0,1,0)
#define RTX_METER                   Units(1,            0,1,0)
#define RTX_CENTIMETER              Units(.01,          0,1,0)
// volume
#define RTX_CUBIC_METER             Units(1,            0,3,0)
#define RTX_GALLON                  Units(.00378541,    0,3,0)
#define RTX_MILLION_GALLON          Units(3785.41178,   0,3,0)
#define RTX_LITER                   Units(.001,         0,3,0)
#define RTX_CUBIC_FOOT              Units(.0283168466,  0,3,0)
//flow
#define RTX_CUBIC_METER_PER_SECOND  Units(1,            0,3,-1)
#define RTX_CUBIC_FOOT_PER_SECOND   Units(.0283168466,  0,3,-1)
#define RTX_GALLON_PER_SECOND       Units(.00378541178, 0,3,-1)
#define RTX_GALLON_PER_MINUTE       Units(.00006309020, 0,3,-1)
#define RTX_MILLION_GALLON_PER_DAY  Units(.0438126364,  0,3,-1)
#define RTX_LITER_PER_SECOND        Units(.001,         0,3,-1)
#define RTX_LITER_PER_MINUTE        Units(.00001666667, 0,3,-1)
#define RTX_MILLION_LITER_PER_DAY   Units(.0115740741,  0,3,-1)
#define RTX_CUBIC_METER_PER_HOUR    Units(.000277777778,0,3,-1)
#define RTX_CUBIC_METER_PER_DAY     Units(.000011574074,0,3,-1)
#define RTX_ACRE_FOOT_PER_DAY       Units(.0142764102,  0,3,-1)
#define RTX_IMPERIAL_MILLION_GALLON_PER_DAY Units(.0526168042,0,3,-1)
// time
#define RTX_SECOND                  Units(1,            0,0,1)
#define RTX_MINUTE                  Units(60,           0,0,1)
#define RTX_HOUR                    Units(3600,         0,0,1)
#define RTX_DAY                     Units(86400,        0,0,1)
// mass
#define RTX_MILLIGRAM               Units(.000001       1,0,0)
#define RTX_GRAM                    Units(.001          1,0,0)
#define RTX_KILOGRAM                Units(1,            1,0,0)
// concentration
#define RTX_MILLIGRAMS_PER_LITER    Units(.001,         1,-3,0)
// conductance
#define RTX_MICROSIEMENS_PER_CM     Units(.0001,        -1,-3,3,2)


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
  
  
  
  
  
  
  class Units {
  public:
    Units(double conversion = 1., int mass = 0, int length = 0, int time = 0, int current = 0, int temperature = 0, int amount = 0, int intensity = 0);
    
    Units operator*(const Units& unit) const;
    Units operator/(const Units& unit) const;
    bool operator==(const Units& unit) const;
    
    bool isSameDimensionAs(const Units& unit) const;
    bool isDimensionless();
    double conversion() const;
    static double convertValue(double value, const Units& fromUnits, const Units& toUnits);
    static Units unitOfType(const std::string& unitString);
    static std::map<std::string, Units> unitStringMap();
    std::string unitString();
    
    virtual std::ostream& toStream(std::ostream &stream);
    
  private:
    int _length;
    int _mass;
    int _time;
    int _current;
    int _temperature;
    int _amount;
    int _intensity;
    double _conversion;
  };
  
  std::ostream& operator<< (std::ostream &out, Units &unit);

} // namespace



#endif
