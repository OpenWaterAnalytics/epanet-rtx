//
//  FirstDerivative.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#include "FirstDerivative.h"

using namespace std;
using namespace RTX;

FirstDerivative::FirstDerivative() {
  
}

FirstDerivative::~FirstDerivative() {
  
}

void FirstDerivative::setSource(TimeSeries::sharedPointer source) {
  this->setUnits(RTX_DIMENSIONLESS);  // non-dimensionalize so that we can accept this source.
  Resampler::setSource(source);
  
  // get the rate of change units
  Units rate = source->units() / RTX_SECOND;
  
  this->setUnits(rate);  // re-set the units.
}

void FirstDerivative::setUnits(Units newUnits) {
  
  // only set the units if there is no source or the source's rate is dimensionally consistent with the passed-in units.
  if (!source() || (source()->units() / RTX_SECOND).isSameDimensionAs(newUnits) ) {
    // just use the base-est class method for this, since we don't really care
    // if the new units are the same as the source units.
    TimeSeries::setUnits(newUnits);
  }
  else if (!units().isDimensionless()) {
    std::cerr << "units are not dimensionally consistent" << std::endl;
  }
}

Point FirstDerivative::point(time_t time) {
  Point p;

  p = TimeSeries::point(time);
  if (p.isValid) {
    return p;
  }
  else {
    std::pair<Point,Point> adjacent = source()->adjacentPoints(time);
    Point secondPoint = adjacent.second;
    Point firstPoint = adjacent.first;
    if (!firstPoint.isValid) {
      firstPoint = Point();
    }
    
    pointBuffer_t buf;
    buf.set_capacity(2);
    buf.push_back(firstPoint);
    buf.push_back(secondPoint);
    
    p = filteredSingle(buf, time, source()->units());
    insert(p);
  }
  return p;
}


Point FirstDerivative::filteredSingle(const pointBuffer_t& window, time_t t, Units fromUnits) {
  if (window.size() < 2) {
    return Point();
  }
  pointBuffer_t::const_iterator it = window.begin();
  Point p1, p2;
  p1 = *it;
  p2 = *(++it);
  
  if (!(p1.isValid && p2.isValid)) {
    return Point();
  }
  time_t dt = p2.time - p1.time;
  double dv = p2.value - p1.value;
  double dvdt = Units::convertValue(dv / double(dt), fromUnits / RTX_SECOND, this->units());
  return Point(t, dvdt);
}


std::ostream& FirstDerivative::toStream(std::ostream &stream) {
  TimeSeries::toStream(stream);
  stream << "First Derivative Of: " << *source() << "\n";
  return stream;
}
