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

#include <future>

using namespace RTX;
using namespace std;
using PC = PointCollection;
using ST = StatsTimeSeries;

const map<StatsTimeSeries::StatsTimeSeriesType, function<double(const PC::pvRange, double)> > __getters({
  {ST::StatsTimeSeriesMean,   [](const PC::pvRange r, double pct)->double { return PC::mean(r); } },
  {ST::StatsTimeSeriesStdDev, [](const PC::pvRange r, double pct)->double { return sqrt(PC::variance(r)); } },
  {ST::StatsTimeSeriesMedian, [](const PC::pvRange r, double pct)->double { return PC::percentile(.5,r); } },
  {ST::StatsTimeSeriesQ25,    [](const PC::pvRange r, double pct)->double { return PC::percentile(.25,r); } },
  {ST::StatsTimeSeriesQ75,    [](const PC::pvRange r, double pct)->double { return PC::percentile(.75,r); } },
  {ST::StatsTimeSeriesIQR,    [](const PC::pvRange r, double pct)->double { return PC::interquartilerange(r); } },
  {ST::StatsTimeSeriesMax,    [](const PC::pvRange r, double pct)->double { return PC::max(r); } },
  {ST::StatsTimeSeriesMin,    [](const PC::pvRange r, double pct)->double { return PC::min(r); } },
  {ST::StatsTimeSeriesCount,  [](const PC::pvRange r, double pct)->double { return PC::count(r); } },
  {ST::StatsTimeSeriesVar,    [](const PC::pvRange r, double pct)->double { return PC::variance(r); } },
  {ST::StatsTimeSeriesRMS,    [](const PC::pvRange r, double pct)->double { return PC::variance(r) + pow(PC::mean(r), 2.); } },
  {ST::StatsTimeSeriesPercentile, [](const PC::pvRange r, double pct)->double { return PC::percentile(pct,r); } },
});


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


PointCollection StatsTimeSeries::filterPointsInRange(TimeRange range) {
  
  TimeRange qRange = range;
  if (this->willResample()) {
    // expand range
    qRange.start = this->source()->timeBefore(range.start + 1);
    qRange.end = this->source()->timeAfter(range.end - 1);
    qRange.correctWithRange(range);
  }
  
  set<time_t> times = this->timeValuesInRange(qRange);
  
  auto subranges = this->subRanges(times);
  vector<Point> outPoints;
  outPoints.reserve(subranges.ranges.size());
  
  map< time_t,future<double> > statsTasks;
  for(auto &x : subranges.ranges) {
    time_t t = x.first;
    PC::pvRange r = x.second;
    if (PC::count(r) == 0 && _statsType != StatsTimeSeriesCount) {
      continue;
    }
    
    auto mySource = this->source();
    Units u1 = this->statsUnits(this->source()->units(), this->statsType());
    Units u2 = this->units();
    double pct = _percentile;
    
    auto task = async(launch::deferred, [=]()->double {
      if (!mySource) {
        return 0.0;
      }
      double v = __getters.at(_statsType)(r,pct);
      return Units::convertValue(v, u1, u2);
    });
    
    statsTasks.insert( std::pair< time_t, future<double> >(t,std::move(task)) );
  }
  
  for (auto& taskPair : statsTasks) {
    time_t t = taskPair.first;
    double v = taskPair.second.get();
    Point outPoint(t,v);
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
    case StatsTimeSeriesIQR:
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
    case StatsTimeSeriesPercentile:
      return sourceUnits;
      break;
    default:
      break;
  }
  return sourceUnits;
}
