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

bool OutlierExclusionTimeSeries::willResample() {
  return TimeSeriesFilter::willResample() || ( this->clock() );
}

//Point OutlierExclusionTimeSeries::pointBefore(time_t searchTime) {
//  Point priorPoint;
//  if (!this->source()) {
//    return priorPoint;
//  }
//  
//  time_t priorTime = this->source()->pointBefore(searchTime).time;
//  if (priorTime == 0) {
//    return priorPoint;
//  }
//  
//  priorPoint = this->filteredSingle(priorTime);
//  while (!priorPoint.isValid && priorPoint.time > 0) {
//    priorTime = this->source()->pointBefore(priorTime).time;
//    priorPoint = this->filteredSingle(priorTime);
//  }
//  
//  return priorPoint;
//}
//
//Point OutlierExclusionTimeSeries::pointAfter(time_t time) {
//  if (!this->source()) {
//    return Point();
//  }
//  Point thePoint;
//  time_t searchTime = time;
//  thePoint.time = searchTime;
//  while (!thePoint.isValid && thePoint.time > 0) {
//    searchTime = this->source()->pointAfter(thePoint.time).time;
//    thePoint = this->point
//  }
//  
//  return Point();
//  
//  time_t afterTime = this->source()->pointAfter(searchTime).time;
//  if (afterTime == 0) {
//    return Point();
//  }
//  
//  Point afterPoint = this->filteredSingle(afterTime);
//  while (!afterPoint.isValid && afterPoint.time > 0) {
//    afterTime = this->source()->pointAfter(afterTime).time;
//    afterPoint = this->filteredSingle(afterTime);
//  }
//  return afterPoint;
//}


TimeSeries::PointCollection OutlierExclusionTimeSeries::filterPointsInRange(TimeRange range) {
  
  // if we are to resample, then there's a possibility that we need to expand the range
  // used to query the source ts. but we have to limit the search to something reasonable, in case
  // too many points are excluded. yikes!
  TimeRange sourceQuery = range;
  if (this->willResample() && range.duration() > 0) {
    // expand range so that we can resample at the start and/or end of the range requested
    // this is quick and dirty, and probably will fail for badly behaved data.
    
    sourceQuery.start -= range.duration();
    sourceQuery.end += range.duration();
    
  }

  
  
  
  // get raw values, exclude outliers, then resample if needed.
  vector<Point> rawSourcePoints = this->source()->points(sourceQuery);
  set<time_t> rawTimes;
  BOOST_FOREACH(const Point& p, rawSourcePoints) {
    rawTimes.insert(p.time);
  }
  
  set<time_t> outTimes = this->timeValuesInRange(range); // output times. some of these may be excluded.
  
  vector<pointSummaryPair_t> summaries = this->filterSummaryCollection(rawTimes);
  TimeSeries::_sp sourceTs = this->source();
  
  
  vector<Point> goodPoints;
  
  
  goodPoints.reserve(summaries.size());
  
  BOOST_FOREACH(const pointSummaryPair_t& psp, summaries) {
    
    // fetch pair values from the source summary collection
    Point p = psp.first;
    PointCollection col = psp.second;
    Point summaryPoint = this->pointWithCollectionAndPoint(col, p);
    
    if (summaryPoint.isValid) {
      goodPoints.push_back(summaryPoint);
    }
    
  }// end for each summary
  
  PointCollection outCollection(goodPoints, this->units());
  if (this->willResample()) {
    outCollection.resample(outTimes);
  }
  
  return outCollection;
  
}



Point OutlierExclusionTimeSeries::pointWithCollectionAndPoint(RTX::TimeSeries::PointCollection col, Point p) {
  Point pOut;
  double q25,q75,iqr,mean,stddev;
  double m = this->outlierMultiplier();
  
  switch (this->exclusionMode()) {
    case OutlierExclusionModeInterquartileRange:
    {
      q25 = col.percentile(.25);
      q75 = col.percentile(.75);
      iqr = q75 - q25;
      if ( !( (p.value < q25 - m*iqr) || (m*iqr + q75 < p.value) )) {
        // store the point if it's within bounds
        pOut = Point::convertPoint(p, this->source()->units(), this->units());
      }
    }
      break; // OutlierExclusionModeInterquartileRange
    case OutlierExclusionModeStdDeviation:
    {
      mean = col.mean();
      stddev = sqrt(col.variance());
      if ( fabs(mean - p.value) <= (m * stddev) ) {
        pOut = Point::convertPoint(p, this->source()->units(), this->units());
      }
    }
      break; // OutlierExclusionModeStdDeviation
  } // end switch mode
  
  return pOut;
  
}






