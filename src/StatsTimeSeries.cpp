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
  Units originalUnits = this->units();
  if (source()) {
    Units newUnits = statsUnits(source()->units(), type);
    if (!newUnits.isSameDimensionAs(originalUnits)) {
      setUnits(newUnits);
    }
  }
}

vector<Point> StatsTimeSeries::filteredPoints(TimeSeries::sharedPointer sourceTs, time_t fromTime, time_t toTime) {
  
  vector<pointSummaryPair_t> summaryCollection = this->filteredSummaryPoints(sourceTs, fromTime, toTime);
  
  vector<Point> outPoints;
  outPoints.reserve(summaryCollection.size());
  
  // use these only for regular time series access, otherwise use the source points to set the intervals.
  bool isRegular = this->clock()->isRegular();
  time_t cursor = fromTime;
  Stats tempCacheSummary;
  
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
      Stats s = pIt->second;
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


double StatsTimeSeries::valueFromSummary(TimeSeries::Stats s) {
  double v;
  switch (_statsType) {
    case StatsTimeSeriesMean:
      v = s.mean;
      break;
    case StatsTimeSeriesStdDev:
      v = sqrt(s.variance);
      break;
    case StatsTimeSeriesMedian:
      v = s.quartiles.q50;
      break;
    case StatsTimeSeriesQ25:
      v = s.quartiles.q25;
      break;
    case StatsTimeSeriesQ75:
      v = s.quartiles.q75;
      break;
    case StatsTimeSeriesInterQuartileRange:
      v = s.quartiles.q75 - s.quartiles.q25;
      break;
    case StatsTimeSeriesMax:
      v = s.max;
      break;
    case StatsTimeSeriesMin:
      v = s.min;
      break;
    case StatsTimeSeriesCount:
      v = s.count;
      break;
    case StatsTimeSeriesVar:
      v = s.variance;
      break;
    case StatsTimeSeriesRMS:
      v = sqrt(s.variance + s.mean*s.mean);
      break;      
    default:
      break;
  }
  return Units::convertValue(v, source()->units(), this->units());
}

void StatsTimeSeries::setSource(TimeSeries::sharedPointer source) {
  Units originalUnits = this->units();
  this->setUnits(RTX_DIMENSIONLESS);  // non-dimensionalize so that we can accept this source.
  ModularTimeSeries::setSource(source);
  if (source) {
    Units units = statsUnits(source->units(), statsType());
    
    if (units.isSameDimensionAs(originalUnits)) {
      units = originalUnits;
    }
    this->setUnits(units);
  }

}

void StatsTimeSeries::setUnits(Units newUnits) {
  
  // only set the units if there is no source or the source's units are dimensionally consistent with the passed-in units.
  if (!source()) {
    // just use the base-est class method for this, since we don't really care
    // if the new units are the same as the source units.
    TimeSeries::setUnits(newUnits);
  }
  else {
    Units units = statsUnits(source()->units(), statsType());
    if (units.isSameDimensionAs(newUnits)) {
      TimeSeries::setUnits(newUnits);
    }
    else {
      std::cerr << "units are not dimensionally consistent" << std::endl;
    }
  }
}

Units StatsTimeSeries::statsUnits(Units sourceUnits, StatsTimeSeriesType type) {
  switch (type) {
    case StatsTimeSeriesMean:
      return sourceUnits;
      break;
    case StatsTimeSeriesStdDev:
      return sourceUnits;
      break;
    case StatsTimeSeriesMedian:
      return sourceUnits;
      break;
    case StatsTimeSeriesQ25:
      return sourceUnits;
      break;
    case StatsTimeSeriesQ75:
      return sourceUnits;
      break;
    case StatsTimeSeriesInterQuartileRange:
      return sourceUnits;
      break;
    case StatsTimeSeriesMax:
      return sourceUnits;
      break;
    case StatsTimeSeriesMin:
      return sourceUnits;
      break;
    case StatsTimeSeriesCount:
      return RTX_DIMENSIONLESS;
      break;
    case StatsTimeSeriesVar:
      return sourceUnits * sourceUnits;
      break;
    case StatsTimeSeriesRMS:
      return sourceUnits;
      break;
    default:
      break;
  }
}