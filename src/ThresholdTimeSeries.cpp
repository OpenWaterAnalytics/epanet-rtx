//
//  StatusTimeSeries.cpp
//  epanet-rtx
//
//  Created by Jim Uber on 4/9/2013 based on OffsetTimeSeries Class.
//
//

#include <boost/foreach.hpp>

#include "StatusTimeSeries.h"

using namespace std;
using namespace RTX;

StatusTimeSeries::StatusTimeSeries() {
  setThreshold(0.);
}

Point StatusTimeSeries::point(time_t time){
  
  Point sourcePoint = source()->point(time);
  Point newPoint = this->convertWithThreshold(sourcePoint);
  return newPoint;
  
}

std::vector<Point> StatusTimeSeries::filteredPoints(time_t fromTime, time_t toTime, const std::vector<Point>& sourcePoints) {
  
  vector<Point> statusPoints;
  statusPoints.reserve(sourcePoints.size());
  
  BOOST_FOREACH(const Point& p, sourcePoints) {
    if (p.time < fromTime || p.time > toTime) {
      continue;
    }
    Point newP = this->convertWithThreshold(p);
    statusPoints.push_back(newP);
  }
  
  return statusPoints;
}

void StatusTimeSeries::setThreshold(double threshold) {
  _threshold = threshold;
}

double StatusTimeSeries::threshold() {
  return _threshold;
}

Point StatusTimeSeries::convertWithThreshold(Point p) {
  double pointValue = 0.;
  if(p.value > _threshold) pointValue = 1.;
  Point newPoint(p.time, pointValue, p.quality, p.confidence);
  return newPoint;
}

void StatusTimeSeries::setUnits(Units newUnits) {
  cerr << "can not set units for status time series " << name() << endl;
}
