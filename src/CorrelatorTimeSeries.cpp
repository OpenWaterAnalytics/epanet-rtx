//
//  CorrelatorTimeSeries.cpp
//  epanet-rtx
//
//  Open Water Analytics [wateranalytics.org]
//  See README.md and license.txt for more information
//


#include "CorrelatorTimeSeries.h"

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
}



TimeSeries::_sp CorrelatorTimeSeries::correlatorTimeSeries() {
  return _secondary;
}

void CorrelatorTimeSeries::setCorrelatorTimeSeries(TimeSeries::_sp ts) {
  
  if (this->source() && ts && !this->source()->units().isSameDimensionAs(ts->units())) {
    return; // can't do it.
  }
  
  _secondary = ts;
  this->invalidate();
}

Clock::_sp CorrelatorTimeSeries::correlationWindow() {
  return _corWindow;
}

void CorrelatorTimeSeries::setCorrelationWindow(Clock::_sp window) {
  _corWindow = window;
  this->invalidate();
}


#pragma mark - superclass overrides

TimeSeries::PointCollection CorrelatorTimeSeries::filterPointsInRange(TimeRange range) {
  
  PointCollection data(vector<Point>(), this->units());
  if (!this->correlatorTimeSeries() || !this->source()) {
    return data;
  }
  
  // force pre-cache
  this->source()->points(range.first - this->correlationWindow()->period(), range.second);
  this->correlatorTimeSeries()->points(range.first - this->correlationWindow()->period(), range.second);
  
  TimeSeries::_sp sourceTs = this->source();
  time_t windowWidth = this->correlationWindow()->period();
  
  set<time_t> times = this->timeValuesInRange(range);
  vector<Point> thePoints;
  thePoints.reserve(times.size());
  
  BOOST_FOREACH(time_t t, times) {
    double corrcoef = 0;
    PointCollection sourceCollection = sourceTs->pointCollection(t - windowWidth, t);
    
    set<time_t> sourceTimeValues;
    BOOST_FOREACH(const Point& p, sourceCollection.points) {
      sourceTimeValues.insert(p.time);
    }
    
    // expand the query range for the secondary collection
    TimeRange primaryRange = make_pair(*(sourceTimeValues.begin()), *(sourceTimeValues.rbegin()));
    TimeRange secondaryRange;
    secondaryRange.first = this->correlatorTimeSeries()->pointBefore(primaryRange.first + 1).time;
    secondaryRange.second = this->correlatorTimeSeries()->pointAfter(primaryRange.second - 1).time;
    
    PointCollection secondaryCollection = this->correlatorTimeSeries()->points(secondaryRange);
    secondaryCollection.resample(sourceTimeValues);
    
    if (sourceCollection.count() == 0 || secondaryCollection.count() == 0) {
      continue; // no points to correlate
    }
    
    if (sourceCollection.count() != secondaryCollection.count()) {
      cout << "Unequal number of points" << endl;
      return PointCollection(vector<Point>(), this->units());
    }
    
    // get consistent units
    secondaryCollection.convertToUnits(sourceCollection.units);
    
    // correlation coefficient
    accumulator_set<double, stats<tag::mean, tag::variance> > acc1;
    accumulator_set<double, stats<tag::mean, tag::variance> > acc2;
    accumulator_set<double, stats<tag::covariance<double, tag::covariate1> > > acc3;
    for (int i = 0; i < sourceCollection.count(); i++) {
      Point p1 = sourceCollection.points.at(i);
      Point p2 = secondaryCollection.points.at(i);
      acc1(p1.value);
      acc2(p2.value);
      acc3(p1.value, covariate1 = p2.value);
    }
    corrcoef = covariance(acc3)/sqrt(variance(acc1))/sqrt(variance(acc2));
    
    thePoints.push_back(Point(t,corrcoef));
    
  }
  
  return PointCollection(thePoints, this->units());
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


