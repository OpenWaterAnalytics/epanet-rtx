//
//  CorrelatorTimeSeries.cpp
//  epanet-rtx
//
//  Open Water Analytics [wateranalytics.org]
//  See README.md and license.txt for more information
//


#include "CorrelatorTimeSeries.h"
#include "AggregatorTimeSeries.h"
#include "LagTimeSeries.h"

#include <math.h>
#include <float.h>
#include <boost/foreach.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/covariance.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/variance.hpp>
#include <boost/accumulators/statistics/variates/covariate.hpp>

using namespace boost::accumulators;
using namespace RTX;
using namespace std;


CorrelatorTimeSeries::CorrelatorTimeSeries() {
//  Clock::_sp c( new Clock(3600) );
//  _corWindow = c;
  _lagSeconds = 0;
  this->setUnits(RTX_DIMENSIONLESS);
}

bool CorrelatorTimeSeries::canSetSecondary(TimeSeries::_sp secondary) {
  if (this->source() && secondary && !this->source()->units().isSameDimensionAs(secondary->units())) {
    return false; // can't do it.
  }
  else {
    return true;
  }
}

void CorrelatorTimeSeries::didSetSecondary(TimeSeries::_sp secondary) {
  if (this->source() && secondary) {
    this->setUnits(RTX_DIMENSIONLESS);
  }
  else {
    this->setUnits(RTX_NO_UNITS);
  }
  TimeSeriesFilterSecondary::didSetSecondary(secondary);
}

Clock::_sp CorrelatorTimeSeries::correlationWindow() {
  return _corWindow;
}

void CorrelatorTimeSeries::setCorrelationWindow(Clock::_sp window) {
  _corWindow = window;
  this->invalidate();
}


int CorrelatorTimeSeries::lagSeconds() {
  return _lagSeconds;
}

void CorrelatorTimeSeries::setLagSeconds(int nSeconds) {
  _lagSeconds = abs(nSeconds);
  this->invalidate();
}



#pragma mark - superclass overrides

PointCollection CorrelatorTimeSeries::filterPointsInRange(TimeRange range) {
  
  PointCollection data(vector<Point>(), this->units());
  if (!this->secondary() || !this->source()) {
    return data;
  }
  
  // force pre-cache
  TimeRange preFetchRange(range.start - this->correlationWindow()->period(), range.end + _lagSeconds);
  TimeRange correlatorFetchRange = preFetchRange;
  // widen for inclusion of lags
  time_t correlatorPrior = correlatorFetchRange.start - this->lagSeconds();
  time_t correlatorNext = correlatorFetchRange.end + this->lagSeconds();
  // widen for resampling capabilities
  correlatorFetchRange.start = this->secondary()->timeBefore(correlatorPrior + 1);
  correlatorFetchRange.end = this->secondary()->timeAfter(correlatorNext - 1);
  correlatorFetchRange.correctWithRange(TimeRange(correlatorPrior,correlatorNext)); // get rid of zero-range
  
  PointCollection m_primaryCollection = this->source()->pointCollection(preFetchRange);
  PointCollection m_secondaryCollection = this->secondary()->pointCollection(correlatorFetchRange);
  m_secondaryCollection.convertToUnits(m_primaryCollection.units);
  
  TimeSeries::_sp sourceTs = this->source();
  time_t windowWidth = this->correlationWindow()->period();
  
  set<time_t> sampleTimes;
  if (this->clock()) {
    sampleTimes = this->clock()->timeValuesInRange(range);
  }
  else {
    sampleTimes = m_primaryCollection.trimmedToRange(range).times(); //this->timeValuesInRange(range);
  }
  vector<Point> thePoints;
  thePoints.reserve(sampleTimes.size());
  
  for(time_t t : sampleTimes) {
    double corrcoef = 0;
    TimeRange q(t-windowWidth, t);
    PointCollection sourceCollection = m_primaryCollection.trimmedToRange(q); //sourceTs->pointCollection(q);
    
    set<time_t> sourceTimeValues = sourceCollection.times();
    
    set<time_t> lagEvaluationTimes = m_primaryCollection.trimmedToRange(TimeRange(t - _lagSeconds, t + _lagSeconds)).times();
    if (lagEvaluationTimes.size() == 0) {
      continue; // next time.
    }
    
    pair<double, int> maxCorrelationAtLaggedTime(-FLT_MAX, 0);
    
    for(time_t lagTime : lagEvaluationTimes) {
      
      int timeDistance = (int)(t - lagTime);
      PointCollection sourceCollectionForAnalysis = sourceCollection;
      PointCollection secondaryCollection = m_secondaryCollection;
      // perform a time lag on the secondary points if needed.
      if (timeDistance != 0) {
        auto pv = secondaryCollection.points();
        for (Point &p : pv) {
          p.time -= timeDistance;
        }          
        secondaryCollection.setPoints(pv);
      }
      
      // resample secondary points for the correlation analysis.
      secondaryCollection.resample(sourceCollection.times());
      
      if (secondaryCollection.count() < 2) {
        continue;
      }
      // trim points on primary?
      if (secondaryCollection.count() != sourceCollectionForAnalysis.count()) {
        sourceCollectionForAnalysis.resample(secondaryCollection.times());
      }
      
      if (secondaryCollection.count() != sourceCollectionForAnalysis.count()) {
        continue; // skip this. something is wrong.
      }
      
      // do the correlation calculation.
      
      accumulator_set<double, stats<tag::mean, tag::variance> > acc1;
      accumulator_set<double, stats<tag::mean, tag::variance> > acc2;
      accumulator_set<double, stats<tag::covariance<double, tag::covariate1> > > acc3;
      auto sca = sourceCollectionForAnalysis.points();
      auto sc = secondaryCollection.points();
      for (size_t i = 0; i < sourceCollectionForAnalysis.count(); i++) {
        Point p1 = sca.at(i);
        Point p2 = sc.at(i);
        acc1(p1.value);
        acc2(p2.value);
        acc3(p1.value, covariate1 = p2.value);
      }
      corrcoef = covariance(acc3)/sqrt(variance(acc1))/sqrt(variance(acc2));
      
      if (corrcoef > maxCorrelationAtLaggedTime.first) {
        maxCorrelationAtLaggedTime.first = corrcoef;
        maxCorrelationAtLaggedTime.second = timeDistance;
      }
      
    }
    
    
    if (maxCorrelationAtLaggedTime.first > -FLT_MAX) {
      thePoints.push_back(Point(t,maxCorrelationAtLaggedTime.first, Point::opc_rtx_override, (double)(maxCorrelationAtLaggedTime.second)));
    }
    
  }
  
  return PointCollection(thePoints, RTX_DIMENSIONLESS);
}

bool CorrelatorTimeSeries::canSetSource(TimeSeries::_sp ts) {
  if (this->secondary() && !ts->units().isSameDimensionAs(this->secondary()->units())) {
    return false;
  }
  return true;
}

void CorrelatorTimeSeries::didSetSource(TimeSeries::_sp ts) {
  this->invalidate();
  if (this->source() && this->secondary()) {
    this->setUnits(RTX_DIMENSIONLESS);
  }
  else {
    this->setUnits(RTX_NO_UNITS);
  }
}

bool CorrelatorTimeSeries::canChangeToUnits(Units units) {
  if (units.isDimensionless()) {
    return true;
  }
  return false;
}

