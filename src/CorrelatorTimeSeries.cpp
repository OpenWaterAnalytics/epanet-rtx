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
  Clock::_sp c( new Clock(3600) );
  _corWindow = c;
  _lagSeconds = 0;
}



TimeSeries::_sp CorrelatorTimeSeries::correlatorTimeSeries() {
  return _secondary;
}

void CorrelatorTimeSeries::setCorrelatorTimeSeries(TimeSeries::_sp ts) {
  
  if (this->source() && ts && !this->source()->units().isSameDimensionAs(ts->units())) {
    return; // can't do it.
  }
  
  _secondary = ts;
  
  if (this->source() && ts) {
    this->setUnits(RTX_DIMENSIONLESS);
  }
  else {
    this->setUnits(RTX_NO_UNITS);
  }
  
  this->invalidate();
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

TimeSeries::PointCollection CorrelatorTimeSeries::filterPointsInRange(TimeRange range) {
  
  PointCollection data(vector<Point>(), this->units());
  if (!this->correlatorTimeSeries() || !this->source()) {
    return data;
  }
  
  // force pre-cache
  TimeRange preFetchRange(range.start - this->correlationWindow()->period(), range.end);
  TimeRange correlatorFetchRange = preFetchRange;
  // widen for inclusion of lags
  time_t correlatorPrior = correlatorFetchRange.start - this->lagSeconds();
  time_t correlatorNext = correlatorFetchRange.end + this->lagSeconds();
  // widen for resampling capabilities
  correlatorFetchRange.start = this->correlatorTimeSeries()->timeBefore(correlatorPrior + 1);
  correlatorFetchRange.end = this->correlatorTimeSeries()->timeAfter(correlatorNext - 1);
  correlatorFetchRange.correctWithRange(TimeRange(correlatorPrior,correlatorNext)); // get rid of zero-range
  
  PointCollection m_primaryCollection = this->source()->pointCollection(preFetchRange);
  PointCollection m_secondaryCollection = this->correlatorTimeSeries()->pointCollection(correlatorFetchRange);
  m_secondaryCollection.convertToUnits(m_primaryCollection.units);
  
  TimeSeries::_sp sourceTs = this->source();
  time_t windowWidth = this->correlationWindow()->period();
  
  set<time_t> times = m_primaryCollection.trimmedToRange(range).times(); //this->timeValuesInRange(range);
  if (this->clock()) {
    times = this->clock()->timeValuesInRange(range);
  }
  vector<Point> thePoints;
  thePoints.reserve(times.size());
  
  BOOST_FOREACH(time_t t, times) {
    double corrcoef = 0;
    TimeRange q(t-windowWidth, t);
    PointCollection sourceCollection = m_primaryCollection.trimmedToRange(q); //sourceTs->pointCollection(q);
    
    set<time_t> sourceTimeValues = sourceCollection.times();
    
    set<time_t> lagEvaluationTimes = sourceCollection.trimmedToRange(TimeRange(t - _lagSeconds, t + _lagSeconds)).times();
    if (lagEvaluationTimes.size() == 0) {
      continue; // next time.
    }
    
    pair<double, int> maxCorrelationAtLaggedTime(-MAXFLOAT,0);
    
    BOOST_FOREACH(time_t lagTime, lagEvaluationTimes) {
      
      int timeDistance = (int)(t - lagTime);
      PointCollection sourceCollectionForAnalysis = sourceCollection;
      PointCollection secondaryCollection = m_secondaryCollection;
      // perform a time lag on the secondary points if needed.
      if (timeDistance != 0) {
        BOOST_FOREACH(Point& p, secondaryCollection.points) {
          p.time -= timeDistance;
        }
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
      for (int i = 0; i < sourceCollectionForAnalysis.count(); i++) {
        Point p1 = sourceCollectionForAnalysis.points.at(i);
        Point p2 = secondaryCollection.points.at(i);
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
    
    
    
    thePoints.push_back(Point(t,maxCorrelationAtLaggedTime.first, Point::opc_rtx_override, (double)(maxCorrelationAtLaggedTime.second)));
    
  }
  
  return PointCollection(thePoints, RTX_DIMENSIONLESS);
}

bool CorrelatorTimeSeries::canSetSource(TimeSeries::_sp ts) {
  if (this->correlatorTimeSeries() && !ts->units().isSameDimensionAs(this->correlatorTimeSeries()->units())) {
    return false;
  }
  return true;
}

void CorrelatorTimeSeries::didSetSource(TimeSeries::_sp ts) {
  this->invalidate();
}

bool CorrelatorTimeSeries::canChangeToUnits(Units units) {
  if (units.isDimensionless()) {
    return true;
  }
  return false;
}

