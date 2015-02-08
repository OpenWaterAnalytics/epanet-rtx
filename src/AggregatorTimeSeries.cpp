//
//  AggregatorTimeSeries.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#include <iostream>

#include "AggregatorTimeSeries.h"
#include <boost/foreach.hpp>
#include <boost/range/adaptors.hpp>
#include <set>

using namespace RTX;
using namespace std;

ostream& AggregatorTimeSeries::toStream(ostream &stream) {
  TimeSeries::toStream(stream);
  stream << "Connected to: " << _tsList.size() << " time series:" << "\n";
  
  typedef std::pair<TimeSeries::_sp,double> tsMultPair_t;
  BOOST_FOREACH(const AggregatorSource& aggSource, _tsList) {
    string dir = (aggSource.multiplier > 0)? "(+)" : "(-)";
    stream << "    " << dir << " " << aggSource.timeseries->name() << endl;
  }
  return stream;
}

TimeSeries::_sp AggregatorTimeSeries::source() {
  if (this->sources().size() > 0) {
    return this->sources().front().timeseries;
  }
  return TimeSeries::_sp();
}

void AggregatorTimeSeries::setSource(TimeSeries::_sp ts) {
  // nope
}

void AggregatorTimeSeries::addSource(TimeSeries::_sp timeSeries, double multiplier) throw(RtxException) {
  
  // check compatibility
  bool isCompatible = false;
  
  if (this->sources().size() == 0) {
    isCompatible = true;
  }
  else if (timeSeries->units().isSameDimensionAs(this->units())) {
    isCompatible = true;
  }
  
  if (!isCompatible) {
    cerr << "Incompatible time series: " << *timeSeries << endl;
    return;
  }
  
  if (units().isDimensionless() && sources().size() == 0) {
    // we have default units and no sources yet, so it would be safe to adopt the new source's units.
    this->setUnits(timeSeries->units());
  }
  
  _tsList.push_back((AggregatorSource){timeSeries,multiplier});
  this->invalidate();
}

void AggregatorTimeSeries::removeSource(TimeSeries::_sp timeSeries) {
  
  std::vector< AggregatorSource > newSourceList;
  
  // find the index of the requested time series
  BOOST_FOREACH(AggregatorSource aggSource, _tsList) {
    if (timeSeries == aggSource.timeseries) {
      // don't copy
    }
    else {
      newSourceList.push_back(aggSource);
    }
  }
  // save the new source list
  _tsList = newSourceList;
  
  this->invalidate();
}

std::vector< AggregatorTimeSeries::AggregatorSource > AggregatorTimeSeries::sources() {
  return _tsList;
}

void AggregatorTimeSeries::setMultiplierForSource(TimeSeries::_sp timeSeries, double multiplier) {
  // _tsList[x].first == TimeSeries, _tsList[x].second == multipier
  // (private) std::vector< std::pair<TimeSeries::_sp,double> > _tsList;
  typedef std::pair<TimeSeries::_sp, double> tsDoublePair_t;
  BOOST_FOREACH(AggregatorSource& item, _tsList) {
    if (item.timeseries == timeSeries) {
      item.multiplier = multiplier;
      // todo -- reset pointrecord backing store?
      this->invalidate();
    }
  }
}



Point AggregatorTimeSeries::pointBefore(time_t time) {

  std::set<time_t> timeSet;
  
  if (this->clock()) {
    timeSet.insert(this->clock()->timeBefore(time));
  }
  else {
    BOOST_FOREACH(AggregatorSource& item, _tsList) {
      time_t thisBefore = item.timeseries->pointBefore(time).time;
      timeSet.insert(thisBefore);
    }
  }
    
  if (timeSet.empty()) {
    return Point();
  }
  
  set<time_t>::reverse_iterator rIt = timeSet.rbegin();
  time_t before = *rIt;
  
  return this->point(before);
}


Point AggregatorTimeSeries::pointAfter(time_t time) {
  
  std::set<time_t> timeSet;
  
  if (this->clock()) {
    timeSet.insert(this->clock()->timeAfter(time));
  }
  else {
    BOOST_FOREACH(AggregatorSource& item, _tsList) {
      time_t thisAfter = item.timeseries->pointAfter(time).time;
      timeSet.insert(thisAfter);
    }
  }
  
  if (timeSet.empty()) {
    return Point();
  }
  
  set<time_t>::iterator rIt = timeSet.begin();
  time_t after = *rIt;
  
  return this->point(after);
}





std::set<time_t> AggregatorTimeSeries::timeValuesInRange(TimeRange range) {
  set<time_t> timeList;
  
  if (this->clock()) {
    // align the query with the clock
    timeList = this->clock()->timeValuesInRange(range.first, range.second);
  }
  else {
    // get the set of times from the aggregator sources
    BOOST_FOREACH(AggregatorSource aggSource, this->sources()) {
      vector<Point> thisSourcePoints = aggSource.timeseries->points(range.first, range.second);
      BOOST_FOREACH(Point p, thisSourcePoints) {
        timeList.insert(p.time);
      }
    }
  }
  return timeList;
}

TimeSeries::PointCollection AggregatorTimeSeries::filterPointsInRange(TimeRange range) {
  set<time_t> droppedTimes;
  vector<Point> aggregated;
  
  
  set<time_t> desiredTimes = this->timeValuesInRange(range);
  
  // pre-load a vector of points.
  BOOST_FOREACH(time_t now, desiredTimes) {
    aggregated.push_back(Point(now));
  }
  
  BOOST_FOREACH(AggregatorSource aggSource, this->sources()) {
    TimeSeries::_sp sourceTs = aggSource.timeseries;
    double multiplier = aggSource.multiplier;
    TimeRange componentRange = range;
    Point leftseek = sourceTs->pointBefore(range.first + 1).time;
    Point rightseek = sourceTs->pointAfter(range.second - 1).time;
    componentRange.first = leftseek.time > 0 ? leftseek.time : range.first;
    componentRange.second = rightseek.time > 0 ? rightseek.time : range.second;
    
    PointCollection componentCollection = sourceTs->points(componentRange);
    componentCollection.resample(desiredTimes);
    componentCollection.convertToUnits(this->units());
    
    // make it easy to find any times that were dropped (bad points from a source series)
    map<time_t, Point> sourcePointMap;
    BOOST_FOREACH(const Point& p, componentCollection.points) {
      sourcePointMap[p.time] = p;
    }
    
    // do the aggregation.
    BOOST_FOREACH(Point& p, aggregated) {
      if (sourcePointMap.count(p.time) > 0) {
        Point addingPoint = sourcePointMap[p.time] * multiplier;
        p += addingPoint;
      }
      else {
        droppedTimes.insert(p.time);
      }
    }
  }
  
  // prune dropped points from aggregation result.
  vector<Point> goodPoints;
  BOOST_FOREACH(const Point& p, aggregated) {
    if (droppedTimes.count(p.time) == 0) {
      goodPoints.push_back(p);
    }
  }
  
  PointCollection data(goodPoints, this->units());
  data.resample(desiredTimes);
  
  return data;
}

bool AggregatorTimeSeries::canSetSource(TimeSeries::_sp ts) {
  return false;
}

void AggregatorTimeSeries::didSetSource(TimeSeries::_sp ts) {
  return;
}

bool AggregatorTimeSeries::canChangeToUnits(Units units) {
  if (this->sources().size() == 0) {
    return true;
  }
  else if (this->sources().front().timeseries->units().isSameDimensionAs(units)) {
    return true;
  }
  else {
    return false;
  }
}
