#include <boost/foreach.hpp>
#include <cmath>
//#include "WarpingTimeSeries.h"

using namespace std;
using namespace RTX;

TimeSeries::_sp WarpingTimeSeries::warp() {
  return _warpingBasis;
}

void WarpingTimeSeries::setWarp(TimeSeries::_sp warp) {
  if (!warp) {
    _warpingBasis = TimeSeries::_sp();
    return;
  }
  if (warp->units().isSameDimensionAs(RTX_SECOND)) {
    _warpingBasis = warp;
  }
}



vector<Point> WarpingTimeSeries::filteredPoints(TimeSeries::_sp sourceTs, time_t fromTime, time_t toTime) {
  // sanity
  if (!_warpingBasis) {
    return ModularTimeSeries::filteredPoints(sourceTs, fromTime, toTime);
  }
  
  vector<Point> warpedPoints;
  
  // we have a basis for warping. let's get those time values.
  Units warpUnits = this->warp()->units();
  vector<Point> warpingPoints = _warpingBasis->points(fromTime, toTime);
  BOOST_FOREACH(const Point& p, warpingPoints) {
    time_t warpedTime = p.time - (time_t)lround( Units::convertValue(p.value, warpUnits, RTX_SECOND) );
    Point sourcePoint = this->source()->pointAtOrBefore(warpedTime);
    Point warpedPoint(p.time, sourcePoint.value, sourcePoint.quality, sourcePoint.confidence);
    warpedPoints.push_back(warpedPoint);
  }
  
  return warpedPoints;
  
}