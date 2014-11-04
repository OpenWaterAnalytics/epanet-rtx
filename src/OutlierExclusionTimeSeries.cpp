//
//  OutlierExclusionTimeSeries.cpp
//  epanet-rtx
//
//  Created by Sam Hatchett on 7/7/14.
//
//

#include "OutlierExclusionTimeSeries.h"
#include <boost/foreach.hpp>
#include <math.h>

static size_t outlierAccumulatorCacheSize = 10000;

using namespace RTX;
using namespace std;

OutlierExclusionTimeSeries::OutlierExclusionTimeSeries() {
  _exclusionMode = OutlierExclusionModeStdDeviation;
  _outlierMultiplier = 1.;
  this->setSummaryOnly(true);
}


void OutlierExclusionTimeSeries::setExclusionMode(exclusion_mode_t mode) {
  _exclusionMode = mode;
  this->record()->invalidate(this->name());
}
OutlierExclusionTimeSeries::exclusion_mode_t OutlierExclusionTimeSeries::exclusionMode() {
  return _exclusionMode;
}


void OutlierExclusionTimeSeries::setOutlierMultiplier(double multiplier) {
  _outlierMultiplier = multiplier;
  this->record()->invalidate(this->name());
}
double OutlierExclusionTimeSeries::outlierMultiplier() {
  return _outlierMultiplier;
}

Point OutlierExclusionTimeSeries::pointBefore(time_t searchTime) {
  Point priorPoint;
  if (!this->source()) {
    return priorPoint;
  }
  
  time_t priorTime = this->source()->pointBefore(searchTime).time;
  if (priorTime == 0) {
    return priorPoint;
  }
  
  priorPoint = this->filteredSingle(priorTime);
  while (!priorPoint.isValid && priorPoint.time > 0) {
    priorTime = this->source()->pointBefore(priorTime).time;
    priorPoint = this->filteredSingle(priorTime);
  }
  
  return priorPoint;
}

Point OutlierExclusionTimeSeries::pointAfter(time_t searchTime) {
  if (!this->source()) {
    return Point();
  }
  time_t afterTime = this->source()->pointAfter(searchTime).time;
  if (afterTime == 0) {
    return Point();
  }
  
  Point afterPoint = this->filteredSingle(afterTime);
  while (!afterPoint.isValid && afterPoint.time > 0) {
    afterTime = this->source()->pointAfter(afterTime).time;
    afterPoint = this->filteredSingle(afterTime);
  }
  return afterPoint;
}


Point OutlierExclusionTimeSeries::filteredSingle(time_t time) {
  
  vector<Point> candidates = this->filteredPointsWithMissing(this->source(), time, time, true);
  if (candidates.size() == 0) {
    candidates.push_back(Point());
  }
  
  return candidates.front();
}


vector<Point> OutlierExclusionTimeSeries::filteredPointsWithMissing(TimeSeries::sharedPointer sourceTs, time_t fromTime, time_t toTime, bool returnMissing) {
  
  double m = this->outlierMultiplier();
  vector<Point> goodPoints;
  double q25,q75,iqr,mean,stddev;
  
  vector< pointSummaryPair_t > summaryCollection = this->filteredSummaryPoints(sourceTs, fromTime, toTime);
  goodPoints.reserve(summaryCollection.size());
  
  BOOST_FOREACH(const pointSummaryPair_t& psp, summaryCollection) {
    
    // fetch pair values from the source summary collection
    Point p = psp.first;
    Stats s = psp.second;
    
    switch (this->exclusionMode()) {
      case OutlierExclusionModeInterquartileRange:
      {
        q25 = s.quartiles.q25;
        q75 = s.quartiles.q75;
        iqr = q75 - q25;
        if ( !( (p.value < q25 - m*iqr) || (m*iqr + q75 < p.value) )) {
          // store the point if it's within bounds
          goodPoints.push_back(Point::convertPoint(p, sourceTs->units(), this->units()));
        }
        else if (returnMissing) {
          goodPoints.push_back(Point(p.time, 0, Point::missing)); // bad point placeholder for searching
        }
      }
        break; // OutlierExclusionModeInterquartileRange
      case OutlierExclusionModeStdDeviation:
      {
        mean = s.mean;
        stddev = sqrt(s.variance);
        if ( fabs(mean - p.value) <= (m * stddev) ) {
          goodPoints.push_back(Point::convertPoint(p, sourceTs->units(), this->units()));
        }
        else if (returnMissing) {
          goodPoints.push_back(Point(p.time, 0, Point::missing)); // bad point placeholder for searching
        }
      }
        break; // OutlierExclusionModeStdDeviation
    } // end switch mode
  }// end for each summary
  
  return goodPoints;
  
}


vector<Point> OutlierExclusionTimeSeries::filteredPoints(TimeSeries::sharedPointer sourceTs, time_t fromTime, time_t toTime) {
  
  return this->filteredPointsWithMissing(sourceTs, fromTime, toTime, false);
  
}









