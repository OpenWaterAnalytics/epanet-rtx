#include <boost/foreach.hpp>

#include "TimeGapTimeSeries.h"

using namespace std;
using namespace RTX;

TimeGapTimeSeries::TimeGapTimeSeries() {
  this->setUnits(RTX_SECOND); // Default time unit
}

void TimeGapTimeSeries::setSource(TimeSeries::sharedPointer source) {
  Units originalUnits = this->units();
  this->setUnits(RTX_DIMENSIONLESS);  // non-dimensionalize so that we can accept this source.
  ModularTimeSeries::setSource(source);
  
  // Time Gap units are independent of the source units - reset them
  this->setUnits(originalUnits);
}

void TimeGapTimeSeries::setUnits(Units newUnits) {
  
  // TimeGap requires time units
  if (newUnits.isSameDimensionAs(RTX_SECOND)) {
    TimeSeries::setUnits(newUnits);
  }
  else {
    std::cerr << "Time Gap requires time units" << std::endl;
  }
}

vector<Point> TimeGapTimeSeries::filteredPoints(TimeSeries::sharedPointer sourceTs, time_t fromTime, time_t toTime) {
  vector<Point> filtered;
  
  std::vector<Point> sourcePoints = sourceTs->gaps(fromTime, toTime);
  
  if (sourcePoints.size() == 0) {
    return filtered;
  }
  
  BOOST_FOREACH(const Point& p, sourcePoints) {
    if (p.time < fromTime || p.time > toTime) {
      continue; // skip the points we didn't ask for.
    }
    Point aPoint = Point::convertPoint(p, RTX_SECOND, this->units());
    filtered.push_back(aPoint);
  }
  
  return filtered;
}
