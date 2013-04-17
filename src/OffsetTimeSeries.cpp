//
//  OffsetTimeSeries.cpp
//  epanet-rtx
//
//  Created by Sam Hatchett on 10/18/12.
//
//

#include <boost/foreach.hpp>

#include "OffsetTimeSeries.h"

using namespace std;
using namespace RTX;

OffsetTimeSeries::OffsetTimeSeries() {
  setOffset(0.);
}

Point OffsetTimeSeries::point(time_t time){
  
  Point sourcePoint = source()->point(time);
  Point newPoint = this->convertWithOffset(sourcePoint, source()->units());
  return newPoint;
  
}

std::vector<Point> OffsetTimeSeries::filteredPoints(time_t fromTime, time_t toTime, const std::vector<Point>& sourcePoints) {
  
  Units sourceU = source()->units();
  
  vector<Point> offsetPoints;
  offsetPoints.reserve(sourcePoints.size());
  
  BOOST_FOREACH(const Point& p, sourcePoints) {
    if (p.time < fromTime || p.time > toTime) {
      continue;
    }
    Point newP = this->convertWithOffset(p, sourceU);
    offsetPoints.push_back(newP);
  }
  
  return offsetPoints;
}

void OffsetTimeSeries::setOffset(double offset) {
  _offset = offset;
}

double OffsetTimeSeries::offset() {
  return _offset;
}

Point OffsetTimeSeries::convertWithOffset(Point p, Units sourceU) {
  Point convertedSourcePoint = Point::convertPoint(p, sourceU, units());
  double pointValue = convertedSourcePoint.value;
  pointValue += offset();
  Point newPoint(convertedSourcePoint.time, pointValue, convertedSourcePoint.quality, convertedSourcePoint.confidence);
  return newPoint;
}