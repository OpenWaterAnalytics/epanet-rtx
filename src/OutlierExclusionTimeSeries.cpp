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
  this->invalidate();
}
OutlierExclusionTimeSeries::exclusion_mode_t OutlierExclusionTimeSeries::exclusionMode() {
  return _exclusionMode;
}


void OutlierExclusionTimeSeries::setOutlierMultiplier(double multiplier) {
  _outlierMultiplier = multiplier;
  this->invalidate();
}
double OutlierExclusionTimeSeries::outlierMultiplier() {
  return _outlierMultiplier;
}

bool OutlierExclusionTimeSeries::willResample() {
  return TimeSeriesFilter::willResample() || ( this->clock() );
}

bool OutlierExclusionTimeSeries::canDropPoints() {
  return true;
}

PointCollection OutlierExclusionTimeSeries::filterPointsInRange(TimeRange range) {
  
  // if we are to resample, then there's a possibility that we need to expand the range
  // used to query the source ts. but we have to limit the search to something reasonable, in case
  // too many points are excluded. yikes!
  TimeRange sourceQuery = range;
  if (this->willResample()) {
    sourceQuery = this->expandedRange(range);
  }

  // get raw values, exclude outliers, then resample if needed.
  PointCollection raw = this->source()->pointCollection(sourceQuery);
  set<time_t> rawTimes = raw.times();
  
  set<time_t> proposedOutTimes; // = this->timeValuesInRange(range); // can't do this because recursion.
  if (this->clock()) {
    proposedOutTimes = this->clock()->timeValuesInRange(range);
  }
  else {
    PointCollection trim = raw.trimmedToRange(range);
    proposedOutTimes = trim.times();
  }
  
  auto subranges = this->subRanges(rawTimes);
  TimeSeries::_sp sourceTs = this->source();
  
  
  vector<Point> goodPoints;
  
  
  goodPoints.reserve(subranges.ranges.size());
  
  // we have to re-map the points to the summaries that surround the points
  BOOST_FOREACH(const Point& p, raw.points()) {
    // find the summary corresponding to this point's time
    if (subranges.ranges.find(p.time) != subranges.ranges.end()) {
      auto range = subranges.ranges[p.time];
      Point summaryPoint = this->pointWithSampleAndPoint(range, p);
      if (summaryPoint.isValid) {
        goodPoints.push_back(summaryPoint);
      }
    }
  }// end for each raw point
  
  PointCollection outCollection(goodPoints, this->units());
  if (this->willResample()) {
    outCollection.resample(proposedOutTimes);
  }
  
  return outCollection;
  
}



Point OutlierExclusionTimeSeries::pointWithSampleAndPoint(PointCollection::pvRange sample, Point p) {
  using PC = PointCollection;
  
  Point pOut;
  double q25,q75,iqr,mean,stddev;
  const double m = this->outlierMultiplier();
  
  switch (this->exclusionMode()) {
    case OutlierExclusionModeInterquartileRange:
    {
      q25 = PC::percentile(.25, sample);
      q75 = PC::percentile(.75, sample);
      iqr = q75 - q25;
      if ( !( (p.value < q25 - m*iqr) || (m*iqr + q75 < p.value) )) {
        // store the point if it's within bounds
        pOut = Point::convertPoint(p, this->source()->units(), this->units());
      }
    }
      break; // OutlierExclusionModeInterquartileRange
    case OutlierExclusionModeStdDeviation:
    {
      mean = PC::mean(sample);
      stddev = sqrt(PC::variance(sample));
      if ( fabs(mean - p.value) <= (m * stddev) ) {
        pOut = Point::convertPoint(p, this->source()->units(), this->units());
      }
    }
      break; // OutlierExclusionModeStdDeviation
  } // end switch mode
  
  return pOut;
}






