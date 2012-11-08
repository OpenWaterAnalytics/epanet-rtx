//
//  Units.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#include <iostream>
#include "Units.h"

using namespace RTX;


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


std::ostream& RTX::operator<< (std::ostream &out, Units &unit) {
  return unit.toStream(out);
}

std::ostream& Units::toStream(std::ostream &stream) {
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


// class methods
double Units::convertValue(double value, const Units& fromUnits, const Units& toUnits) {
  if (fromUnits.isSameDimensionAs(toUnits)) {
    return (value * fromUnits._conversion / toUnits._conversion);
  }
  else {
    std::cerr << "Units are not dimensionally consistent" << std::endl;
    return 0.;
  }
}

// factory for string input
Units Units::unitOfType(const std::string& unitString) {
  if (RTX_STRINGS_ARE_EQUAL(unitString, "gpm")) {
    return RTX_GALLON_PER_MINUTE;
  }
  else if (RTX_STRINGS_ARE_EQUAL(unitString, "mgd")) {
    return RTX_MILLION_GALLON_PER_DAY;
  }
  else if (RTX_STRINGS_ARE_EQUAL(unitString, "mld")) {
    return RTX_MILLION_LITER_PER_DAY;
  }
  else if (RTX_STRINGS_ARE_EQUAL(unitString, "cfs")) {
    return RTX_CUBIC_FOOT_PER_SECOND;
  }
  else if (RTX_STRINGS_ARE_EQUAL(unitString, "lpm")) {
    return RTX_LITER_PER_MINUTE;
  }
  else if (RTX_STRINGS_ARE_EQUAL(unitString, "lps")) {
    return RTX_LITER_PER_SECOND;
  }
  
  else if (RTX_STRINGS_ARE_EQUAL(unitString, "m")) {
    return RTX_METER;
  }
  else if (RTX_STRINGS_ARE_EQUAL(unitString, "ft")) {
    return RTX_FOOT;
  }
  else if (RTX_STRINGS_ARE_EQUAL(unitString, "in")) {
    return RTX_INCH;
  }
  
  else if (RTX_STRINGS_ARE_EQUAL(unitString, "d")) {
    return RTX_DAY;
  }
  else if (RTX_STRINGS_ARE_EQUAL(unitString, "hr")) {
    return RTX_HOUR;
  }
  else if (RTX_STRINGS_ARE_EQUAL(unitString, "min")) {
    return RTX_MINUTE;
  }
  else if (RTX_STRINGS_ARE_EQUAL(unitString, "s")) {
    return RTX_SECOND;
  }
  
  else {
    std::cerr << "Units not recognized: " << unitString << std::endl;
    return RTX_DIMENSIONLESS;
  }
}


