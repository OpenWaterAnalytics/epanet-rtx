//
//  ModularTimeSeries.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#include <iostream>
#include <boost/foreach.hpp>
#include "ModularTimeSeries.h"

using namespace RTX;
using namespace std;

ModularTimeSeries::ModularTimeSeries() : TimeSeries::TimeSeries() {
  _doesHaveSource = false;
}

ModularTimeSeries::~ModularTimeSeries() {
  
}

ostream& ModularTimeSeries::toStream(ostream &stream) {
  TimeSeries::toStream(stream);
  stream << "Connected to: " << *_source << "\n";
  return stream;
}

void ModularTimeSeries::setSource(TimeSeries::sharedPointer sourceTimeSeries) {
  if( isCompatibleWith(sourceTimeSeries) ) {
    _source = sourceTimeSeries;
    _doesHaveSource = true;
    //resetCache();
    // if this is an irregular time series, then set this clock to the same as that guy's clock.
    if (!clock()->isRegular()) {
      setClock(source()->clock());
    }
    // and if i don't have units, just borrow from the source.
    if (units().isDimensionless()) {
      setUnits(_source->units()); // as a copy, in case it changes.
    }
  }
  else {
    cerr << "Incompatible. Could not set source for:\n";
    cerr << *this;
    // TODO -- throw something?
  }
}

TimeSeries::sharedPointer ModularTimeSeries::source() {
  return _source;
}

bool ModularTimeSeries::doesHaveSource() {
  return _doesHaveSource;
}

void ModularTimeSeries::setUnits(Units newUnits) {
  if (!doesHaveSource() || (doesHaveSource() && newUnits.isSameDimensionAs(source()->units()))) {
    TimeSeries::setUnits(newUnits);
  }
  else {
    cerr << "could not set units for time series " << name() << endl;
  }
}
/*
bool ModularTimeSeries::isPointAvailable(time_t time) {
  bool isCacheAvailable = false, isSourceAvailable = false;
  
  isCacheAvailable = TimeSeries::isPointAvailable(time);
  
  if (!isCacheAvailable) {
    isSourceAvailable = source()->isPointAvailable(time);
  }
  
  if (isCacheAvailable || isSourceAvailable) {
    return true;
  }
  else {
    return false;
  }
}
*/
Point ModularTimeSeries::point(time_t time) {
  // check the base-class availability. if it's cached or stored here locally, then send it on.
  // otherwise, check the upstream availability. if it's there, store it locally and pass it on.
  
  // check the requested time for validity
  // if the time is not valid, rewind until a valid time is reached.
  time_t newTime = clock()->validTime(time);

  // if my clock can't find it, maybe my source's clock can?
  if (newTime == 0) {
    time = source()->clock()->validTime(time);
  } else {
    time = newTime;
  }
  
  Point p = TimeSeries::point(time);
  if (p.isValid) {
    return p;
  }
  else {
    
    Point sourcePoint = source()->point(time);

    if (sourcePoint.isValid) {
      // create a new point object and convert from source units
      Point aPoint = Point::convertPoint(sourcePoint, source()->units(), units());
      insert(aPoint);
      return aPoint;
    }
    else {
      cerr << "ModularTimeSeries: check point availability first\n";
      // TODO -- throw something? reminder to check point availability first...
      Point point;
      return point;
    }
  }
}

Point ModularTimeSeries::pointBefore(time_t time) {
  time_t timeBefore = clock()->timeBefore(time);
  if (timeBefore == 0) {
    timeBefore = source()->clock()->timeBefore(time);
  }
  return point(timeBefore);
}

Point ModularTimeSeries::pointAfter(time_t time) {
  time_t timeAfter = clock()->timeAfter(time);
  if (timeAfter == 0) {
    timeAfter = source()->clock()->timeAfter(time);
  }
  return point(timeAfter);
}

vector< Point > ModularTimeSeries::points(time_t start, time_t end) {
  
  if (!clock()->isRegular()) {
    // if the clock is irregular, there's no easy way around this.
    // call the source points method to get it to cache points...
    //cout << "calling source cache (" << source()->name() << ")" << endl;
    time_t sStart = start;
    time_t sEnd = end;
    
    // widen the source window
    for (int i = 0; i < margin(); ++i) {
      sStart = source()->clock()->timeBefore(sStart);
      sEnd = source()->clock()->timeAfter(sEnd);
    }
    
    sStart = sStart>0 ? sStart : start;
    sEnd = sEnd>0 ? sEnd : end;
    
    vector<Point> sourcePoints = source()->points(sStart, sEnd);
    vector<Point> filtered = this->filteredPoints(start, end, sourcePoints);
    this->insertPoints(filtered);
    return filtered;
  }
  
  
  // otherwise, the clock is regular.
  // we can do some important optimizations here.
  
  // make sure the times are aligned with the clock.
  time_t newStart = (clock()->isValid(start)) ? start : clock()->timeAfter(start);
  time_t newEnd = (clock()->isValid(end)) ? end : clock()->timeBefore(end);
  
  PointRecord::time_pair_t prRange = record()->range(name());
  if (prRange.first <= newStart && newEnd <= prRange.second) {
    // the record's range covers it, but
    // the record may not be continuous -- so check it.
    time_t now = newStart;
    vector<Point> rpVec = record()->pointsInRange(name(), newStart, newEnd);
    if (rpVec.size() == 0) {
      // fully internal
      // like this:
      // ppppp---[--- req ---]---ppppp
      
      rpVec.push_back(record()->pointBefore(name(), newStart));
      rpVec.push_back(record()->pointAfter(name(), newEnd));
      
    }
    
    
    vector<Point> stitchedPoints;
    vector<Point>::const_iterator it = rpVec.begin();
    while (it != rpVec.end()) {
      Point recordPoint = *it;
      // cout << "P: " << recordPoint << endl;
      
      if (recordPoint.time == now) {
        stitchedPoints.push_back(recordPoint);
        now = clock()->timeAfter(now);
      }
      else if (recordPoint.time < now) {
        ++it;
        continue;
      }
      else {
        // aha, a gap.
        // determine the size of the gap
        time_t gapStart, gapEnd;
        
        gapStart = now;
        gapEnd = recordPoint.time;
        
        Point gapSourceStart = Point(gapStart, 0);
        Point gapSourceEnd = recordPoint;
        
        for (int i = 0; i < margin(); ++i) {
          gapSourceStart = source()->pointBefore(gapSourceStart.time);
          gapSourceEnd = source()->pointAfter(gapSourceEnd.time);
        }
        
        
        vector<Point> gapSourcePoints = source()->points(gapSourceStart.time, gapSourceEnd.time);
        vector<Point> gapPoints = filteredPoints(gapStart, gapEnd, gapSourcePoints);
        if (gapPoints.size() > 0) {
          this->insertPoints(gapPoints);
          now = gapPoints.back().time;
          BOOST_FOREACH(Point p, gapPoints) {
            stitchedPoints.push_back(p);
          }
        }
        else {
          // skipping the gap.
          now = recordPoint.time;
          continue; // skip the ++it
        }
        
      }
      
      ++it;
    }
    
    return stitchedPoints;
  }
  
  // otherwise, construct new points.
  // get the times for the source query
  Point sourceStart, sourceEnd;
  Point s = source()->pointBefore(newStart);
  Point e = source()->pointAfter(newEnd);
  sourceStart = (s.isValid)? s : Point(newStart,0); //newStart;
  sourceEnd = (e.isValid>0)? e : Point(newEnd,0); //newEnd;
  
  
  // get the source points
  std::vector<Point> sourcePoints = source()->points(sourceStart.time, sourceEnd.time);
  if (sourcePoints.size() < 2) {
    vector<Point> empty;
    return empty;
  }
  
  // create a place for the new points
  std::vector<Point> filtered;
  
  filtered = filteredPoints(newStart, newEnd, sourcePoints);
  
  // finally, add the points to myself.
  this->insertPoints(filtered);
  
  return filtered;
  
}


int ModularTimeSeries::margin() {
  return 1;
}


vector<Point> ModularTimeSeries::filteredPoints(time_t fromTime, time_t toTime, const vector<Point>& sourcePoints) {
  Units sourceUnits = source()->units();
  Units myUnits = units();
  vector<Point> filtered;
  
  BOOST_FOREACH(const Point& p, sourcePoints) {
    if (p.time < fromTime || p.time > toTime) {
      // skip the points we didn't ask for.
      continue;
    }
    Point aPoint = Point::convertPoint(p, sourceUnits, myUnits);
    filtered.push_back(aPoint);
  }
  
  return filtered;
}
