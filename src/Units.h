//
//  Units.h
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#ifndef epanet_rtx_units_h
#define epanet_rtx_units_h

#include <string>
#include <map>

// convenience defines ------------ unit= conversion,   dimension (m,l,t,current,temp,amount,intensity)
#define RTX_DIMENSIONLESS           RTX::Units(1)
// Pressure
#define RTX_PSI                     RTX::Units(6894.75728,   1,-1,-2)
#define RTX_PASCAL                  RTX::Units(1,            1,-1,-2)
#define RTX_KILOPASCAL              RTX::Units(1000,         1,-1,-2)
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
#define RTX_CUBIC_FOOT_PER_SECOND   RTX::Units(.0283168466,  0,3,-1)
#define RTX_GALLON_PER_SECOND       RTX::Units(.00378541178, 0,3,-1)
#define RTX_GALLON_PER_MINUTE       RTX::Units(.00006309020, 0,3,-1)
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
#define RTX_MILLIGRAM               RTX::Units(.000001,      1,0,0)
#define RTX_GRAM                    RTX::Units(.001,         1,0,0)
#define RTX_KILOGRAM                RTX::Units(1,            1,0,0)
// concentration
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
