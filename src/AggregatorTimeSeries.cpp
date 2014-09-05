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

using namespace RTX;
using namespace std;

TimeSeries::sharedPointer AggregatorTimeSeries::source() {
  vector< AggregatorSource > sourceVec = this->sources();
  if (sourceVec.size() > 0) {
    return sourceVec.front().timeseries;
  }
  else {
    TimeSeries::sharedPointer empty;
    return empty;
  }
}
void AggregatorTimeSeries::setSource(TimeSeries::sharedPointer source) {
  return;
}
bool AggregatorTimeSeries::doesHaveSource() {
  return !(_tsList.empty());
}

ostream& AggregatorTimeSeries::toStream(ostream &stream) {
  TimeSeries::toStream(stream);
  stream << "Connected to: " << _tsList.size() << " time series:" << "\n";
  
  typedef std::pair<TimeSeries::sharedPointer,double> tsMultPair_t;
  BOOST_FOREACH(const AggregatorSource& aggSource, _tsList) {
    string dir = (aggSource.multiplier > 0)? "(+)" : "(-)";
    stream << "    " << dir << " " << aggSource.timeseries->name() << endl;
  }
  return stream;
}

void AggregatorTimeSeries::addSource(TimeSeries::sharedPointer timeSeries, double multiplier) throw(RtxException) {
  
  // check compatibility
  if (!isCompatibleWith(timeSeries)) {
    cerr << "Incompatible time series: " << *timeSeries << endl;
    return;
  }
  
  if (units().isDimensionless() && sources().size() == 0) {
    // we have default units and no sources yet, so it would be safe to adopt the new source's units.
    this->setUnits(timeSeries->units());
  }
  
  _tsList.push_back((AggregatorSource){timeSeries,multiplier});
  
  // set my clock to the lesser-period of any source.
  /*
  if (this->clock()->period() < timeSeries->clock()->period()) {
    this->setClock(timeSeries->clock());
  }
  */
}

void AggregatorTimeSeries::removeSource(TimeSeries::sharedPointer timeSeries) {
  
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
}

std::vector< AggregatorTimeSeries::AggregatorSource > AggregatorTimeSeries::sources() {
  return _tsList;
}

void AggregatorTimeSeries::setMultiplierForSource(TimeSeries::sharedPointer timeSeries, double multiplier) {
  // _tsList[x].first == TimeSeries, _tsList[x].second == multipier
  // (private) std::vector< std::pair<TimeSeries::sharedPointer,double> > _tsList;
  typedef std::pair<TimeSeries::sharedPointer, double> tsDoublePair_t;
  BOOST_FOREACH(AggregatorSource& item, _tsList) {
    if (item.timeseries == timeSeries) {
      item.multiplier = multiplier;
      // todo -- reset pointrecord backing store?
    }
  }
}



Point AggregatorTimeSeries::pointBefore(time_t time) {

  std::set<time_t> timeSet;
  
  if (this->clock()->isRegular()) {
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
  
  if (this->clock()->isRegular()) {
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


Point AggregatorTimeSeries::point(time_t time) {
  return ModularTimeSeries::point(time);
}

std::vector<Point> AggregatorTimeSeries::filteredPoints(TimeSeries::sharedPointer sourceTs, time_t fromTime, time_t toTime) {
  
  vector<Point> aggregated;
  if (clock()->isRegular()) {
    // align the query with the clock
    fromTime = (clock()->isValid(fromTime)) ? fromTime : clock()->timeAfter(fromTime);
    toTime = (clock()->isValid(toTime)) ? toTime : clock()->timeBefore(toTime);
    
    // get the set of times from the clock
    aggregated.reserve(1 + (toTime-fromTime)/(clock()->period()));
    for (time_t thisTime = fromTime; thisTime <= toTime; thisTime = this->clock()->timeAfter(thisTime)) {
      Point p(thisTime, 0);
      aggregated.push_back(p);
    }
    
  }
  
  else {
    
    // get the set of times from the aggregator sources
    std::set<time_t> aggregatedTimes;
    BOOST_FOREACH(AggregatorSource aggSource, _tsList) {
      vector<Point> thisSourcePoints = aggSource.timeseries->points(fromTime, toTime);
      BOOST_FOREACH(Point p, thisSourcePoints) {
        aggregatedTimes.insert(p.time);
      }
    }
    BOOST_FOREACH(time_t t, aggregatedTimes) {
      Point p(t, 0);
      aggregated.push_back(p);
    }
  }
  
    if (aggregated.size() == 0) {
      // no points in range
      return aggregated;
    }
    
    BOOST_FOREACH(AggregatorSource aggSource , _tsList) {
      // Try to expand the point range
      pair<time_t,time_t> sourceRange; // = expandedRange(aggSource.timeseries, fromTime, toTime);
      sourceRange.first = aggSource.timeseries->pointAtOrBefore(fromTime).time;
      sourceRange.second = aggSource.timeseries->pointAfter(toTime-1).time;
      
      // check for coverage. does the upstream source contain the needed range?
      if (fromTime < sourceRange.first || sourceRange.second < toTime) {
        cerr << "Aggregation range not covered" << endl;
        
        sourceRange.second = aggSource.timeseries->pointAfter(toTime-1).time;
      }
      
      
      // get the source points
      vector<Point> upstreamPoints = aggSource.timeseries->points(sourceRange.first, sourceRange.second);
      if (upstreamPoints.size() == 0) {
        continue;
      }
      
      if (sourceRange.first != upstreamPoints.front().time || sourceRange.second != upstreamPoints.back().time) {
        cerr << "source points not what expected" << endl;
      }
      
      vector<Point>::const_iterator cursorPoint = upstreamPoints.begin();
      Point trailingPoint = *cursorPoint;
      ++cursorPoint;
      if (cursorPoint == upstreamPoints.end()) {
        // possibility that trailing point is perfectly aligned.
        if (trailingPoint.time != aggregated.front().time) {
          cerr << "not enough upstream data" << endl;
        }
      }
      
      // add in the new points.
      BOOST_FOREACH(Point& p, aggregated) {
        
        // crawl along the upstream vector, interpolating as needed.
        // as long as the current aggregation point is within the range set by the trailing point and the cursor,
        // there's no problem. otherwise, let's skip ahead.
        
        // *** this is OK:
        //   trailing <-------------------------> cursor
        //                    ^
        //                    p
        
        
        // the following block gets evaluated when we're not yet within the correct range:
        //        trailing <-------------------------> cursor
        //   ^
        //   p
        if (p.time < trailingPoint.time) {
          cerr << "aggregator skipping ahead" << endl;
          // this is error
          continue; // get to the next point.
        }
        
        
        else if (trailingPoint.time == p.time) {
          p += trailingPoint * aggSource.multiplier;
          continue;
          // you are perfect.
        }
        
        // the following block gets evaluated when we've gone outside the trailing---cursor range:
        //   trailing <-------------------------> cursor
        //                                                 ^
        //                                                 p
        else while (cursorPoint->time < p.time) {
          // move both markers forward
          trailingPoint = *cursorPoint;
          ++cursorPoint;
          if (cursorPoint == upstreamPoints.end()) {
            continue;
          }
        }
        
        // if we've gotten this far, then we're in good shape.
        Point aggregationPoint = Point::linearInterpolate(trailingPoint, *cursorPoint, p.time);
        p += aggregationPoint * aggSource.multiplier;
        
      }
    }
    
  
  
  
  return aggregated;
}



