#include "TimeOffsetTimeSeries.h"


using namespace std;
using namespace RTX;

TimeOffsetTimeSeries::TimeOffsetTimeSeries() {
  setOffset(0);
}

void TimeOffsetTimeSeries::setOffset(time_t offset) {
  _offset = offset;
}

time_t TimeOffsetTimeSeries::offset() {
  return _offset;
}



//Point TimeOffsetTimeSeries::point(time_t time) {
//  return TIME_OFFSET_SUPER::point(time - _offset);
//}


/*
 
 for the pointBefore and pointAfter methods, we have to skip cache checking to work around a cache invalidation bug. thus the seemingly repeated code here.
 */

Point TimeOffsetTimeSeries::pointBefore(time_t time) {
  Point sourcePoint = source()->pointBefore(time - _offset);
  Point p = this->filteredSingle(sourcePoint, source()->units());
  if (time < p.time) {
    cerr << "point not actually before" << endl;
  }
  return p;
}

Point TimeOffsetTimeSeries::pointAfter(time_t time) {
  Point sourcePoint = source()->pointAfter(time - _offset);
  Point p = this->filteredSingle(sourcePoint, source()->units());
  if (p.time < time) {
    cerr << "point not actually after" << endl;
  }
  return p;
}

vector<Point> TimeOffsetTimeSeries::filteredPoints(TimeSeries::sharedPointer sourceTs, time_t fromTime, time_t toTime) {
  return TIME_OFFSET_SUPER::filteredPoints(sourceTs, fromTime - _offset, toTime - _offset);
}

Point TimeOffsetTimeSeries::filteredSingle(Point p, Units sourceU) {
  Point convertedSourcePoint = Point::convertPoint(p, sourceU, units());
  time_t pointTime = convertedSourcePoint.time;
  pointTime += offset();
  Point newPoint(pointTime, convertedSourcePoint.value, convertedSourcePoint.quality, convertedSourcePoint.confidence);
  return newPoint;
}