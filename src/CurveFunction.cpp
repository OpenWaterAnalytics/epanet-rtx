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

void CurveFunction::setSource(TimeSeries::sharedPointer source) {
  // hijack the base class implementation
  Units myRealUnits = units();
  this->setUnits(RTX_DIMENSIONLESS);  // non-dimensionalize so that we can accept this source.
  ModularTimeSeries::setSource(source);
  this->setUnits(myRealUnits);  // re-set the units.
  // by default, input units set to source units
  if (source) {
    this->setInputUnits(source->units());
  }
  
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

Units CurveFunction::inputUnits() {
  return _inputUnits;
}

void CurveFunction::addCurveCoordinate(double inputValue, double outputValue) {
  std::pair<double,double> newCoord(inputValue,outputValue);
  _curve.push_back(newCoord);
}

void CurveFunction::setCurve( vector<pair<double,double> > curve) {
  _curve = curve;
}

void CurveFunction::clearCurve() {
  _curve.clear();
}

std::vector<std::pair<double,double> > CurveFunction::curve() {
  return _curve;
}

Point CurveFunction::point(time_t time) {
  Units sourceU = source()->units();
  Point sourcePoint = source()->point(time);
  
  if (sourcePoint.isValid) {
    Point newPoint = this->convertWithCurve(sourcePoint, sourceU);
    return newPoint;
  } else {
    //std::cerr << "CurveFunction \"" << this->name() << "\": check point availability first\n";
    // TODO -- throw something?
    Point newPoint(time, 0.0, Point::missing, 0.0);
    return newPoint;
  }
}

std::vector<Point> CurveFunction::filteredPoints(TimeSeries::sharedPointer sourceTs, time_t fromTime, time_t toTime) {
  
  Units sourceU = sourceTs->units();
  std::vector<Point> sourcePoints = sourceTs->points(fromTime, toTime);
  
  std::vector<Point> curvePoints;
  curvePoints.reserve(sourcePoints.size());
  
  BOOST_FOREACH(const Point& p, sourcePoints) {
    if (p.time < fromTime || p.time > toTime) {
      continue;
    }
    Point newP = this->convertWithCurve(p, sourceU);
    curvePoints.push_back(newP);
  }
  
  return curvePoints;
}

Point CurveFunction::convertWithCurve(Point p, Units sourceU) {
  // convert input units and get the value
  Point convertedSourcePoint = Point::convertPoint(p, sourceU, _inputUnits);
  double convertedSourceValue = convertedSourcePoint.value;
  
  if (_curve.size() < 1) {
    return Point(p.time,0,Point::missing);
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
