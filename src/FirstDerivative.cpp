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


TimeSeries::PointCollection FirstDerivative::filterPointsAtTimes(std::set<time_t> times) {
  
  vector<Point> outPoints;
  Units fromUnits = this->source()->units();
  
  // left difference, so get one more to the left.
  Point prior = this->source()->pointBefore(*times.begin());
  set<time_t> getSourceTimes = times;
  getSourceTimes.insert(prior.time);
  
  vector<Point> sourceData = this->source()->resampled(getSourceTimes);
  
  if (sourceData.size() < 2) {
    return outCollection;
  }
  
  
  vector<Point>::const_iterator cursor, prev, vEnd;
  cursor = sourceData.begin();
  prev = sourceData.begin();
  vEnd = sourceData.end();
  
  ++cursor;
  
  while (cursor != vEnd) {
    time_t dt = cursor->time - prev->time;
    double dv = cursor->value - prev->value;
    double dvdt = Units::convertValue(dv / double(dt), fromUnits / RTX_SECOND, this->units());
    
    outPoints.push_back(Point(t, dvdt));
    
    ++cursor;
    ++prev;
  }
  
  return PointCollection(outPoints, this->units());
}

bool FirstDerivative::canSetSource(TimeSeries::_sp ts) {
  return (!this->source() || this->units().isSameDimensionAs(ts->units() / RTX_SECOND));
}

void FirstDerivative::didSetSource(TimeSeries::_sp ts) {
  if (this->units().isDimensionless() || !this->units().isSameDimensionAs(ts->units() / RTX_SECOND)) {
    this->setUnits(ts->units() / RTX_SECOND);
  }
}

bool FirstDerivative::canChangeToUnits(Units units) {
  if (!this->source()) {
    return true;
  }
  else if (units.isSameDimensionAs(this->source() / RTX_SECOND)) {
    return true;
  }
  else {
    return false;
  }
}



std::ostream& FirstDerivative::toStream(std::ostream &stream) {
  TimeSeries::toStream(stream);
  if (source()) {
    stream << "First Derivative Of: " << *source() << "\n";
  }
  
  return stream;
}
