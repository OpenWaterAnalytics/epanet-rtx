#include "MetaTimeSeries.h"
#include <boost/foreach.hpp>

using namespace std;
using namespace RTX;

MetaTimeSeries::MetaTimeSeries() {
  this->setUnits(RTX_SECOND); // Default time unit
  _metaMode = MetaModeGap;
}


TimeSeries::PointCollection MetaTimeSeries::filterPointsInRange(TimeRange range) {
  
  PointCollection gaps(vector<Point>(), RTX_DIMENSIONLESS);
  
  if (_metaMode == MetaModeGap) {
    gaps.units = RTX_SECOND;
  }
  
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
    Point metaPoint(t2);
    
    switch (_metaMode) {
      case MetaModeGap:
        metaPoint.value = (double)(t2-t1);
        break;
      case MetaModeConfidence:
        metaPoint.value = it->confidence;
        break;
      case MetaModeQuality:
        metaPoint.value = (double)(it->quality);
        break;
      default:
        break;
    }
    
    metaPoint.addQualFlag(Point::rtx_integrated);
    gaps.points.push_back(metaPoint);
    
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

bool MetaTimeSeries::canSetSource(TimeSeries::_sp ts) {
  return true;
}

void MetaTimeSeries::didSetSource(TimeSeries::_sp ts) {
  this->invalidate();
}

bool MetaTimeSeries::canChangeToUnits(Units units) {
  if (_metaMode == MetaModeGap) {
    return (units.isSameDimensionAs(RTX_SECOND));
  }
  else if (units.isDimensionless()) {
    return true;
  }
  return false;
}

void MetaTimeSeries::setMetaMode(RTX::MetaTimeSeries::MetaMode mode) {
  if (mode == _metaMode) {
    return;
  }
  
  _metaMode = mode;
  this->invalidate();
  
  if (_metaMode == MetaModeGap) {
    this->setUnits(RTX_SECOND);
  }
}

MetaTimeSeries::MetaMode MetaTimeSeries::metaMode() {
  return _metaMode;
}
