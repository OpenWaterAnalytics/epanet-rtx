//
//  Units.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#include <iostream>
#include <map>
#include "Units.h"

using namespace RTX;
using namespace std;


Units::Units(double conversion, int mass, int length, int time, int current, int temperature, int amount, int intensity) {
  _mass         = mass;
  _length       = length;
  _time         = time;
  _current      = current;
  _temperature  = temperature;
  _amount       = amount;
  _intensity    = intensity;
  _conversion   = conversion;
}

double Units::conversion() const {
  return _conversion;
}


Units Units::operator*(const Units& unit) const {
  
  return Units(_conversion * unit._conversion,
               _mass + unit._mass,
               _length + unit._length,
               _time + unit._time,
               _current + unit._current,
               _temperature + unit._temperature,
               _amount + unit._amount,
               _intensity + unit._intensity);
}

Units Units::operator/(const Units& unit) const {
  
  return Units(_conversion / unit._conversion,
               _mass - unit._mass,
               _length - unit._length,
               _time - unit._time,
               _current - unit._current,
               _temperature - unit._temperature,
               _amount - unit._amount,
               _intensity - unit._intensity);
}

bool Units::operator==(const RTX::Units &unit) const {
  if (_conversion == unit._conversion && this->isSameDimensionAs(unit)) {
    return true;
  }
  return false;
}


bool Units::isSameDimensionAs(const Units& unit) const {
  
  if (_mass         == unit._mass         &&
      _length       == unit._length       &&
      _time         == unit._time         &&
      _current      == unit._current      &&
      _temperature  == unit._temperature  &&
      _amount       == unit._amount       &&
      _intensity    == unit._intensity ) {
    return true;
  }
  else {
    return false;
  }
}

bool Units::isDimensionless() {
  if (_mass         == 0  &&
      _length       == 0  &&
      _time         == 0  &&
      _current      == 0  &&
      _temperature  == 0  &&
      _amount       == 0  &&
      _intensity    == 0  ) {
    return true;
  }
  else {
    return false;
  }
}


ostream& RTX::operator<< (ostream &out, Units &unit) {
  return unit.toStream(out);
}

ostream& Units::toStream(ostream &stream) {
  if (isDimensionless()) {
    stream << "(dimensionless)";
    return stream;
  }
  
  stream << conversion();
  
  if (_mass != 0)    { stream << "*kilograms^"<< _mass; }
  if (_length != 0)  { stream << "*meters^"   << _length; }
  if (_time != 0)    { stream << "*seconds^"  << _time; }
  if (_current != 0) { stream << "*ampere^" << _current; }
  if (_temperature != 0) { stream << "*kelvin^" << _temperature; }
  if (_amount != 0) { stream << "*mole^" << _amount; }
  if (_intensity != 0) { stream << "*candela^" << _intensity; }
  
  return stream;
}


string Units::unitString() {
  map<string, Units> unitMap = Units::unitStringMap();
  
  map<string, Units>::const_iterator it = unitMap.begin();
  
  while (it != unitMap.end()) {
    Units theseUnits = it->second;
    if (theseUnits == (*this)) {
      return it->first;
    }
    ++it;
  }
  
  return "UNKNOWN UNITS";
  
}



// class methods
double Units::convertValue(double value, const Units& fromUnits, const Units& toUnits) {
  if (fromUnits.isSameDimensionAs(toUnits)) {
    return (value * fromUnits._conversion / toUnits._conversion);
  }
  else {
    cerr << "Units are not dimensionally consistent" << endl;
    return 0.;
  }
}


map<string, Units> Units::unitStringMap() {
  map<string, Units> m;
  
  m["none"]= RTX_DIMENSIONLESS;
  m["gpm"] = RTX_GALLON_PER_MINUTE;
  m["mgd"] = RTX_MILLION_GALLON_PER_DAY;
  m["mld"] = RTX_MILLION_LITER_PER_DAY;
  m["cfs"] = RTX_CUBIC_FOOT_PER_SECOND;
  m["lpm"] = RTX_LITER_PER_MINUTE;
  m["lps"] = RTX_LITER_PER_SECOND;
  m["m"]   = RTX_METER;
  m["ft"]  = RTX_FOOT;
  m["in"]  = RTX_INCH;
  m["d"]   = RTX_DAY;
  m["hr"]  = RTX_HOUR;
  m["min"] = RTX_MINUTE;
  m["s"]   = RTX_SECOND;
  m["psi"] = RTX_PSI;
  m["pa"]  = RTX_PASCAL;
  m["kpa"] = RTX_KILOPASCAL;
  
  return m;
}

// factory for string input
Units Units::unitOfType(const string& unitString) {
  map<string, Units> unitMap = Units::unitStringMap();
  if (unitMap.find(unitString) != unitMap.end()) {
    return unitMap[unitString];
  }
  else {
    cerr << "Units not recognized: " << unitString << endl;
    return RTX_DIMENSIONLESS;
  }
}


