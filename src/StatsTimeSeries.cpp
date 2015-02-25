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
  _percentile = .5;
  this->setSummaryOnly(false);
}


StatsTimeSeries::StatsTimeSeriesType StatsTimeSeries::statsType() {
  return _statsType;
}

void StatsTimeSeries::setStatsType(StatsTimeSeriesType type) {
  _statsType = type;
  this->invalidate();
  
  if (this->source()) {
    Units newUnits = statsUnits(this->source()->units(), type);
    if (!newUnits.isSameDimensionAs(this->units())) {
      setUnits(newUnits);
    }
  }
}

double StatsTimeSeries::arbitraryPercentile() {
  return _percentile;
}
void StatsTimeSeries::setArbitraryPercentile(double p) {
  
  if (p > 1.) {
    p = 1.;
  }
  else if (p < 0.) {
    p = 0.;
  }
  
  _percentile = p;
  this->invalidate();
}


TimeSeries::PointCollection StatsTimeSeries::filterPointsInRange(TimeRange range) {
  
  TimeRange qRange = range;
  if (this->willResample()) {
    // expand range
    qRange.start = this->source()->pointBefore(range.start + 1).time;
    qRange.end = this->source()->pointAfter(range.end - 1).time;
  }
  
  qRange.start  = qRange.start  > 0 ? qRange.start  : range.start;
  qRange.end = qRange.end > 0 ? qRange.end : range.end;
  
  set<time_t> times = this->timeValuesInRange(qRange);
  
  vector<pointSummaryPair_t> summaries = this->filterSummaryCollection(times);
  vector<Point> outPoints;
  outPoints.reserve(summaries.size());
  
  BOOST_FOREACH(pointSummaryPair_t summary, summaries) {
    Point p = summary.first;
    PointCollection col = summary.second;
    if (col.count() == 0) {
      continue;
    }
    double v = this->valueFromSummary(col);
    Point outPoint(p.time, v);
    if (outPoint.isValid) {
      outPoints.push_back(outPoint);
    }
  }
  
  PointCollection ret(outPoints, this->units());
  
  if (this->willResample()) {
    ret.resample(times);
  }
  
  switch (statsType()) {
    case StatsTimeSeriesMean:
    case StatsTimeSeriesMedian:
      ret.addQualityFlag(Point::rtx_averaged);
      break;
    default:
      ret.addQualityFlag(Point::rtx_aggregated);
      break;
  }
  
  return ret;
}




double StatsTimeSeries::valueFromSummary(TimeSeries::PointCollection col) {
  double v;
  switch (_statsType) {
    case StatsTimeSeriesMean:
      v = col.mean();
      break;
    case StatsTimeSeriesStdDev:
      v = sqrt(col.variance());
      break;
    case StatsTimeSeriesMedian:
      v = col.percentile(.5);
      break;
    case StatsTimeSeriesQ25:
      v = col.percentile(.25);
      break;
    case StatsTimeSeriesQ75:
      v = col.percentile(.75);
      break;
    case StatsTimeSeriesInterQuartileRange:
      v = col.percentile(.75) - col.percentile(.25);
      break;
    case StatsTimeSeriesMax:
      v = col.max();
      break;
    case StatsTimeSeriesMin:
      v = col.min();
      break;
    case StatsTimeSeriesCount:
      v = col.count();
      break;
    case StatsTimeSeriesVar:
      v = col.variance();
      break;
    case StatsTimeSeriesRMS:
      v = sqrt(col.variance() + col.mean()*col.mean());
      break;
    case StatsTimeSeriesPercentile:
      v = col.percentile(_percentile);
    default:
      break;
  }
  return Units::convertValue(v, source()->units(), this->units());
}


bool StatsTimeSeries::canSetSource(TimeSeries::_sp ts) {
  if (this->units().isDimensionless()) {
    return true;
  }
  else if (this->source() && this->units().isSameDimensionAs(this->statsUnits(ts->units(), this->statsType()))) {
    return true;
  }
  else if (!this->source()) {
    return true;
  }
  return false;
}

void StatsTimeSeries::didSetSource(TimeSeries::_sp source) {
  
  if (source) {
    Units units = statsUnits(source->units(), statsType());
    if (!units.isSameDimensionAs(this->units())) {
      this->setUnits(units);
    }
  }
  
}

bool StatsTimeSeries::canChangeToUnits(Units newUnits) {
  
  // only set the units if there is no source or the source's units are dimensionally consistent with the passed-in units.
  if (!this->source()) {
    return true;
  }
  else {
    Units units = statsUnits(source()->units(), statsType());
    if (units.isSameDimensionAs(newUnits)) {
      return true;
    }
    else {
      return false;
    }
  }
  return false;
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
