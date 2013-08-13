#include "RunTimeStatusModularTimeSeries.h"
#include <boost/foreach.hpp>
#include <math.h>

using namespace std;
using namespace RTX;

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
  
  Point beforePoint;
  
  std::cerr << "RunTimeStatusModularTimeSeries::pointBefore: \"" << this->name() << "\": no pointBefore method\n";

  return beforePoint;
}

Point RunTimeStatusModularTimeSeries::pointAfter(time_t time) {
  
//  cout << endl << endl << "PointAfter: " << time << endl;
  
  Point lastPoint, nextPoint, newPoint;
  static int lookbackWindowSize = 2; // we will start this many points before
  static time_t oneWeek(60*60*24*7);
  static time_t statusPointShelfLife(12*oneWeek);
  Units sourceU = source()->units();
  
  double lastTime, changeTime;
  int lastStatus;
  if (_cachedPoint.isValid && _cachedPoint.time <= time && _cachedPoint.time > (time - statusPointShelfLife) ) {
    // start with a known prior status point
    lastStatus = _cachedPoint.value;
    lastTime = (double)_cachedPoint.time;
    changeTime = lastTime;
    // and a known prior runtime point
    lastPoint = _cachedSourcePoint;
  }
  else {
    // start a few points back and figure out the status
    time_t backTime = time;
    for (int iBack = 0; iBack < lookbackWindowSize; iBack++) {
      lastPoint = source()->pointBefore(backTime);
      if (!lastPoint.isValid) {
        return newPoint; // failed - can't determine change in status
      }
      lastPoint = Point::convertPoint(lastPoint, sourceU, RTX_SECOND);
      backTime = lastPoint.time;
    }
    _cachedSourcePoint = lastPoint;
    lastStatus = 0; // arbitrary starting guess
    lastTime = (double)lastPoint.time;
    changeTime = lastTime;
  }
  
//  cout << "<<t=" << lastTime << " v=" << lastPoint.value << endl;
  
  // have a valid starting point and status -- move forward to detect a status change
  bool changeStatus = false;
  double dT = 0, dR = 0, accumTime = 0, accumRuntime = 0;
  while (!changeStatus) {
    
    // get the next point
    nextPoint = source()->pointAfter((time_t)lastTime);
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
        if ((time_t)changeTime >= time) changeStatus = true;
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
        if ((time_t)changeTime >= time) changeStatus = true;
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
