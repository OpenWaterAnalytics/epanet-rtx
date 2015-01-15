//
//  TimeSeriesFilter.cpp
//  epanet-rtx
//

#include "TimeSeriesFilter.h"
#include <boost/foreach.hpp>

using namespace RTX;
using namespace std;

Clock::_sp TimeSeriesFilter::clock() {
  return _clock;
}
void TimeSeriesFilter::setClock(Clock::_sp clock) {
  if (clock) {
    _clock = clock;
  }
  else {
    _clock.reset();
  }
  this->invalidate();
}

TimeSeries::_sp TimeSeriesFilter::source() {
  return _source;
}
void TimeSeriesFilter::setSource(TimeSeries::_sp ts) {
  this->invalidate();
  _source = ts;
  TimeSeriesFilter::_sp filterSource = boost::dynamic_pointer_cast<TimeSeriesFilter>(ts);
  if (filterSource && !this->clock()) {
    this->setClock(filterSource->clock());
  }
}


vector<Point> TimeSeriesFilter::points(time_t start, time_t end) {
  vector<Point> filtered;
  
  if (! this->source()) {
    return filtered;
  }
  
  TimeRange range = make_pair(start, end);
  range = this->willGetRangeFromSource(this->source(), range);
  PointCollection native;
  
  if (this->clock()->isRegular()) {
    set<time_t> timeList = this->clock()->timeValuesInRange(range.first, range.second);
    
    
    // important optimization. if this range has already been constructed and cached, then don't recreate it.
    bool alreadyCached = false;
    vector<Point> cached = TimeSeries::points(range.first, range.second); // base class call -> find any pre-cached points
    if (cached.size() == timeList.size()) {
      // looks good, let's make sure that all time values line up.
      alreadyCached = true;
      BOOST_FOREACH(const Point& p, cached) {
        if (timeList.count(p.time) == 0) {
          alreadyCached = false;
          break; // break foreach, with false "alreadyCached" flag
        }
      }
    }
    
    if (alreadyCached) {
      return cached; // we're done here.
    }
    
    native = source()->resampled(timeList);
  }
  else {
    native = source()->pointCollection(range.first, range.second);
  }
  
  PointCollection outCollection = this->filterPointsInRange(native, make_pair(start, end));
  
  
  this->insertPoints(outCollection.points);
  
  return outCollection.points;
}



#pragma mark - Pseudo-Delegate methods


TimeSeries::TimeRange TimeSeriesFilter::willGetRangeFromSource(TimeSeries::_sp source, TimeRange range) {
  
  TimeRange outRange = range;
  
  if (this->clock()) {
    outRange.first = this->clock()->timeAfter(range.first - 1);
    outRange.second = this->clock()->timeBefore(range.second + 1);
  }
  
  return outRange;
}

TimeSeries::PointCollection TimeSeriesFilter::filterPointsInRange(RTX::TimeSeries::PointCollection inputCollection, TimeRange outRange) {
  
  vector<Point> outPoints;
  
  BOOST_FOREACH(const Point& p, inputCollection.points) {
    Point out = Point::convertPoint(p, inputCollection.units, this->units());
    outPoints.push_back(out);
  }
  
  PointCollection outCollection(outPoints,this->units());
  return outCollection;
}

bool TimeSeriesFilter::canChangeToUnits(Units units) {
  
  // basic pass-thru filter just alters units, not dimension.
  
  bool canChange = true;
  
  if (this->source()) {
    canChange = this->source()->units().isSameDimensionAs(units);
  }
  
  return canChange;
}





