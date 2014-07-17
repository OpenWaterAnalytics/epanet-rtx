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


vector<Point> OutlierExclusionTimeSeries::filteredPoints(TimeSeries::sharedPointer sourceTs, time_t fromTime, time_t toTime) {
  
  double m = this->outlierMultiplier();
  vector<Point> goodPoints;
  double q25,q75,iqr,mean,stddev;
  
  vector< pointSummaryPair_t > summaryCollection = this->filteredSummaryPoints(sourceTs, fromTime, toTime);
  goodPoints.reserve(summaryCollection.size());
  
  BOOST_FOREACH(const pointSummaryPair_t& psp, summaryCollection) {
    
    // fetch pair values from the source summary collection
    Point p = psp.first;
    Summary s = psp.second;

    switch (this->exclusionMode()) {
      case OutlierExclusionModeInterquartileRange:
      {
        q25 = s.stats.quartiles.q25;
        q75 = s.stats.quartiles.q75;
        iqr = q75 - q25;
        if ( !( (p.value < q25 - m*iqr) || (m*iqr + q75 < p.value) )) {
          // store the point if it's within bounds
          goodPoints.push_back(Point::convertPoint(p, sourceTs->units(), this->units()));
        }
      }
        break; // OutlierExclusionModeInterquartileRange
      case OutlierExclusionModeStdDeviation:
      {
        mean = s.stats.mean;
        stddev = sqrt(s.stats.variance);
        if ( fabs(mean - p.value) <= (m * stddev) ) {
          goodPoints.push_back(Point::convertPoint(p, sourceTs->units(), this->units()));
        }
      }
        break; // OutlierExclusionModeStdDeviation
    } // end switch mode
  }// end for each summary
  
  return goodPoints;
}









