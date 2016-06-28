#include "ValidRangeTimeSeries.h"

#include <boost/foreach.hpp>

using namespace std;
using namespace RTX;

ValidRangeTimeSeries::ValidRangeTimeSeries() {
  _mode = drop;
  _range = make_pair(0, 1);
}

pair<double,double> ValidRangeTimeSeries::range() {
  return _range;
}
void ValidRangeTimeSeries::setRange(double min, double max) {
  _range = make_pair(min, max);
  this->invalidate();
}

ValidRangeTimeSeries::filterMode_t ValidRangeTimeSeries::mode() {
  return _mode;
}
void ValidRangeTimeSeries::setMode(filterMode_t mode) {
  _mode = mode;
  this->invalidate();
}

bool ValidRangeTimeSeries::willResample() {
  return TimeSeriesFilter::willResample() || (this->clock() && _mode == filterMode_t::drop);
}

bool ValidRangeTimeSeries::canDropPoints() {
  return this->mode() == ValidRangeTimeSeries::drop;
}

TimeSeries::PointCollection ValidRangeTimeSeries::filterPointsInRange(TimeRange range) {
  
  // if we are to resample, then there's a possibility that we need to expand the range
  // used to query the source ts. but we have to limit the search to something reasonable, in case
  // too many points are excluded. yikes!
  TimeRange sourceQuery = range;
  if (this->willResample()) {
    // expand range so that we can resample at the start and/or end of the range requested
    // this is quick and dirty, and probably will fail for badly behaved data.
    sourceQuery.start -= range.duration();
    sourceQuery.end += range.duration();
  }
  
  // get raw values, exclude outliers, then resample if needed.
  PointCollection raw = this->source()->pointCollection(sourceQuery);
  
  
  raw.convertToUnits(this->units());
  PointCollection outData(vector<Point>(),this->units());
  
  BOOST_FOREACH(const Point& p, raw.points) {
    Point newP;
    double pointValue = p.value;
    
    if (pointValue < _range.first || _range.second < pointValue) {
      // out of range.
      if (_mode == saturate) {
        // bring pointValue to the nearest max/min value
        pointValue = RTX_MAX(pointValue, _range.first);
        pointValue = RTX_MIN(pointValue, _range.second);
        newP = Point(p.time, pointValue, p.quality, p.confidence);
      }
      else {
        // drop point
        newP.time = p.time;
      }
    }
    else {
      // just pass it through
      newP = p;
    }
    if (newP.isValid) {
      outData.points.push_back(newP);
    }
  }
  
  if (this->willResample()) {
    set<time_t> resTimes = this->timeValuesInRange(range);
    outData.resample(resTimes);
  }
  
  return outData;
}


