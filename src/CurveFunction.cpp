//
//  CurveFunction.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#include <boost/foreach.hpp>

#include "CurveFunction.h"

using namespace RTX;

CurveFunction::CurveFunction() : _inputUnits(1) {
  
}

void CurveFunction::setSource(TimeSeries::sharedPointer source) {
  // hijack the base class implementation
  Units myRealUnits = units();
  this->setUnits(RTX_DIMENSIONLESS);  // non-dimensionalize so that we can accept this source.
  ModularTimeSeries::setSource(source);
  this->setUnits(myRealUnits);  // re-set the units.
}

void CurveFunction::setUnits(Units newUnits) {
  // just use the base-est class method for this, since we don't really care
  // if the new units are compatible with the source units.
  TimeSeries::setUnits(newUnits);
}

// the inputUnits are the units of the "input" values specifed by
// addCurveCoordinate() -- not the units of the (modular) source time series
// as one might expect.
void CurveFunction::setInputUnits(Units inputUnits) {
  // TODO -- check dimensional compatibility with source
  _inputUnits = inputUnits;
}

void CurveFunction::addCurveCoordinate(double inputValue, double outputValue) {
  std::pair<double,double> newCoord(inputValue,outputValue);
  _curve.push_back(newCoord);
}

Point CurveFunction::point(time_t time) {
  
  // get the appropriate point from the source.
  // unfortunately, this is mostly copied from ModularTimeSeries:: -- but we have to modify it for the unit checking
  Point sourcePoint;
  if (clock()->isRegular()) {
    time = clock()->validTime(time);
  }
  Point p = TimeSeries::point(time);
  if (p.isValid) {
    return p;
  }
  else {
    p = source()->point(time);
    if (p.isValid) {
      // create a new point object converted from source units
      sourcePoint = Point::convertPoint(p, source()->units(), _inputUnits);
    }
    else {
      std::cerr << "check point availability first\n";
      // TODO -- throw something?
    }
  }
  
  // get the value from this point
  double sourceValue = sourcePoint.value;
  double sourceConf = sourcePoint.confidence;
  
  // get the interpolated point from the function curve
  double  x1 = _curve.at(0).first,
          y1 = _curve.at(0).second,
          x2 = _curve.at(0).first,
          y2 = _curve.at(0).second;
  
  typedef std::pair<double,double> doublePairType;
  BOOST_FOREACH(doublePairType dpair, _curve) {
    x2 = dpair.first;
    y2 = dpair.second;
    
    if (x2 > sourceValue) {
      // great, we have what we need.
      break;
    }
    else {
      x1 = x2;
      y1 = y2;
    }
  }
  // TODO -- robustify this - edge conditions?
  double newValue = y1 + ( (sourceValue - x1) * (y2 - y1) / (x2 - x1) );
  
  Point newPoint(time, newValue, Point::good, sourceConf);
  
  this->insert(newPoint);
  return newPoint;
}

