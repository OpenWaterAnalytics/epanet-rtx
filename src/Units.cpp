//
//  Units.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#include <iostream>
#include <map>
#include <string>
#include "Units.h"

#include <boost/algorithm/string.hpp>

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
  
  m["dimensionless"]= RTX_DIMENSIONLESS;
  // pressure
  m["psi"] = RTX_PSI;
  m["pa"]  = RTX_PASCAL;
  m["kpa"] = RTX_KILOPASCAL;
  // distance
  m["ft"]  = RTX_FOOT;
  m["in"]  = RTX_INCH;
  m["m"]   = RTX_METER;
  m["cm"]  = RTX_CENTIMETER;
  // volume
  m["m3"]  = RTX_CUBIC_METER;
  m["gal"] = RTX_GALLON;
  m["mgal"]= RTX_MILLION_GALLON;
  m["liter"]=RTX_LITER;
  m["ft3"] = RTX_CUBIC_FOOT;
  // flow
  m["cms"]= RTX_CUBIC_METER_PER_SECOND;
  m["cfs"] = RTX_CUBIC_FOOT_PER_SECOND;
  m["gps"] = RTX_GALLON_PER_SECOND;
  m["gpm"] = RTX_GALLON_PER_MINUTE;
  m["mgd"] = RTX_MILLION_GALLON_PER_DAY;
  m["lps"] = RTX_LITER_PER_SECOND;
  m["lpm"] = RTX_LITER_PER_MINUTE;
  m["mld"] = RTX_MILLION_LITER_PER_DAY;
  m["m3/hr"]=RTX_CUBIC_METER_PER_HOUR;
  m["m3/d"]= RTX_CUBIC_METER_PER_DAY;
  m["acre-ft/d"]=RTX_ACRE_FOOT_PER_DAY;
  m["imgd"]= RTX_IMPERIAL_MILLION_GALLON_PER_DAY;
  // time
  m["s"]   = RTX_SECOND;
  m["min"] = RTX_MINUTE;
  m["hr"]  = RTX_HOUR;
  m["d"]   = RTX_DAY;
  // mass
  m["mg"]  = RTX_MILLIGRAM;
  m["g"]   = RTX_GRAM;
  m["kg"]  = RTX_KILOGRAM;
  // concentration
  m["mg/l"]= RTX_MILLIGRAMS_PER_LITER;
  // conductance
  m["us/cm"]=RTX_MICROSIEMENS_PER_CM;
  // velocity
  m["m/s"] = RTX_METER_PER_SECOND;
  m["fps"] = RTX_FOOT_PER_SECOND;
  m["ft/hr"] = RTX_FOOT_PER_HOUR;
  // acceleration
  m["m/s/s"] = RTX_METER_PER_SECOND_SECOND;
  m["ft/s/s"] = RTX_FOOT_PER_SECOND_SECOND;
  m["ft/hr/hr"] = RTX_FOOT_PER_HOUR_HOUR;

  return m;
}

// factory for string input
Units Units::unitOfType(const string& unitString) {
  map<string, Units> unitMap = Units::unitStringMap();
  string uStr = boost::algorithm::to_lower_copy(unitString);
  if (unitMap.find(uStr) != unitMap.end()) {
    return unitMap[uStr];
  }
  else {
    cerr << "WARNING: Units not recognized: " << uStr << " - defaulting to dimensionless." << endl;
    return RTX_DIMENSIONLESS;
  }
}


