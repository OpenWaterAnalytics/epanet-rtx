//
//  AggregatorTimeSeries.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See Readme.txt and license.txt for more information
//  https://sourceforge.net/projects/epanet-rtx/

#include <iostream>

#include "AggregatorTimeSeries.h"
#include "boost/foreach.hpp"

using namespace RTX;

void AggregatorTimeSeries::addSource(TimeSeries::sharedPointer timeSeries, double multiplier) throw(RtxException) {
  
  // check compatibility
  if (!isCompatibleWith(timeSeries)) throw IncompatibleComponent();
  
  if (units().isDimensionless() && sources().size() == 0) {
    // we have default units and no sources yet, so it would be safe to adopt the new source's units.
    this->setUnits(timeSeries->units());
  }
  
  std::pair<TimeSeries::sharedPointer,double> aggregatorItem(timeSeries, multiplier);
  _tsList.push_back(aggregatorItem);
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


Point::sharedPointer AggregatorTimeSeries::point(time_t time) {
  Point::sharedPointer aPoint;
  
  // call the base class method first, to see if the point is accessible via cache.
  aPoint = TimeSeries::point(time);

  // if not, we construct it.
  if (!aPoint || aPoint->quality()==Point::missing) {
    aPoint.reset( new Point(time, 0, Point::good) );
    // start at zero, and sum other TS's values.
    std::vector< std::pair<TimeSeries::sharedPointer,double> >::iterator it;
    typedef std::pair< TimeSeries::sharedPointer, double > tsPair_t;
    BOOST_FOREACH(tsPair_t tsPair , _tsList) {
      double multiplier;
      Point::sharedPointer thisPoint = Point::convertPoint(*tsPair.first->point(time), tsPair.first->units(), units());
      multiplier = tsPair.second;
      *aPoint += ( (*thisPoint) * multiplier );
    }
    this->insert(aPoint);
  }
  
  return aPoint;
}
