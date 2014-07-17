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
  vector<Point> sourcePoints = sourceTs->points(fromTime, toTime);
  
  switch (this->exclusionMode()) {
    case OutlierExclusionModeInterquartileRange:
    {
      vector<QuartilePoint> qPoints = this->filteredQuartilePoints(sourceTs, fromTime, toTime);
      goodPoints.reserve(qPoints.size());
      vector<Point>::const_iterator pIt = sourcePoints.begin();
      
      BOOST_FOREACH(const QuartilePoint& qp, qPoints) {
        // get the corresponding point from the sources. fast forward if needed.
        while (pIt != goodPoints.end() && pIt->time < qp.time) {
          ++pIt;
        }
        
        // final check. times should be fully registered
        if (pIt->time != qp.time) {
          cerr << "times not registered for IQR filtering" << endl;
          continue;
        }
        
        // check iqr
        double iqr = qp.q75 - qp.q25;
        if (/* NOT */!( (pIt->value < qp.q25 - m*iqr)/* less than iqr mult */ || (m*iqr + qp.q75 < pIt->value)/* or greater than iqr mult */ )) {
          // store the point if it's within bounds
          goodPoints.push_back(Point::convertPoint(*pIt, sourceTs->units(), this->units()));
        }
      }
      // all done with IQR
    }
      break;
    case OutlierExclusionModeStdDeviation:
    {
      
      time_t windowLen = this->window()->period();
      
      // force a pre-cache on the source time series
      sourceTs->points(fromTime - windowLen, toTime);
      
      // TimeSeries::Summary already computes stats, so just use that info.
      goodPoints.reserve(sourcePoints.size());
      
      BOOST_FOREACH(const Point& p, sourcePoints) {
        TimeSeries::Summary s = sourceTs->summary( p.time - windowLen, p.time );
        double stdDev = sqrt(s.stats.variance);
        double mean = s.stats.mean;
        if ( fabs(mean - p.value) <= (m * stdDev) ) {
          goodPoints.push_back(Point::convertPoint(p, sourceTs->units(), this->units()));
        }
        
      }
      
    }
      break;
    default:
      break;
      
  } // end switch
  
  
  return goodPoints;
}









