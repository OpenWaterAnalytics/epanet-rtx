//
//  StatsTimeSeries.cpp
//  epanet-rtx
//
//  Created by Sam Hatchett on 7/18/14.
//
//

#include "StatsTimeSeries.h"
#include <boost/foreach.hpp>
#include <math.h>

using namespace RTX;
using namespace std;


StatsTimeSeries::StatsTimeSeries() {
  _statsType = StatsTimeSeriesMean;
  this->setSummaryOnly(false);
}


StatsTimeSeries::StatsTimeSeriesType StatsTimeSeries::statsType() {
  return _statsType;
}

void StatsTimeSeries::setStatsType(StatsTimeSeriesType type) {
  _statsType = type;
}

vector<Point> StatsTimeSeries::filteredPoints(TimeSeries::sharedPointer sourceTs, time_t fromTime, time_t toTime) {
  
  vector<pointSummaryPair_t> summaryCollection = this->filteredSummaryPoints(sourceTs, fromTime, toTime);
  
  vector<Point> outPoints;
  outPoints.reserve(summaryCollection.size());
  
  // use these only for regular time series access, otherwise use the source points to set the intervals.
  bool isRegular = this->clock()->isRegular();
  time_t cursor = fromTime;
  Summary tempCacheSummary;
  
  vector<pointSummaryPair_t>::const_iterator pIt = summaryCollection.begin();
  
  // pre-fetch the summary point
  if (pIt != summaryCollection.end()) {
    tempCacheSummary = pIt->second;
  }
  
  if (isRegular) {
    while (pIt != summaryCollection.end()) {
      // i'm a regular time series.
      if (pIt->first.time >= cursor) {
        // the iterator has overtaken the cursor. use the previous cached summary.
        double v = this->valueFromSummary(tempCacheSummary);
        Point outPoint(cursor, v);
        if (outPoint.isValid) {
          outPoints.push_back(outPoint);
        }
        // and increment the cursor
        cursor = this->clock()->timeAfter(cursor);
      }
      
      ++pIt;
      if (pIt != summaryCollection.end()) {
        tempCacheSummary = pIt->second;
      }
    }
  }
  else {
    // irregular time series. just pass through.
    while (pIt != summaryCollection.end()) {
      Point p = pIt->first;
      Summary s = pIt->second;
      double v = this->valueFromSummary(s);
      Point outPoint(p.time, v);
      if (outPoint.isValid) {
        outPoints.push_back(outPoint);
      }
      ++pIt;
    }
  }
  
  return outPoints;
}


double StatsTimeSeries::valueFromSummary(TimeSeries::Summary s) {
  switch (_statsType) {
    case StatsTimeSeriesMean:
      return s.stats.mean;
      break;
    case StatsTimeSeriesStdDev:
      return sqrt(s.stats.variance);
      break;
    case StatsTimeSeriesMedian:
      return s.stats.quartiles.q50;
      break;
    case StatsTimeSeriesQ25:
      return s.stats.quartiles.q25;
      break;
    case StatsTimeSeriesQ75:
      return s.stats.quartiles.q75;
      break;
    case StatsTimeSeriesInterQuartileRange:
      return s.stats.quartiles.q75 - s.stats.quartiles.q25;
      break;
    case StatsTimeSeriesMax:
      return s.stats.max;
      break;
    case StatsTimeSeriesMin:
      return s.stats.min;
      break;
    case StatsTimeSeriesCount:
      return s.stats.count;
      break;
    case StatsTimeSeriesVar:
      return s.stats.variance;
      break;
    case StatsTimeSeriesRMSE:
      return sqrt(s.stats.variance + s.stats.mean*s.stats.mean);
      break;
    default:
      break;
  }
}


