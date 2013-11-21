#include "MultiplierTimeSeries.h"

using namespace RTX;
using namespace std;



MultiplierTimeSeries::MultiplierTimeSeries() {
  _multiplierBasis = TimeSeries::sharedPointer(new TimeSeries());
}

void MultiplierTimeSeries::setMultiplier(TimeSeries::sharedPointer ts) {
  _multiplierBasis = ts;
}
TimeSeries::sharedPointer MultiplierTimeSeries::multiplier() {
  return _multiplierBasis;
}

std::vector<Point> MultiplierTimeSeries::filteredPoints(TimeSeries::sharedPointer sourceTs, time_t fromTime, time_t toTime) {
  
  // call the multiplier basis method to cache points
  if (this->multiplier()) {
    vector<Point> multiplierPoints = this->multiplier()->points(fromTime, toTime);
  }
  
  return SinglePointFilterModularTimeSeries::filteredPoints(sourceTs, fromTime, toTime);
}

Point MultiplierTimeSeries::filteredSingle(Point p, Units sourceU) {
  
  Point newP = Point::convertPoint(p, sourceU, units());  
  Point multiplierPoint = (this->multiplier()) ? this->multiplier()->point(p.time) : Point(p.time,1.);
  newP *= multiplierPoint.value;

  return newP;
}
