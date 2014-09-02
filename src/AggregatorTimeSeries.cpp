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
  if (this->clock()->period() < timeSeries->clock()->period()) {
    this->setClock(timeSeries->clock());
  }
  
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


Point AggregatorTimeSeries::point(time_t time) {
  // call the base class method first, to see if the point is accessible via cache.
  Point aPoint = TimeSeries::point(time);
  
  if (this->clock()->isRegular()) {
    
    if (!this->clock()->isValid(time)) {
      return Point();
    }
    
    if (!aPoint.isValid || aPoint.quality == Point::missing) {
      vector<Point> aggP = filteredPoints(source(), time, time);
      if (aggP.size() != 1) {
        cerr << "ERR: times not registered in aggregator" << endl;
        return Point();
      }
      aPoint = aggP.front();
      this->insert(aPoint);
    }
  }
  
  else {
    
    if (!aPoint.isValid || aPoint.quality == Point::missing) {
      vector<Point> aggP = filteredPoints(source(), time, time);
      if (aggP.size() != 1) {
        return Point();
      }
      aPoint = aggP.front();
      this->insert(aPoint);
    }

  }
  
  return aPoint;
}
/*
std::vector< Point > AggregatorTimeSeries::points(time_t start, time_t end) {
  typedef std::pair< TimeSeries::sharedPointer, double > tsPair_t;
  BOOST_FOREACH(tsPair_t tsPair , _tsList) {
    // run the query, so that the source time series are ready
    tsPair.first->points(start, end);
  }
  
  return TimeSeries::points(start, end);
}
*/



std::vector<Point> AggregatorTimeSeries::filteredPoints(TimeSeries::sharedPointer sourceTs, time_t fromTime, time_t toTime) {
  
  vector<Point> aggregated;
  if (clock()->isRegular()) {
    // align the query with the clock
    fromTime = (clock()->isValid(fromTime)) ? fromTime : clock()->timeAfter(fromTime);
    toTime = (clock()->isValid(toTime)) ? toTime : clock()->timeBefore(toTime);
    
    // get the set of times from the clock
    aggregated.reserve(1 + (toTime-fromTime)/(clock()->period()));
    for (time_t t = fromTime; t <= toTime; t += clock()->period()) {
      Point p(t, 0);
      aggregated.push_back(p);
    }

    BOOST_FOREACH(AggregatorSource aggSource , _tsList) {
      // resample the source if needed.
      // this also converts to local units, so we don't have to worry about that here.
      vector<Point> thisSourcePoints = Resampler::filteredPoints(aggSource.timeseries, fromTime, toTime);
      if (thisSourcePoints.size() == 0) {
        cerr << "no points found for : " << aggSource.timeseries->name() << "(" << fromTime << " - " << toTime << ")" << endl;
        continue;
      }
      vector<Point>::const_iterator pIt = thisSourcePoints.begin();
      // add in the new points.
      BOOST_FOREACH(Point& p, aggregated) {
        // just make sure we're at the right time.
        while (pIt != thisSourcePoints.end() && (*pIt).time < p.time) {
          ++pIt;
        }
        if (pIt == thisSourcePoints.end()) {
          cerr << "ERR: times not registered in aggregator" << endl;
          break;
        }
        //
        if ((*pIt).time != p.time) {
          cerr << "ERR: times not registered in aggregator" << endl;
        }
        
        // add it in.
        p += (*pIt) * aggSource.multiplier;
        if ((*pIt).quality != Point::good) {
          p.quality = (*pIt).quality;
        }
        
      }
    }
    
  }
  
  else {
    
    // get the set of times from the aggregator sources
    std::set<time_t> aggregatedTimes;
    BOOST_FOREACH(AggregatorSource aggSource, _tsList) {
      vector<Point> thisSourcePoints = ModularTimeSeries::filteredPoints(aggSource.timeseries, fromTime, toTime);
      BOOST_FOREACH(Point p, thisSourcePoints) {
        aggregatedTimes.insert(p.time);
      }
    }
    BOOST_FOREACH(time_t t, aggregatedTimes) {
      Point p(t, 0);
      aggregated.push_back(p);
    }
    
    if (aggregated.size() == 0) {
      // no points in range
      return aggregated;
    }
    
    BOOST_FOREACH(AggregatorSource aggSource , _tsList) {
      // Try to expand the point range
      pair<time_t,time_t> sourceRange = expandedRange(aggSource.timeseries, fromTime, toTime);
      // get the source points
      vector<Point> thisSourcePoints = ModularTimeSeries::filteredPoints(aggSource.timeseries, sourceRange.first, sourceRange.second);
      if (thisSourcePoints.size() == 0) {
        continue;
      }
      vector<Point>::const_iterator pIt = thisSourcePoints.begin();
      // add in the new points.
      BOOST_FOREACH(Point& p, aggregated) {
        if ((*pIt).time > p.time) {
          continue;
        }
        // position the iterator to bracket the current time
        while (pIt != thisSourcePoints.end() && (*pIt).time < p.time) {
          ++pIt;
        }
        if (pIt == thisSourcePoints.end()) {
          cerr << "ERR: times not registered in aggregator" << endl;
          break;
        }
        // construct the point, interpolate if needed
        Point aggP;
        if ((*pIt).time > p.time) {
          Point p1, p2;
          p1 = *(pIt-1);
          p2 = *pIt;
          aggP = Point::linearInterpolate(p1, p2, p.time);
        }
        else {
          aggP = *pIt;
        }
        
        // add it in.
        p += aggP * aggSource.multiplier;
        
      }
    }
    
  }
  
  
  return aggregated;
}



