#include "RunTimeStatusModularTimeSeries.h"
#include <boost/foreach.hpp>
#include <math.h>

using namespace std;
using namespace RTX;

RunTimeStatusModularTimeSeries::RunTimeStatusModularTimeSeries() {
  _ratioThreshold = 0.9;
  _timeThreshold = 60;
  _resetTolerance = 0.0;
  _timeErrorTolerance = 0.0;
  _resetCeiling = 0.0;
  _resetFloor = 0.0;
}

void RunTimeStatusModularTimeSeries::setRatioThreshold(double threshold) {
  _ratioThreshold = threshold;
}
void RunTimeStatusModularTimeSeries::setTimeThreshold(double threshold) {
  _timeThreshold = threshold;
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
void RunTimeStatusModularTimeSeries::setTimeErrorTolerance(double tolerance) {
  _timeErrorTolerance = tolerance;
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
  
  Point afterPoint = pointNext(time);
  if (afterPoint.isValid) {
    vector< Point > aPoint(1,afterPoint);
    this->insertPoints(aPoint); // should be able to use insert()
  }
  return afterPoint;
}

void RunTimeStatusModularTimeSeries::setClock(Clock::_sp clock) {
  // refuse to retain our own clock so that pointRecord->pointBefore accesses our record and not the sources
  std::cerr << "RunTimeStatusModularTimeSeries clock can not be set\n";
}

Point RunTimeStatusModularTimeSeries::pointNext(time_t time) {

//  cout << endl << endl << "PointNext: " << time << endl;

  Point lastStatusPoint, nextStatusPoint, lastSourcePoint, nextSourcePoint;
  Units sourceU = source()->units();

  // Get a beginning status point at current time
  lastStatusPoint = beginningStatus(time);
  if (!lastStatusPoint.isValid) {
    return Point(); // Can't find the current status; return nothing
  }
  // Interpolate a starting source point
  lastSourcePoint = beginningSource(lastStatusPoint);
  if (!lastSourcePoint.isValid) {
    return Point(); // Can't get any source points; return nothing
  }

//  cout << "t=" << lastSourcePoint.time << " v=" << lastSourcePoint.value << endl;

  // have a valid starting point and status -- move forward to detect the next status change
  bool changeStatus = false;
  double dR = 0, accumRuntime = 0;
  time_t dT = 0, accumTime = 0;
  while (!changeStatus) {

    // get the next point
    nextSourcePoint = source()->pointAfter(lastSourcePoint.time);
    if (!nextSourcePoint.isValid) {
      return Point(); // no change in status so return nothing
    }
    nextSourcePoint = Point::convertPoint(nextSourcePoint, sourceU, RTX_SECOND);

//    cout << "t=" << nextSourcePoint.time << " v=" << nextSourcePoint.value << endl;

    // update the cumulative time and runtime
    dT = nextSourcePoint.time - lastSourcePoint.time;
    accumTime += dT;
    dR = nextSourcePoint.value - lastSourcePoint.value;
    if (dR < -_resetTolerance) {
      // handle runtime resets - hard to distinguish between valid reset and bad value
      if (lastStatusPoint.value) {
        // reset model is value-based - bounces from a ceiling down to a floor value
        // (this happens in NKWD data; not sure where else!) If _resetCeiling=_resetFloor=0 (default) then dR=0
        dR = min((double)dT, (_resetCeiling - lastSourcePoint.value) + (nextSourcePoint.value - _resetFloor));
        if (dR < -_resetTolerance) {
          // bad value, so skip the point
          dR = dT;
//          nextSourcePoint.value = lastSourcePoint.value;
        }
      }
      else {
        // bad value, so skip the point
        dR = 0; // possible to skip some 'on' time for certain resets?
//        nextSourcePoint.value = lastSourcePoint.value;
      }
//      cout << "Runtime Reset ::: " << this->name() << " ::: time = " << nextSourcePoint.time << " ::: dR = " << dR << endl;
    }
    else if (dR > dT + _timeErrorTolerance) {
      // just shouldn't happen
      if (lastStatusPoint.value) {
        dR = dT;
      }
      else {
        dR = 0;
      }
    }
    accumRuntime += dR;

//    cout << "    dT=" << dT << "; dR=" << dR << "; sumT=" << accumTime << "; sumR=" << accumRuntime << endl;
    
    // detect a status change
    if (lastStatusPoint.value) {
      // was on
      if ( dR/(double)dT < 1 - _ratioThreshold ) {
//      if ((double)accumTime - accumRuntime > 60) {
        // now off - new status point
        changeStatus = true;
        time_t changeStatusTime = lastStatusPoint.time + (time_t)accumRuntime;
        if (changeStatusTime <= lastStatusPoint.time) {
          // guard against rounding
          changeStatusTime = lastStatusPoint.time+1;
        }
        // insert the new status change at the time consistent with accumulated runtime
        nextStatusPoint = Point(changeStatusTime, 0., Point::good, 0.);
        vector< Point > aPoint(1,nextStatusPoint);
        this->insertPoints(aPoint); // should be able to use insert()
        // but return the status when the change was detected
        nextStatusPoint = Point(max(lastStatusPoint.time + accumTime, changeStatusTime), 0., Point::good, 0.);
//        cout <<  "  *** 1->0 Status Change: t=" << lastStatusPoint.time + accumRuntime << endl;
      }
    }
    else {
      // was off
      if ( dR/(double)dT > _ratioThreshold ) {
        // now on - new status point
        changeStatus = true;
        time_t changeStatusTime = lastStatusPoint.time + accumTime - (time_t)accumRuntime;
        if (changeStatusTime <= lastStatusPoint.time) {
          // status change is bogus - maybe a bad value or a bad last status
          changeStatusTime = lastStatusPoint.time+1;
//          cout << "  *** bad status change >>> " << endl;
        }
        // insert the new status change at the time corrected for accumulated runtime
        nextStatusPoint = Point(changeStatusTime, 1., Point::good, 0.);
        vector< Point > aPoint(1,nextStatusPoint);
        this->insertPoints(aPoint); // should be able to use insert()
        // but return the status when the change was detected
        nextStatusPoint = Point(max(lastStatusPoint.time + accumTime, changeStatusTime), 1., Point::good, 0.);
//        cout <<  "  *** 0->1 Status Change: t=" << changeStatusTime << endl;
      }
    }
    
    lastSourcePoint = nextSourcePoint;
  }
  
  // found a new status point
  return nextStatusPoint;
}

Point RunTimeStatusModularTimeSeries::beginningSource(Point lastStatusPoint) {
  // Interpolate a starting source point - assume that lastStatusPoint is right; create consistent source point at same time
  
  Units sourceU = source()->units();
  Point lastSourcePoint;
  
  Point p1 = source()->pointBefore(lastStatusPoint.time);
  if (!p1.isValid) {
    return Point(); // no source points - return nothing
  }
  p1 = Point::convertPoint(p1, sourceU, RTX_SECOND);
  
  Point p2 = source()->pointAfter(lastStatusPoint.time-1);
  if (!p2.isValid) {
    return Point(); // no source points - return nothing
  }
  p2 = Point::convertPoint(p2, sourceU, RTX_SECOND);
  
  // try to detect and handle runtime resets
  if (p2.value < p1.value) {
    p2.value = p2.value; // too simple?
  }

  double rt;
  if (lastStatusPoint.value) {
    // On
    rt = p2.value + (double)(lastStatusPoint.time - p2.time);
    rt = max(rt,p1.value);
  }
  else {
    // Off
    rt = p1.value + (double)(lastStatusPoint.time - p1.time);
    rt = min(rt,p2.value);
  }

  return lastSourcePoint = Point(lastStatusPoint.time, rt, Point::good, 0.);
}

Point RunTimeStatusModularTimeSeries::beginningStatus(time_t time) {

  Units sourceU = source()->units();

  // trying to figure out the status now, preliminary to determining the next change
  
  Point statusPoint = TimeSeries::point(time);
  if (statusPoint.isValid) {
    return statusPoint;  // time is a known status change
  }
  
  // find a bracketing source point interval [p1,p2] where p1 <= time (hopefully) && p2.time > time
  Point p1 = source()->pointAtOrBefore(time);
  if (!p1.isValid) {
    p1 = source()->pointAfter(time);
    if (!p1.isValid) {
      return Point(); // no source points!
    }
  }
  Point p2 = source()->pointAfter(p1.time);
  if (!p2.isValid) {
    return Point(); // no source points!
  }
  p1 = Point::convertPoint(p1, sourceU, RTX_SECOND);
  p2 = Point::convertPoint(p2, sourceU, RTX_SECOND);

  // OK we have the interval [p1,p2] - let's figure out a status
  double dR = p2.value - p1.value;
  time_t dT = p2.time - p1.time;
  if (dR/(double)dT >= _ratioThreshold) {
    statusPoint = Point(time, 1.0, Point::good); // on
  }
  else {
    statusPoint = Point(time, 0.0, Point::good); // off
  }
  
  return statusPoint;
}
