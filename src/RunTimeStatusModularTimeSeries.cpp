#include "RunTimeStatusModularTimeSeries.h"
#include <boost/foreach.hpp>

using namespace std;
using namespace RTX;

void RunTimeStatusModularTimeSeries::setThreshold(double threshold) {
  _threshold = threshold;
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
  
//  cout << endl << "PointAfter: " << time << endl;
  
  Point lastPoint, nextPoint, newPoint;
  static int lookbackWindowSize = 2; // we will start this many points before
  static bool fixRuntimeResets = true; // dR < 0 ? dR = 0 : dR = dR
  static time_t oneWeek(60*60*24*7);
  static time_t statusPointShelfLife(4*oneWeek);
  Units sourceU = source()->units();
  
  double lastTime, changeTime;
  int lastStatus;
  if (_cachedPoint.isValid && _cachedPoint.time <= time && _cachedPoint.time > (time - statusPointShelfLife) ) {
    // start with a known prior status point
    lastStatus = _cachedPoint.value;
    lastTime = (double)_cachedPoint.time;
    changeTime = lastTime;
    // and a known prior runtime point
    lastPoint = Point((time_t)lastTime, _cachedRuntime, Point::good, 0);
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

    // update the cumulative time and runtime
    dT = (double)nextPoint.time - lastTime;
    accumTime += dT;
    dR = nextPoint.value - lastPoint.value;
    if (fixRuntimeResets) {
      dR = (dR >= 0) ? dR : 0;
    }
    accumRuntime += dR;
    
//    cout << "    dT=" << dT << "; dR=" << dR << "; sumT=" << accumTime << "; sumR=" << accumRuntime << endl;
    
    // detect a change of status
    if (lastStatus) {
      // was on
      if ( (accumTime - accumRuntime) > _threshold ) {
        // now off - determine the status change
        lastStatus = 0;
        changeTime += accumRuntime;
        if ((time_t)changeTime >= time) changeStatus = true;
        newPoint = Point((time_t)changeTime, (double)lastStatus, Point::good, _threshold);
        _cachedPoint = newPoint;
        _cachedRuntime += accumRuntime;
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
  double tempRuntime = _cachedRuntime;
  Point aPoint = pointAfter(fromTime - 1);
  
  while ( aPoint.isValid && (aPoint.time <= toTime) ) {
    statusPoints.push_back(aPoint);
    tempPoint = _cachedPoint;
    tempRuntime = _cachedRuntime;
    aPoint = pointAfter(aPoint.time);
  }
  
  _cachedPoint = tempPoint;
  _cachedRuntime = tempRuntime;
  return statusPoints;
}
