#include "RunTimeStatusModularTimeSeries.h"
#include <boost/foreach.hpp>
#include <math.h>

using namespace std;
using namespace RTX;

RunTimeStatusModularTimeSeries::RunTimeStatusModularTimeSeries() {
  _threshold = 0.0;
  _resetTolerance = 0.0;
  _resetCeiling = 0.0;
  _resetFloor = 0.0;
  _statusPointShelfLife = 12*60*60*24*7;
  _windowSize = 2;
}

void RunTimeStatusModularTimeSeries::setThreshold(double threshold) {
  _threshold = threshold;
}
void RunTimeStatusModularTimeSeries::setResetCeiling(double ceiling) {
  _resetCeiling = ceiling;
}
void RunTimeStatusModularTimeSeries::setResetFloor(double floor) {
  _resetFloor = floor;
}
void RunTimeStatusModularTimeSeries::setResetTolerance(double tolerance) {
  _resetTolerance = tolerance;
}

Point RunTimeStatusModularTimeSeries::point(time_t time){
  
  // try to get the point from the base class
  Point newPoint = TimeSeries::point(time);
  // but if its not there, then you probably need to use a different method
  if (!newPoint.isValid) {
    std::cerr << "RunTimeStatusModularTimeSeries::point: \"" << this->name() << "\": check point availability first\n";
  }
  return newPoint;
}

Point RunTimeStatusModularTimeSeries::pointBefore(time_t time) {
  
//  Point beforePoint = pointNext(time, before);
  
  cerr << "RunTimeStatus pointBefore method not implemented" << endl;
  Point beforePoint;
  return beforePoint;
}

Point RunTimeStatusModularTimeSeries::pointAfter(time_t time) {
  
  Point afterPoint = pointNext(time, after);
  
  return afterPoint;
}

void RunTimeStatusModularTimeSeries::setClock(Clock::sharedPointer clock) {
  // refuse to retain our own clock so that pointRecord->pointBefore accesses our record and not the sources
  std::cerr << "RunTimeStatusModularTimeSeries clock can not be set\n";
}

Point RunTimeStatusModularTimeSeries::pointNext(time_t time, searchDirection_t dir) {
  
//  cout << endl << endl << "PointAfter: " << time << endl;
  
  Point lastPoint, nextPoint, newPoint;
  Units sourceU = source()->units();
  
  // Get a starting source and status point
  std::pair< Point, Point > startPoints = beginningStatus(time, dir, sourceU);
  Point sourcePoint = startPoints.first;
  Point statusPoint = startPoints.second;
  
  _cachedSourcePoint = sourcePoint;
  lastPoint = sourcePoint;
  double lastTime = (double)statusPoint.time;
  double lastStatus = statusPoint.value;
  double changeTime = lastTime;
  
//  cout << "<<t=" << lastTime << " v=" << lastPoint.value << endl;
  
  // have a valid starting point and status -- move forward to detect a status change
  bool changeStatus = false;
  double dT = 0, dR = 0, accumTime = 0, accumRuntime = 0;
  while (!changeStatus) {
    
    // get the next point
    nextPoint = (dir == after) ? source()->pointAfter((time_t)lastTime) : source()->pointBefore((time_t)lastTime);
    if (!nextPoint.isValid) {
      return newPoint; // no change in status so return nothing
    }
    nextPoint = Point::convertPoint(nextPoint, sourceU, RTX_SECOND);
    
//    if (nextPoint.time <= time) {
//      cout << "<<t=" << nextPoint.time << " v=" << nextPoint.value << endl;
//    }
//    else {
//      cout << "  t=" << nextPoint.time << " v=" << nextPoint.value << endl;
//    }

    // update the cumulative time
    dT = (double)nextPoint.time - lastTime;
    accumTime += dT;
    
    // update the cumulative runtime
    dR = nextPoint.value - lastPoint.value;
    if (dR < -_resetTolerance) {
      if (lastStatus) {
        dR = min(dT, (_resetCeiling - lastPoint.value) + (nextPoint.value - _resetFloor));
        dR = (dR >= -_resetTolerance) ? dR : 0; // only way to handle non-reset RT value error
      }
      else {
        dR = 0; // possible to skip some 'on' time for certain resets?
      }
    }
    accumRuntime += dR;
    
//    cout << "    dT=" << dT << "; dR=" << dR << "; sumT=" << accumTime << "; sumR=" << accumRuntime << endl;
    
    // detect a new status point
    if (lastStatus) {
      // was on
      if ( (accumTime - accumRuntime) > _threshold ) {
        // now off - new status point
        lastStatus = 0;
        changeTime += accumRuntime;
        if ((time_t)changeTime > time) changeStatus = true;
        newPoint = Point((time_t)changeTime, (double)lastStatus, Point::good, _threshold);
        _cachedPoint = newPoint;
        // new source point
        double runtime = _cachedSourcePoint.value;
        if ( accumRuntime >= (_resetCeiling - runtime) && (_resetCeiling > _resetFloor) ) {
          runtime = _resetFloor + fmod( (accumRuntime - (_resetCeiling - runtime)), (_resetCeiling - _resetFloor) );
        }
        else {
          runtime += accumRuntime;
        }
        _cachedSourcePoint = Point((time_t)changeTime, runtime, Point::good, _threshold);
        // update accumulators
        accumRuntime = 0;
        accumTime = (double)nextPoint.time - changeTime;
//        cout <<  "  *** 1->0 Status Change: t=" << (time_t)changeTime << endl;
      }
    }
    else {
      // was off
      if ( accumRuntime > _threshold ) {
        // now on - determine the status change
        lastStatus = 1;
        changeTime += accumTime - accumRuntime;
        if ((time_t)changeTime > time) changeStatus = true;
        newPoint = Point((time_t)changeTime, (double)lastStatus, Point::good, _threshold);
        _cachedPoint = newPoint;
        _cachedSourcePoint = Point((time_t)changeTime, _cachedSourcePoint.value, Point::good, _threshold);
        accumTime = accumRuntime;
//        cout <<  "  *** 0->1 Status Change: t=" << (time_t)changeTime << endl;
      }
    }
    
    lastPoint = nextPoint;
    lastTime = (double)nextPoint.time;
  }
  
  // found a new status point
  return newPoint;
}

std::vector<Point> RunTimeStatusModularTimeSeries::filteredPoints(TimeSeries::sharedPointer sourceTs, time_t fromTime, time_t toTime) {
  
  vector<Point> statusPoints;
  
  Point tempPoint = _cachedPoint;
  Point tempSourcePoint = _cachedSourcePoint;
  Point aPoint = pointAfter(fromTime - 1);
  
  while ( aPoint.isValid && (aPoint.time <= toTime) ) {
    statusPoints.push_back(aPoint);
    tempPoint = _cachedPoint;
    tempSourcePoint = _cachedSourcePoint;
    aPoint = pointAfter(aPoint.time);
  }
  
  _cachedPoint = tempPoint;
  _cachedSourcePoint = tempSourcePoint;
  return statusPoints;
}

std::pair< Point, Point > RunTimeStatusModularTimeSeries::beginningStatus(time_t time, searchDirection_t dir, Units sourceU) {
  
  Point statusPoint, sourcePoint;
  bool cachedPointIsValid;
  if (dir == after) {
    cachedPointIsValid =  (_cachedPoint.isValid && _cachedPoint.time <= time && _cachedPoint.time > (time - _statusPointShelfLife) );
  }
  else {
    cachedPointIsValid =  (_cachedPoint.isValid && _cachedPoint.time >= time && _cachedPoint.time < (time + _statusPointShelfLife) );
  }
  
  if ( cachedPointIsValid ) {
    // start with a known prior status point
    statusPoint = _cachedPoint;
    sourcePoint = _cachedSourcePoint;
  }
  else {
    // start a few points back so we can figure out the status
    time_t lastTime = time;
    Point aPoint, lastPoint;
    for (int iBack = 0; iBack < _windowSize; iBack++) {
      aPoint = (dir == after) ? source()->pointBefore(lastTime) : source()->pointAfter(lastTime);
      if (!aPoint.isValid) {
        break; // failed - can't start from before
      }
      lastPoint = aPoint;
      lastTime = lastPoint.time;
    }
    if (!lastPoint.isValid) {
      // Failed to look backward; try the current time and after
      aPoint = source()->point(time);
      if (!aPoint.isValid) {
        aPoint = (dir == after) ? source()->pointAfter(time) : source()->pointBefore(time);
      }
      lastPoint = aPoint;
    }
    sourcePoint = Point::convertPoint(lastPoint, sourceU, RTX_SECOND);
    if (sourcePoint.isValid) {
      statusPoint = Point(sourcePoint.time, 0.0, Point::good, _threshold);
    }
    else {
      statusPoint = Point(sourcePoint.time, 0.0, Point::bad, _threshold);
    }
  }
  
  return std::make_pair(sourcePoint, statusPoint);
}

