//
//  IntegratorTimeSeries.cpp
//  epanet-rtx
//
//  Created by Sam Hatchett on 2/13/15.
//
//

#include "IntegratorTimeSeries.h"


using namespace RTX;
using namespace std;


void IntegratorTimeSeries::setResetClock(Clock::_sp resetClock) {
  _reset = resetClock;
  this->invalidate();
}

Clock::_sp IntegratorTimeSeries::resetClock() {
  return _reset;
}

TimeSeries::PointCollection IntegratorTimeSeries::filterPointsInRange(TimeRange range) {

  vector<Point> outPoints;
  Units fromUnits = this->source()->units();
  PointCollection data(vector<Point>(), fromUnits * RTX_SECOND);
  
  if (!this->resetClock()) {
    return PointCollection(vector<Point>(), this->units());
  }
  
  // back up to previous reset clock tick
  time_t lastReset = this->resetClock()->timeBefore(range.start + 1);
  
  // lagging area, so get this time or the one before it.
  time_t leftMostTime = this->source()->timeBefore(lastReset + 1);
  // to-do :: should we resample the source here, for special cases?
  
  // get next point in case it's out of the specified range
  time_t seekRightTime = range.end - 1;
  seekRightTime = this->source()->timeAfter(seekRightTime);
  if (seekRightTime > 0) {
    range.end = seekRightTime;
  }
  
  PointCollection sourceData = this->source()->pointCollection(TimeRange(leftMostTime, range.end));
  
  if (sourceData.count() < 2) {
    return data;
  }
  
  vector<Point>::const_iterator cursor, prev, vEnd;
  cursor = sourceData.points.begin();
  prev = sourceData.points.begin();
  vEnd = sourceData.points.end();
  
  time_t nextReset = lastReset; //this->resetClock()->timeAfter(range.start);
  double integratedValue = 0;
  
  ++cursor;
  while (cursor != vEnd) {
    
    time_t dt = cursor->time - prev->time;
    double meanValue = (cursor->value + prev->value) / 2.;
    double area = meanValue * double(dt);
    integratedValue += area;
    
    // should we reset?
    if (cursor->time >= nextReset) {
      integratedValue = 0;
      nextReset = this->resetClock()->timeAfter(cursor->time);
    }
    
    if (range.contains(cursor->time)) {
      Point p(cursor->time, integratedValue);
      p.addQualFlag(Point::rtx_integrated);
      outPoints.push_back(p);
    }
    ++cursor;
    ++prev;
  }
  
  
  data.points = outPoints;
  data.convertToUnits(this->units());
  
  if (this->willResample()) {
    set<time_t> timeValues = this->timeValuesInRange(range);
    data.resample(timeValues);
  }
  
  return data;
}


bool IntegratorTimeSeries::canSetSource(TimeSeries::_sp ts) {
  return (!this->source() || this->units().isSameDimensionAs(ts->units() * RTX_SECOND));
}

void IntegratorTimeSeries::didSetSource(TimeSeries::_sp ts) {
  if (this->units().isDimensionless() || !this->units().isSameDimensionAs(ts->units() * RTX_SECOND)) {
    Units newUnits = ts->units() * RTX_SECOND;
    if (newUnits.isDimensionless()) {
      newUnits = RTX_DIMENSIONLESS;
    }
    this->setUnits(newUnits);
  }
}

bool IntegratorTimeSeries::canChangeToUnits(RTX::Units units) {
  if (!this->source()) {
    return true;
  }
  else if (units.isSameDimensionAs(this->source()->units() * RTX_SECOND)) {
    return true;
  }
  else {
    return false;
  }
}



