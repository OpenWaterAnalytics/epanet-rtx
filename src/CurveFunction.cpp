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
using namespace std;

CurveFunction::CurveFunction() : _inputUnits(1) {
  
}

// the inputUnits are the units of the "input" values specifed by
// addCurveCoordinate() -- not the units of the (modular) source time series
// as one might expect.
void CurveFunction::setInputUnits(Units inputUnits) {
  // TODO -- check dimensional compatibility with source
  _inputUnits = inputUnits;
  this->invalidate();
}
Units CurveFunction::inputUnits() {
  return _inputUnits;
}

void CurveFunction::addCurveCoordinate(double inputValue, double outputValue) {
  std::pair<double,double> newCoord(inputValue,outputValue);
  _curve.push_back(newCoord);
}
void CurveFunction::setCurve( vector<pair<double,double> > curve) {
  _curve = curve;
  this->invalidate();
}
void CurveFunction::clearCurve() {
  _curve.clear();
}
std::vector<std::pair<double,double> > CurveFunction::curve() {
  return _curve;
}





Point CurveFunction::filteredWithSourcePoint(RTX::Point sourcePoint) {
  
  // convert input units and get the value
  Point convertedSourcePoint = Point::convertPoint(sourcePoint, this->source()->units(), this->inputUnits());
  double convertedSourceValue = convertedSourcePoint.value;
  
  if (_curve.size() < 1) {
    return Point(sourcePoint.time,0,Point::opc_bad);
  }
  
  double newValue;
  double minimumX = _curve.front().first;
  double maximumX = _curve.back().first;
  double minimumY = _curve.front().second;
  double maximumY = _curve.back().second;
  
  if (convertedSourceValue > minimumX && convertedSourceValue < maximumX) {
    // get the interpolated point from the function curve
    double  x1 = _curve.at(0).first,
    y1 = _curve.at(0).second,
    x2 = _curve.at(0).first,
    y2 = _curve.at(0).second;
    typedef std::pair<double,double> doublePairType;
    BOOST_FOREACH(doublePairType dpair, _curve) {
      x2 = dpair.first;
      y2 = dpair.second;
      
      if (x2 > convertedSourceValue) {
        // great, we have what we need.
        break;
      }
      else {
        x1 = x2;
        y1 = y2;
      }
    }
    newValue = y1 + ( (convertedSourceValue - x1) * (y2 - y1) / (x2 - x1) );
  }
  else if (convertedSourceValue <= minimumX) {
    // outside left edge of x range -- return minimum y
    //std::cerr << "input x value out of function range\n";
    newValue = minimumY;
  }
  else {
    // outside right edge of x range - return maximum y
    //std::cerr << "input x value out of function range\n";
    newValue = maximumY;
  }
  
  Point newPoint(convertedSourcePoint.time, newValue, convertedSourcePoint.quality, convertedSourcePoint.confidence);
  return newPoint;
  
}



bool CurveFunction::canSetSource(TimeSeries::_sp ts) {
  return true;
}

void CurveFunction::didSetSource(TimeSeries::_sp ts) {
  this->setInputUnits(ts->units());
}

bool CurveFunction::canChangeToUnits(Units units) {
  return true;
}


