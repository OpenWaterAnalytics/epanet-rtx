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
  this->setSummaryOnly(true);
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
  
  BOOST_FOREACH(const pointSummaryPair_t& psp, summaryCollection) {
    Point p = psp.first;
    Summary s = psp.second;
    
    double v = 0;
    
    switch (_statsType) {
      case StatsTimeSeriesMean:
        v = s.stats.mean;
        break;
      case StatsTimeSeriesStdDev:
        v = sqrt(s.stats.variance);
        break;
      case StatsTimeSeriesMedian:
        v = s.stats.quartiles.q50;
        break;
      case StatsTimeSeriesQ25:
        v = s.stats.quartiles.q25;
        break;
      case StatsTimeSeriesQ75:
        v = s.stats.quartiles.q75;
        break;
      case StatsTimeSeriesInterQuartileRange:
        v = s.stats.quartiles.q75 - s.stats.quartiles.q25;
      default:
        break;
    }
    
    Point outPoint(p.time, v);
    if (outPoint.isValid) {
      outPoints.push_back(outPoint);
    }
    
  }
  
  return outPoints;
  
}
