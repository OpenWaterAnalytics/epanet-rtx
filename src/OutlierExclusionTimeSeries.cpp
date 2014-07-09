//
//  OutlierExclusionTimeSeries.cpp
//  epanet-rtx
//
//  Created by Sam Hatchett on 7/7/14.
//
//

#include "OutlierExclusionTimeSeries.h"
#include <boost/foreach.hpp>

static size_t outlierAccumulatorCacheSize = 10000;

using namespace RTX;
using namespace boost::accumulators;
using namespace std;

OutlierExclusionTimeSeries::OutlierExclusionTimeSeries() {
  _exclusionMode = OutlierExclusionModeStdDeviation;
  Clock::sharedPointer window(new Clock(60));
  _window = window;
  _outlierMultiplier = 1.;
}


void OutlierExclusionTimeSeries::setClock(Clock::sharedPointer clock) {
  // not allowed
  return;
}


void OutlierExclusionTimeSeries::setExclusionMode(exclusion_mode_t mode) {
  _exclusionMode = mode;
  this->record()->invalidate(this->name());
}

OutlierExclusionTimeSeries::exclusion_mode_t OutlierExclusionTimeSeries::exclusionMode() {
  return _exclusionMode;
}


void OutlierExclusionTimeSeries::setWindow(Clock::sharedPointer window) {
  _window = window;
  this->record()->invalidate(this->name());
}

Clock::sharedPointer OutlierExclusionTimeSeries::window() {
  return _window;
}


void OutlierExclusionTimeSeries::setOutlierMultiplier(double multiplier) {
  _outlierMultiplier = multiplier;
  this->record()->invalidate(this->name());
}

double OutlierExclusionTimeSeries::outlierMultiplier() {
  return _outlierMultiplier;
}


vector<Point> OutlierExclusionTimeSeries::filteredPoints(TimeSeries::sharedPointer sourceTs, time_t fromTime, time_t toTime) {
  
  time_t rearCursor = fromTime - this->window()->period();
  vector<Point> sourcePoints = sourceTs->points(rearCursor, toTime);
  vector<Point> filtered;
  filtered.reserve(sourcePoints.size());
  
  // use a double-ended queue for faster searching.
  deque<Point> pointQueue;
  
  // load up the pointQueue with prior points
  time_t cursorTime = rearCursor;
  vector<Point>::iterator pIt = sourcePoints.begin();
  while (pIt != sourcePoints.end() && (*pIt).time < fromTime) {
    pointQueue.push_back(*pIt);
    ++pIt;
  }
  
  
  while (pIt != sourcePoints.end()) {
    Point p = *pIt;
    // include the point?
    if (this->shouldIncludePointOnBasis(p, pointQueue)) {
      Point newPoint = Point::convertPoint(p, sourceTs->units(), this->units());
      filtered.push_back(newPoint);
    }
    
    // update the rear cursor and trim any old points from the front of the deque
    rearCursor = p.time - this->window()->period();
    while (pointQueue.size() > 0 && pointQueue.front().time < rearCursor) {
      pointQueue.pop_front();
    }
    
    // add this point to the queue
    pointQueue.push_back(p);
    // update front cursor (the iterator)
    ++pIt;
    // check for the end of the vector
    if (pIt == sourcePoints.end()) {
      break;
    }
    
  }
  
  return filtered;
}



bool OutlierExclusionTimeSeries::shouldIncludePointOnBasis(const RTX::Point &p, const std::deque<Point> basis) {
  
  bool isOk = false;
  double m = this->outlierMultiplier();
  
  switch (_exclusionMode) {
    case OutlierExclusionModeInterquartileRange:
    {
      accumulator_t_right upperQuant( tag::tail<boost::accumulators::right>::cache_size = outlierAccumulatorCacheSize );
      accumulator_t_left  lowerQuant( tag::tail<boost::accumulators::left>::cache_size = outlierAccumulatorCacheSize );
      BOOST_FOREACH(const Point& p, basis) {
        upperQuant(p.value);
        lowerQuant(p.value);
      }
      double quart25 = quantile(lowerQuant, quantile_probability = 0.25);
      double quart75 = quantile(upperQuant, quantile_probability = 0.75);
      double iqr = quart75 - quart25;
      if (!( (p.value < quart25 - m*iqr) || (m*iqr + quart75 < p.value) )) {
        isOk = true;
      }
      break;
    }
    case OutlierExclusionModeStdDeviation:
    {
      accumulator_t_mean_var varAccum;
      BOOST_FOREACH(const Point& p, basis) {
        varAccum(p.value);
      }
      double stDev = sqrt( boost::accumulators::variance(varAccum) );
      double mean = boost::accumulators::mean(varAccum);
      if ( fabs(mean - p.value) <= (m * stDev) ) {
        isOk = true;
      }
      break;
    }
    default:
      break;
  }
  
  return isOk;
}









