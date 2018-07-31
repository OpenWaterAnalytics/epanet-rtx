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
#include <vector>
#include <deque>
#include "Units.h"
#include "rtxMacros.h"

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>

using namespace RTX;
using namespace std;


Units::Units(double conversion, int mass, int length, int time, int current, int temperature, int amount, int intensity, double offset) {
  _kg         = mass;
  _m          = length;
  _s          = time;
  _A          = current;
  _K          = temperature;
  _mol        = amount;
  _cd         = intensity;
  _conversion = conversion;
  _offset     = offset;
}

Units::Units(const std::string& type) {
  Units u = Units::unitOfType(type);
  
  _kg = u._kg;
  _m = u._m;
  _s = u._s;
  _A = u._A;
  _K = u._K;
  _mol = u._mol;
  _cd = u._cd;
  _conversion = u._conversion;
  _offset = u._offset;
  
}

const double Units::conversion() const {
  return _conversion;
}
const double Units::offset() const {
  return _offset;
}


Units Units::operator*(const Units& unit) const {
  
  if ( this->isInvalid() || unit.isInvalid()) {
    return RTX_NO_UNITS;
  }
  
  return Units(_conversion * unit._conversion,
               _kg + unit._kg,
               _m + unit._m,
               _s + unit._s,
               _A + unit._A,
               _K + unit._K,
               _mol + unit._mol,
               _cd + unit._cd);
}

Units Units::operator*(const double factor) const {
  return Units(_conversion * factor,
               _kg, _m, _s, _A, _K, _mol, _cd, _offset);
}


Units Units::operator/(const Units& unit) const {
  if ( this->isInvalid() || unit.isInvalid()) {
    return RTX_NO_UNITS;
  }
  return Units(_conversion / unit._conversion,
               _kg - unit._kg,
               _m - unit._m,
               _s - unit._s,
               _A - unit._A,
               _K - unit._K,
               _mol - unit._mol,
               _cd - unit._cd);
}


Units Units::operator^(const double power) const {
  int mass = round(_kg*power);
  int length = round(_m*power);
  int time = round(_s*power);
  int current = round(_A*power);
  int temperature = round(_K*power);
  int amount = round(_mol*power);
  int intensity = round(_cd*power);
  
  return Units(pow(_conversion,power), mass, length, time, current, temperature, amount, intensity);
}

bool Units::operator==(const RTX::Units &unit) const {
  if (_conversion == unit._conversion &&
      _kg         == unit._kg       &&
      _m          == unit._m        &&
      _s          == unit._s        &&
      _A          == unit._A        &&
      _K          == unit._K        &&
      _mol        == unit._mol      &&
      _cd         == unit._cd       &&
      _offset     == unit._offset   ){
    return true;
  }
  return false;
}

bool Units::operator!=(const RTX::Units &unit) const {
  return !(*this == unit);
}

const bool Units::isInvalid() const {
  return this->isDimensionless() && this->conversion() == 0;
}

const bool Units::isSameDimensionAs(const Units& unit) const {
  
  if (_conversion == 0 || unit._conversion == 0) {
    // if no units assigned, can't be same dimension, right?
    return false;
  }
  if (_kg         == unit._kg         &&
      _m       == unit._m       &&
      _s         == unit._s         &&
      _A      == unit._A      &&
      _K  == unit._K  &&
      _mol       == unit._mol       &&
      _cd    == unit._cd ) {
    return true;
  }
  else {
    return false;
  }
}

const bool Units::isDimensionless() const {
  if (_kg         == 0  &&
      _m       == 0  &&
      _s         == 0  &&
      _A      == 0  &&
      _K  == 0  &&
      _mol       == 0  &&
      _cd    == 0  ) {
    return true;
  }
  else {
    return false;
  }
}


ostream& RTX::operator<< (ostream &out, Units &unit) {
  return unit.toStream(out);
}

ostream& Units::toStream(ostream &stream) const {
  if (isDimensionless() && conversion() == 1) {
    stream << "dimensionless";
    return stream;
  }
  else if (isDimensionless() && conversion() == 0) {
    stream << "no_units";
    return stream;
  }
  
  stream << this->rawUnitString(true);
  
  return stream;
}


const string Units::to_string() const {
  map<string, Units> unitMap = Units::unitStringMap;
  
  map<string, Units>::const_iterator it = unitMap.begin();
  
  while (it != unitMap.end()) {
    Units theseUnits = it->second;
    if (theseUnits == (*this)) {
      return it->first;
    } // OR APPROXIMATE UNITS
    else if (theseUnits.isSameDimensionAs(*this) &&
             fabs(theseUnits.conversion() - this->conversion())/this->conversion() < 0.00005 &&
             this->offset() == theseUnits.offset() ) {
      return it->first;
    }
    ++it;
  }
  
  return this->rawUnitString(true);
}

const string Units::rawUnitString(bool ignoreZeroDimensions) const {
  stringstream stream;
  stream << boost::lexical_cast<string>(_conversion);
  if (_kg != 0    || !ignoreZeroDimensions) { stream << "*[kg^"      << _kg         << "]"; }
  if (_m != 0     || !ignoreZeroDimensions) { stream << "*[m^"       << _m       << "]"; }
  if (_s != 0     || !ignoreZeroDimensions) { stream << "*[s^"       << _s         << "]"; }
  if (_A != 0     || !ignoreZeroDimensions) { stream << "*[A^"       << _A      << "]"; }
  if (_K != 0     || !ignoreZeroDimensions) { stream << "*[K^"       << _K  << "]"; }
  if (_mol != 0   || !ignoreZeroDimensions) { stream << "*[mol^"     << _mol       << "]"; }
  if (_cd != 0    || !ignoreZeroDimensions) { stream << "*[cd^"      << _cd    << "]"; }
  if (_offset != 0|| !ignoreZeroDimensions) { stream << "+[offset="  << _offset       << "]"; }
  return stream.str();
}


// class methods
double Units::convertValue(double value, const Units& fromUnits, const Units& toUnits) {
  if (fromUnits.isSameDimensionAs(toUnits)) {
    return ((value + fromUnits._offset) * fromUnits._conversion / toUnits._conversion) - toUnits._offset;
  }
  else {
    cerr << "Units are not dimensionally consistent" << endl;
    return 0.;
  }
}

std::map<std::string, Units> Units::unitStringMap = []()
{
  map<string, Units> m;
  
  m["dimensionless"]= RTX_DIMENSIONLESS;
  m["Hz"] = RTX_HERTZ;
  m["rpm"] = RTX_RPM;
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
  m["m³"]  = RTX_CUBIC_METER;
  m["gal"] = RTX_GALLON;
  m["mgal"]= RTX_MILLION_GALLON;
  m["liter"]=RTX_LITER;
  m["ft³"] = RTX_CUBIC_FOOT;
  // flow
  m["cms"]= RTX_CUBIC_METER_PER_SECOND;
  m["cfs"] = RTX_CUBIC_FOOT_PER_SECOND;
  m["gps"] = RTX_GALLON_PER_SECOND;
  m["gpm"] = RTX_GALLON_PER_MINUTE;
  m["gpd"] = RTX_GALLON_PER_DAY;
  m["mgd"] = RTX_MILLION_GALLON_PER_DAY;
  m["lps"] = RTX_LITER_PER_SECOND;
  m["lpm"] = RTX_LITER_PER_MINUTE;
  m["mld"] = RTX_MILLION_LITER_PER_DAY;
  m["m³/hr"]=RTX_CUBIC_METER_PER_HOUR;
  m["m³/d"]= RTX_CUBIC_METER_PER_DAY;
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
  m["mg/L"]= RTX_MILLIGRAMS_PER_LITER;
  // conductance
  m["us/cm"]=RTX_MICROSIEMENS_PER_CM;
  // velocity
  m["m/s"] = RTX_METER_PER_SECOND;
  m["fps"] = RTX_FOOT_PER_SECOND;
  m["ft/hr"] = RTX_FOOT_PER_HOUR;
  // acceleration
  m["m/s²"] = RTX_METER_PER_SECOND_SECOND;
  m["ft/s²"] = RTX_FOOT_PER_SECOND_SECOND;
  m["ft/hr²"] = RTX_FOOT_PER_HOUR_HOUR;

//  m["mgd/s"] = RTX_MILLION_GALLON_PER_DAY_PER_SECOND;
  
  // temperature
  m["kelvin"] = RTX_DEGREE_KELVIN;
  m["rankine"] = RTX_DEGREE_RANKINE;
  m["celsius"] = RTX_DEGREE_CELSIUS;
  m["farenheit"] = RTX_DEGREE_FARENHEIT;
  
  m["kwh"] = RTX_KILOWATT_HOUR;
  m["mj"] = RTX_MEGAJOULE;
  m["j"] = RTX_JOULE;
  
  // energy density
  m["kW-H/m³"] = RTX_ENERGY_DENSITY;
  m["kW-H/MG"] = RTX_ENERGY_DENSITY_MG;
  
  m["xx-no-units"] = RTX_NO_UNITS;
  m["%"] = RTX_PERCENT;
  
  m["ft-per-psi"] = RTX_FOOT * 2.30665873688 / RTX_PSI;
  m["psi-per-ft"] = RTX_PSI / (RTX_FOOT * 2.30665873688);
  m["W"] = RTX_WATT;
  m["kW"] = RTX_KILOWATT;
  
  m["V"] = RTX_VOLT;
  m["A"] = RTX_AMP;
  
  m["ft²"] = RTX_SQ_FOOT;
  m["m²"] = RTX_SQ_METER;
  m["cm²"] = RTX_SQ_CENTIMETER;
  m["in²"] = RTX_SQ_INCH;
  
  return m;
}();

// factory for string input
Units Units::unitOfType(const string& unitString) {
  
  if (RTX_STRINGS_ARE_EQUAL(unitString, "")) {
    return RTX_NO_UNITS;
  }
  
  double conversionFactor = 1;
  int mass=0, length=0, time=0, current=0, temperature=0, amount=0, intensity=0;
  double offset=0;
  
//  const map<string, Units> unitMap = Units::unitStringMap;
  map<string, Units>::const_iterator found = Units::unitStringMap.find(unitString);
  if (found != Units::unitStringMap.end()) {
    return found->second;
  }
  
  
  
  // superscript?
  {
    string uStr = unitString;
    size_t loc3 = unitString.find("3");
    if (loc3 != string::npos) {
      uStr.replace(loc3,1,"³");
    }
    size_t loc2 = unitString.find("2");
    if (loc2 != string::npos) {
      uStr.replace(loc2,1,"²");
    }
    
    // try again
    found = Units::unitStringMap.find(uStr);
    if (found != Units::unitStringMap.end()) {
      return found->second;
    }
  }
  
  
  // case insensitive search?
  typedef std::pair<std::string, Units> stringUnitsPair;
  for(const stringUnitsPair& sup : Units::unitStringMap) {
    const string u = sup.first;
    if (RTX_STRINGS_ARE_EQUAL(u, unitString)) {
      return sup.second;
    }
  }
  
  
  
  // attempt to deserialize the streamed format of the unit conversion and dimension.
  deque<string> components;
  boost::split(components, unitString, boost::is_any_of("*[]"), boost::algorithm::token_compress_on); // split on separators
  
  if (components.size() < 1) {
    cerr << "WARNING: Units not recognized: " << unitString << " - defaulting to NO UNITS." << endl;
    return RTX_NO_UNITS;
  }
  
  // first component will be the unit conversion, so cast that into a number.
  // this may fail, so catch it if so.
  try {
    conversionFactor = boost::lexical_cast<double>(components.front());
  } catch (...) {
    cerr << "WARNING: Units not recognized: " << unitString << " - defaulting to NO UNITS." << endl;
    return RTX_NO_UNITS;
  }
  
  components.pop_front();
  
  // get each dimensional power and set it.
  for(const string& part : components) {
    
    vector<string> dimensionPower;
    boost::split(dimensionPower, part, boost::is_any_of("^="));
    if (dimensionPower.size() != 2) {
      continue;
    }
    int power;
    double offsetFromStr;
    if (!boost::conversion::try_lexical_convert(dimensionPower.back(), power)) {
      offsetFromStr = boost::lexical_cast<double>(dimensionPower.back());
    }
    dimensionPower.pop_back();
    string dim = dimensionPower.back();
    
    // match the SI dimension name
    if (RTX_STRINGS_ARE_EQUAL(dim, "kilograms")      || RTX_STRINGS_ARE_EQUAL_CS(dim, "kg")) {
      mass = power;
    } else if (RTX_STRINGS_ARE_EQUAL(dim, "meters")  || RTX_STRINGS_ARE_EQUAL_CS(dim, "m")) {
      length = power;
    } else if (RTX_STRINGS_ARE_EQUAL(dim, "seconds") || RTX_STRINGS_ARE_EQUAL_CS(dim, "s")) {
      time = power;
    } else if (RTX_STRINGS_ARE_EQUAL(dim, "ampere")  || RTX_STRINGS_ARE_EQUAL_CS(dim, "A")) {
      current = power;
    } else if (RTX_STRINGS_ARE_EQUAL(dim, "kelvin")  || RTX_STRINGS_ARE_EQUAL_CS(dim, "K")) {
      temperature = power;
    } else if (RTX_STRINGS_ARE_EQUAL(dim, "mole")    || RTX_STRINGS_ARE_EQUAL_CS(dim, "mol")) {
      amount = power;
    } else if (RTX_STRINGS_ARE_EQUAL(dim, "candela") || RTX_STRINGS_ARE_EQUAL_CS(dim, "cd")) {
      intensity = power;
    } else if (RTX_STRINGS_ARE_EQUAL(dim, "offset")) {
      offset = offsetFromStr;
    }
  }
  
  return Units(conversionFactor, mass, length, time, current, temperature, amount, intensity, offset);
  
}


