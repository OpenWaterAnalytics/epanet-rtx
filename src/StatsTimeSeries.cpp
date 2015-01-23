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
  this->invalidate();
  
  if (this->source()) {
    Units newUnits = statsUnits(this->source()->units(), type);
    if (!newUnits.isSameDimensionAs(this->units())) {
      setUnits(newUnits);
    }
  }
}



TimeSeries::PointCollection StatsTimeSeries::filterPointsInRange(TimeRange range) {
  
  set<time_t> times = this->timeValuesInRange(range);
  
  vector<pointSummaryPair_t> summaries = this->filterSummaryCollection(times);
  vector<Point> outPoints;
  outPoints.reserve(summaries.size());
  
  BOOST_FOREACH(pointSummaryPair_t summary, summaries) {
    Point p = summary.first;
    PointCollection col = summary.second;
    double v = this->valueFromSummary(col);
    Point outPoint(p.time, v);
    if (outPoint.isValid) {
      outPoints.push_back(outPoint);
    }
  }
  
  return PointCollection(outPoints, this->units());
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
  return false;
}

void StatsTimeSeries::didSetSource(TimeSeries::_sp source) {
  Units originalUnits = this->units();
  this->setUnits(RTX_DIMENSIONLESS);  // non-dimensionalize so that we can accept this source.
  TimeSeriesFilter::didSetSource(source);
  if (source) {
    Units units = statsUnits(source->units(), statsType());
    
    if (!units.isSameDimensionAs(originalUnits)) {
      units = originalUnits;
    }
    this->setUnits(units);
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
      std::cerr << "units are not dimensionally consistent" << std::endl;
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
