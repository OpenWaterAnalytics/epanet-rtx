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
  vector< pair<TimeSeries::sharedPointer,double> > sourceVec = this->sources();
  if (sourceVec.size() > 0) {
    return sourceVec.front().first;
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
  BOOST_FOREACH(const tsMultPair_t& tsmult, _tsList) {
    double multiplier = tsmult.second;
    TimeSeries::sharedPointer ts = tsmult.first;
    string dir = (multiplier > 0)? "(+)" : "(-)";
    stream << "    " << dir << " " << ts->name() << endl;
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
  
  std::pair<TimeSeries::sharedPointer,double> aggregatorItem(timeSeries, multiplier);
  _tsList.push_back(aggregatorItem);
  
  // set my clock to the lesser-period of any source.
  if (this->clock()->period() < timeSeries->clock()->period()) {
    this->setClock(timeSeries->clock());
  }
  
}

void AggregatorTimeSeries::removeSource(TimeSeries::sharedPointer timeSeries) {
  
  typedef std::pair<TimeSeries::sharedPointer,double> tsDoublePairType;
  std::vector< tsDoublePairType > newSourceList;
  
  // find the index of the requested time series
  BOOST_FOREACH(tsDoublePairType ts, _tsList) {
    if (timeSeries == ts.first) {
      // don't copy
    }
    else {
      newSourceList.push_back(ts);
    }
  }
  // save the new source list
  _tsList = newSourceList;
}

std::vector< std::pair<TimeSeries::sharedPointer,double> > AggregatorTimeSeries::sources() {
  return _tsList;
}

void AggregatorTimeSeries::setMultiplierForSource(TimeSeries::sharedPointer timeSeries, double multiplier) {
  // _tsList[x].first == TimeSeries, _tsList[x].second == multipier
  // (private) std::vector< std::pair<TimeSeries::sharedPointer,double> > _tsList;
  typedef std::pair<TimeSeries::sharedPointer, double> tsDoublePair_t;
  BOOST_FOREACH(tsDoublePair_t& item, _tsList) {
    TimeSeries::sharedPointer ts = item.first;
    if (ts == timeSeries) {
      item.second = multiplier;
      // todo -- reset pointrecord backing store?
    }
  }
}


Point AggregatorTimeSeries::point(time_t time) {
  // call the base class method first, to see if the point is accessible via cache.
  Point aPoint = TimeSeries::point(time);
  
  if (!this->clock()->isValid(time)) {
    return Point();
  }
  
  // if not, we construct it.
  if (!aPoint.isValid || aPoint.quality == Point::missing) {
    aPoint = Point(time, 0, Point::good);
    // start at zero, and sum other TS's values.
    //std::vector< std::pair<TimeSeries::sharedPointer,double> >::iterator it;
    typedef std::pair< TimeSeries::sharedPointer, double > tsPair_t;
    BOOST_FOREACH(tsPair_t tsPair , _tsList) {
      double multiplier;
      Point sourcePoint = tsPair.first->point(time);
      Units sourceUnits = tsPair.first->units();
      Units myUnits = units();
      Point thisPoint = Point::convertPoint(sourcePoint, sourceUnits, myUnits);
      multiplier = tsPair.second;
      aPoint += ( thisPoint * multiplier );
    }
    this->insert(aPoint);
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
  
  // align the query with the clock
  fromTime = (clock()->isValid(fromTime)) ? fromTime : clock()->timeAfter(fromTime);
  toTime = (clock()->isValid(toTime)) ? toTime : clock()->timeBefore(toTime);
  
  vector<Point> aggregated;
  if (clock()->isRegular()) {
    aggregated.reserve((toTime-fromTime)/(clock()->period()));
    for (time_t t = fromTime; t <= toTime; t += clock()->period()) {
      Point p(t, 0);
      aggregated.push_back(p);
    }
  }
  else {
    cerr << "Aggregator must have a regular clock" << endl;
  }
  
  typedef std::pair< TimeSeries::sharedPointer, double > tsPair_t;
  BOOST_FOREACH(tsPair_t tsPair , _tsList) {
    double multiplier = tsPair.second;
    TimeSeries::sharedPointer ts = tsPair.first;
    
    // resample the source if needed.
    // this also converts to local units, so we don't have to worry about that here.
    vector<Point> thisSourcePoints = Resampler::filteredPoints(ts, fromTime, toTime);
    if (thisSourcePoints.size() == 0) {
      cerr << "no points found for : " << ts->name() << "(" << fromTime << " - " << toTime << ")" << endl;
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
        break;
      }
      //
      if ((*pIt).time != p.time) {
        cerr << "ERR: times not registered in aggregator" << endl;
      }
      
      // add it in.
      p += (*pIt) * multiplier;
      if ((*pIt).quality != Point::good) {
        p.quality = (*pIt).quality;
      }
      
    }
  }
  
  return aggregated;
}



