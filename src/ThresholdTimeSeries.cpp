#include <boost/foreach.hpp>

#include "ThresholdTimeSeries.h"

using namespace std;
using namespace RTX;

ThresholdTimeSeries::ThresholdTimeSeries() {
  _threshold = 0;
  _fixedValue = 1;
}

Point ThresholdTimeSeries::point(time_t time){
  
  Point sourcePoint = source()->point(time);
  Point newPoint = this->convertWithThreshold(sourcePoint);
  return newPoint;
  
}

std::vector<Point> ThresholdTimeSeries::filteredPoints(time_t fromTime, time_t toTime, const std::vector<Point>& sourcePoints) {
  
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

void ThresholdTimeSeries::setThreshold(double threshold) {
  _threshold = threshold;
}

double ThresholdTimeSeries::threshold() {
  return _threshold;
}

void ThresholdTimeSeries::setValue(double val) {
  _fixedValue = val;
}

double ThresholdTimeSeries::value() {
  return _fixedValue;
}

Point ThresholdTimeSeries::convertWithThreshold(Point p) {
  double pointValue = (p.value > _threshold) ? _fixedValue : 0.;
  Point newPoint(p.time, pointValue, p.quality, p.confidence);
  return newPoint;
}

