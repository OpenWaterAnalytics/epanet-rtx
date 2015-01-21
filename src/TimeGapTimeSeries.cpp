#include <boost/foreach.hpp>

#include "TimeGapTimeSeries.h"

using namespace std;
using namespace RTX;

TimeGapTimeSeries::TimeGapTimeSeries() {
  this->setUnits(RTX_SECOND); // Default time unit
}


TimeSeries::PointCollection TimeGapTimeSeries::filterPointsAtTimes(std::set<time_t> times) {
  
  vector<Point> gaps;
  
  BOOST_FOREACH(time_t now, times) {
    Point thisOne = this->source()->pointAtOrBefore(now);
    Point lastOne = this->source()->pointBefore(thisOne.time);
    
    if (!thisOne.isValid || !lastOne.isValid) {
      continue; // skip this one.
    }
    time_t gapLen = thisOne.time - lastOne.time;
    double gapLenConverted = Units::convertValue((double)gapLen, RTX_SECOND, this->units());
    
    Point gapPoint(now, gapLenConverted);
    gaps.push_back(gapPoint);
  }
  
  
  return PointCollection(gaps, this->units());
}

bool TimeGapTimeSeries::canSetSource(TimeSeries::_sp ts) {
  return true;
}

void TimeGapTimeSeries::didSetSource(TimeSeries::_sp ts) {
  this->invalidate();
}

bool TimeGapTimeSeries::canChangeToUnits(Units units) {
  if (units.isSameDimensionAs(RTX_SECOND)) {
    return true;
  }
  return false;
}

