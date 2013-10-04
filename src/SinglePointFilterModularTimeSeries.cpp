#include "SinglePointFilterModularTimeSeries.h"
#include <boost/foreach.hpp>

using namespace RTX;
using namespace std;


Point SinglePointFilterModularTimeSeries::point(time_t time){
  if (!source()) {
    return Point();
  }
  Point newPoint = TimeSeries::point(time);
  if (newPoint.isValid) {
    return newPoint;
  }
  Units sourceU = source()->units();
  Point sourcePoint = source()->point(time);
  
  if (sourcePoint.isValid) {
    newPoint = this->filteredSingle(sourcePoint, sourceU);
    if (newPoint.isValid) { // filtering can invalidate
      this->insert(newPoint);
    }
  }
  else {
    //std::cerr << "SinglePointFilter: \"" << this->name() << "\": check point availability first\n";
  }
  
  return newPoint;
}

std::vector<Point> SinglePointFilterModularTimeSeries::filteredPoints(TimeSeries::sharedPointer sourceTs, time_t fromTime, time_t toTime) {
  
  Units sourceU = source()->units();
  std::vector<Point> sourcePoints = sourceTs->points(fromTime, toTime);
  
  vector<Point> filteredPoints;
  filteredPoints.reserve(sourcePoints.size());
  
  BOOST_FOREACH(const Point& p, sourcePoints) {
    if (p.time < fromTime || p.time > toTime) {
      continue;
    }
    Point newP = this->filteredSingle(p, sourceU);
    if (newP.isValid) {
      filteredPoints.push_back(newP);
    }
  } // foreach source point
  
  return filteredPoints;
}

Point SinglePointFilterModularTimeSeries::filteredSingle(Point p, Units sourceU) {
  Point convertedPoint = Point::convertPoint(p, sourceU, units());
  return convertedPoint;
}
