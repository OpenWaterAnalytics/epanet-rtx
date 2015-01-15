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

bool CorrelatorTimeSeries::isCompatibleWith(TimeSeries::_sp withTimeSeries) {
  return (units().isDimensionless() || units().isSameDimensionAs(withTimeSeries->units()));
  // it's ok if the clocks aren't compatible.
}


TimeSeries::_sp CorrelatorTimeSeries::correlatorTimeSeries() {
  return _secondary;
}

void CorrelatorTimeSeries::setCorrelatorTimeSeries(TimeSeries::_sp ts) {
  _secondary = ts;
}

Clock::_sp CorrelatorTimeSeries::correlationWindow() {
  return _corWindow;
}

void CorrelatorTimeSeries::setCorrelationWindow(Clock::_sp window) {
  _corWindow = window;
}




#pragma mark - superclass overrides

void CorrelatorTimeSeries::setSource(TimeSeries::_sp source) {
  ModularTimeSeries::setSource(source);
  if (source) {
    this->setUnits(RTX_DIMENSIONLESS);
  }
}


vector<Point> CorrelatorTimeSeries::filteredPoints(TimeSeries::_sp sourceTs, time_t fromTime, time_t toTime) {
  
  std::vector<Point> thePoints;
  if (!this->correlatorTimeSeries() || !this->source()) {
    return thePoints;
  }
  
  // force pre-cache
  this->source()->points(fromTime - this->correlationWindow()->period(), toTime);
  this->correlatorTimeSeries()->points(fromTime - this->correlationWindow()->period(), toTime);
  
  vector<time_t> times;
  if (this->clock()->isRegular()) {
    times = this->clock()->timeValuesInRange(fromTime, toTime);
  }
  else {
    vector<Point>sourcePointsForTimes = this->source()->points(fromTime, toTime);
    BOOST_FOREACH(const Point& p, sourcePointsForTimes) {
      times.push_back(p.time);
    }
  }
  
  
  Units sourceU = sourceTs->units();
  Units secondaryUnits = this->correlatorTimeSeries()->units();
  time_t windowWidth = this->correlationWindow()->period();
  
  BOOST_FOREACH(time_t t, times) {
    double corrcoef = 0;
    vector<Point> sourcePoints = sourceTs->points(t - windowWidth, t);
    vector<Point> secondaryPoints = this->correlatorTimeSeries()->points(t - windowWidth, t);
    
    if (sourcePoints.size() != secondaryPoints.size()) {
      cout << "Unequal number of points" << endl;
      return thePoints;
    }
    
    // correlation coefficient
    accumulator_set<double, stats<tag::mean, tag::variance> > acc1;
    accumulator_set<double, stats<tag::mean, tag::variance> > acc2;
    accumulator_set<double, stats<tag::covariance<double, tag::covariate1> > > acc3;
    for (int i = 0; i < sourcePoints.size(); i++) {
      Point p1 = sourcePoints.at(i);
      Point p2 = Point::convertPoint(secondaryPoints.at(i), sourceU, secondaryUnits);
      acc1(p1.value);
      acc2(p2.value);
      acc3(p1.value, covariate1 = p2.value);
    }
    corrcoef = covariance(acc3)/sqrt(variance(acc1))/sqrt(variance(acc2));
    
    thePoints.push_back(Point(t,corrcoef));
    
  }
  
  return thePoints;
}







