//
//  AggregatorTimeSeries.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#include <iostream>
#include <limits>

#include "AggregatorTimeSeries.h"
#include <boost/foreach.hpp>
#include <boost/range/adaptors.hpp>
#include <set>
#include <future>

using namespace RTX;
using namespace std;

ostream& AggregatorTimeSeries::toStream(ostream &stream) {
  TimeSeries::toStream(stream);
  stream << "Connected to: " << _tsList.size() << " time series:" << "\n";
  
  for(const AggregatorSource& aggSource: _tsList) {
    string dir = (aggSource.multiplier > 0)? "(+)" : "(-)";
    stream << "    " << dir << " " << aggSource.timeseries->name() << endl;
  }
  return stream;
}


AggregatorTimeSeries::AggregatorTimeSeries() {
  _mode = AggregatorModeSum;
}

AggregatorTimeSeries::AggregatorMode AggregatorTimeSeries::aggregatorMode() {
  return _mode;
}
void AggregatorTimeSeries::setAggregatorMode(RTX::AggregatorTimeSeries::AggregatorMode mode) {
  _mode = mode;
  this->invalidate();
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
  for(AggregatorSource aggSource: _tsList) {
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
  for(AggregatorSource& item: _tsList) {
    if (item.timeseries == timeSeries) {
      item.multiplier = multiplier;
      // todo -- reset pointrecord backing store?
      this->invalidate();
    }
  }
}


time_t AggregatorTimeSeries::timeBefore(time_t time) {
  std::set<time_t> timeSet;
  
  if (this->clock()) {
    timeSet.insert(this->clock()->timeBefore(time));
  }
  else {
    for(AggregatorSource& item: _tsList) {
      timeSet.insert(item.timeseries->timeBefore(time));
    }
  }
  
  if (timeSet.empty()) {
    return 0;
  }
  
  set<time_t>::reverse_iterator rIt = timeSet.rbegin();
  time_t before = *rIt;
  return before;
}

time_t AggregatorTimeSeries::timeAfter(time_t time) {
  std::set<time_t> timeSet;
  
  if (this->clock()) {
    timeSet.insert(this->clock()->timeAfter(time));
  }
  else {
    for(AggregatorSource& item: _tsList) {
      timeSet.insert(item.timeseries->timeAfter(time));
    }
  }
  
  if (timeSet.empty()) {
    return 0;
  }
  
  set<time_t>::iterator rIt = timeSet.begin();
  time_t after = *rIt;
  return after;
}



std::set<time_t> AggregatorTimeSeries::timeValuesInRange(TimeRange range) {
  set<time_t> timeList;
  
  if (this->clock()) {
    // align the query with the clock
    timeList = this->clock()->timeValuesInRange(range);
  }
  else {
    // get the set of times from the aggregator sources
    for(AggregatorSource aggSource: this->sources()) {
      set<time_t> sourceTimes = aggSource.timeseries->timeValuesInRange(range);
      timeList.insert(sourceTimes.begin(), sourceTimes.end());
    }
  }
  return timeList;
}

PointCollection AggregatorTimeSeries::filterPointsInRange(TimeRange range) {
  set<time_t> droppedTimes;
  vector<Point> aggregated;
  double nSources = (double)(this->sources().size());
  
  set<time_t> desiredTimes = this->timeValuesInRange(range);
  
  // pre-load a vector of points.
  for(time_t now: desiredTimes) {
    Point p(now);
    
    switch (_mode) {
      case AggregatorModeMin:
        p.value = std::numeric_limits<double>::max();
        break;
      case AggregatorModeMax:
        p.value = -(std::numeric_limits<double>::max());
        break;
      default:
        break;
    }
    
    p.addQualFlag(Point::rtx_aggregated);
    aggregated.push_back(p);
  }
  
  
  
  // asynchronously get source series
  vector< future< map< time_t, Point> > > aggSeriesData;
  auto mySources = this->sources();
  auto mode = this->_mode;
  for(AggregatorSource sourceDesc : mySources) {
    aggSeriesData.push_back(async(launch::async, std::bind([=](AggregatorSource sd) -> map< time_t, Point> {
      TimeSeries::_sp sourceTs = sd.timeseries;
      double multiplier = sd.multiplier;
      TimeRange componentRange = range;
      componentRange.start = sourceTs->timeBefore(range.start + 1);
      componentRange.end = sourceTs->timeAfter(range.end - 1);
      componentRange.correctWithRange(range);
      
      PointCollection componentCollection = sourceTs->pointCollection(componentRange);
      if (mode != AggregatorModeUnion) {
        componentCollection.resample(desiredTimes);
      }
      componentCollection.convertToUnits(this->units());
      
      // make it easy to find any times that were dropped (bad points from a source series)
      map<time_t, Point> sourcePointMap;
      componentCollection.apply([&](const Point& p){
        sourcePointMap[p.time] = p * multiplier;
      });
      return sourcePointMap;
    }, sourceDesc) //bind
                                  ) //async
                            ); //push_back;
  }//for sourceDesc
  
  
  
  set<time_t> unionSet;
  
  for(auto &task : aggSeriesData) {
    
    map<time_t, Point> sourcePointMap = task.get();
    
    // do the aggregation.
    for(Point& p: aggregated) {
      if (sourcePointMap.count(p.time) > 0) {
        Point pointToAggregate = sourcePointMap[p.time]; // already multiplied
        
        switch (_mode) {
          case AggregatorModeSum:
          {
            p += pointToAggregate;
          }
            break;
          case AggregatorModeMax:
          {
            if (p.value < pointToAggregate.value) {
              p = pointToAggregate;
            }
          }
            break;
          case AggregatorModeMean:
          {
            p += pointToAggregate / nSources;
          }
            break;
          case AggregatorModeMin:
          {
            if (p.value > pointToAggregate.value) {
              p = pointToAggregate;
            }
          }
            break;
          case AggregatorModeUnion:
          {
            if (unionSet.count(pointToAggregate.time) == 0) {
              p = pointToAggregate;
              unionSet.insert(pointToAggregate.time);
            }
          }
            break;
          default:
            break;
        }
        
      }
      else {
        if (_mode != AggregatorModeUnion) {
          droppedTimes.insert(p.time); // if any member is missing, then remove the point from the output
        }
      }
    }
  }
  
  // prune dropped points from aggregation result.
  vector<Point> goodPoints;
  for(const Point& p: aggregated) {
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
