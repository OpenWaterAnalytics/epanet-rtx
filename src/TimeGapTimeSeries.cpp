#include <boost/foreach.hpp>

#include "TimeGapTimeSeries.h"

using namespace std;
using namespace RTX;

TimeGapTimeSeries::TimeGapTimeSeries() {
  this->setUnits(RTX_SECOND); // Default time unit
}


TimeSeries::PointCollection TimeGapTimeSeries::filterPointsInRange(TimeRange range) {
  
  PointCollection gaps(vector<Point>(), RTX_SECOND);
  
  TimeRange qRange = range;
  if (this->willResample()) {
    // expand range
    qRange.start = this->source()->timeBefore(range.start + 1);
    qRange.end = this->source()->timeAfter(range.end - 1);
  }
  
  // one prior
  qRange.start = this->source()->timeBefore(qRange.start);
  
  qRange.correctWithRange(range);
  PointCollection sourceData = this->source()->pointCollection(qRange);
  
  if (sourceData.count() < 2) {
    return PointCollection(vector<Point>(), this->units());
  }
  
  vector<Point>::const_iterator it = sourceData.points.cbegin();
  vector<Point>::const_iterator prev = it;
  ++it;
  while (it != sourceData.points.cend()) {
    
    time_t t1,t2;
    t1 = prev->time;
    t2 = it->time;
    
    double gapV = (double)(t2-t1);
    Point gapP(t2,gapV);
    gapP.addQualFlag(Point::rtx_integrated);
    gaps.points.push_back(gapP);
    
    ++prev;
    ++it;
  }
  
  gaps.convertToUnits(this->units());
  
  if (this->willResample()) {
    set<time_t> times = this->timeValuesInRange(range);
    gaps.resample(times);
  }
  
  return gaps;
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

