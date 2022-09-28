//
//  TimeSeries.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//

#include <limits.h>
#include <boost/foreach.hpp>

#include "TimeSeries.h"
#include "PointRecord.h"
#include "PointCollection.h"

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp>
#include <boost/accumulators/statistics/tail_quantile.hpp>

#include "TimeSeriesFilter.h"


using namespace RTX;
using namespace std;

#pragma mark - Time Series methods


TimeSeries::TimeSeries() : _valid(true) {
  _name = "";
  _points.reset( new PointRecord() );
  setName("Time Series");
  _units = RTX_NO_UNITS;
}

TimeSeries::TimeSeries(const std::string& name, const RTX::Units& units) {
  _name = name;
  _units = units;
  _points.reset( new PointRecord() );
  _points->registerAndGetIdentifierForSeriesWithUnits(name, units);
  _valid = true;
}

TimeSeries::~TimeSeries() {
  // empty Dtor
}

std::ostream& RTX::operator<< (std::ostream &out, TimeSeries &ts) {
  return ts.toStream(out);
}

#pragma mark Public Methods

bool TimeSeries::valid(time_t t) {
  return _valid;
}
void TimeSeries::setValid(bool v) {
  _valid = v;
}

void TimeSeries::setName(const std::string& name) {
  _name = name;
  _points->registerAndGetIdentifierForSeriesWithUnits(name, this->units());
}

std::string TimeSeries::name() {
  return _name;
}


std::string TimeSeries::userDescription() {
  return _userDescription;
}

void TimeSeries::setUserDescription(const std::string& desc) {
  _userDescription = desc;
}

void TimeSeries::insert(Point thisPoint) {
  _points->addPoint(name(), thisPoint);
}

void TimeSeries::insertPoints(std::vector<Point> points) {
  _points->addPoints(name(), points);
}

Point TimeSeries::point(time_t time) {
  Point p;
  vector<Point> single = this->points(TimeRange(time,time));
  if (single.size() > 0) {
    Point goodPoint = single.front();
    if (goodPoint.time == time) {
      p = goodPoint;
    }
  }
  return p;
}

PointCollection TimeSeries::pointCollection(TimeRange range) {
  return PointCollection(this->points(range), this->units());
}

// get a range of points from this TimeSeries' point method
std::vector< Point > TimeSeries::points(TimeRange range) {
  // container for points in this range
  std::vector< Point > points;

  if (!range.isValid()) {
    return points;
  }

  if (!this->record()->exists(this->name(), this->units())) {
    this->record()->registerAndGetIdentifierForSeriesWithUnits(this->name(), this->units());
  }

  points = this->record()->pointsInRange(this->name(), range);
  return points;
}

std::set<time_t> TimeSeries::timeValuesInRange(TimeRange range) {
  auto points = this->pointCollection(range);
  return points.times();
}

time_t TimeSeries::timeAfter(time_t t) {
  if (this->clock()) {
    return this->clock()->timeAfter(t);
  }
  else {
    return TimeSeries::pointAfter(t).time;
  }
}


time_t TimeSeries::timeBefore(time_t t) {
  if (this->clock()) {
    return this->clock()->timeBefore(t);
  }
  else {
    return TimeSeries::pointBefore(t).time;
  }
}


Point TimeSeries::pointBefore(time_t time) {
  if (time == 0) {
    return Point();
  }
  return this->record()->pointBefore(this->name(), time);
}

Point TimeSeries::pointAfter(time_t time) {
  if (time == 0) {
    return Point();
  }
  return this->record()->pointAfter(this->name(), time);
}

Point TimeSeries::pointBefore(time_t time, WhereClause q) {
  if (time == 0) {
    return Point();
  }
  return this->record()->pointBefore(this->name(), time, q);
}

Point TimeSeries::pointAfter(time_t time, WhereClause q) {
  if (time == 0) {
    return Point();
  }
  return this->record()->pointAfter(this->name(), time, q);
}

Point TimeSeries::pointAtOrBefore(time_t time) {
  Point p = this->point(time);
  if (!p.isValid) {
    p = this->pointBefore(time);
  }
  return p;
}


void TimeSeries::setRecord(PointRecord::_sp record) {
  if (!record) {
    PointRecord::_sp pr( new PointRecord() );
    record = pr;
  }
  if (record->registerAndGetIdentifierForSeriesWithUnits(this->name(),this->units())) {
    _points = record;
  }
  return;
}

PointRecord::_sp TimeSeries::record() {
  return _points;
}

void TimeSeries::resetCache() {
  _points->reset(name());
}

void TimeSeries::invalidate() {
  if(_points) {
    _points->invalidate(this->name());
    if (!_points->registerAndGetIdentifierForSeriesWithUnits(this->name(), this->units())) {
      PointRecord::_sp pr( new PointRecord() );
      _points = pr;
    }
  }
}


void TimeSeries::setUnits(Units newUnits) {
  // changing units means the values here are no good anymore.
  if (this->canChangeToUnits(newUnits)) {
    if (! (newUnits == this->units())) {
      // if original units were NONE, then we don't need to invalidate. we are just setting this up for the first time.
      bool shouldInvalidate = true;
      if (_units == RTX_NO_UNITS || _units == newUnits) {
        shouldInvalidate = false;
      }
      _units = newUnits;
      if (shouldInvalidate) {
        this->invalidate();
      }
      else {
        if (_points) {
          _points->registerAndGetIdentifierForSeriesWithUnits(this->name(), this->units());
        }
      }
    }
  }
}

Units TimeSeries::units() {
  return _units;
}


time_t TimeSeries::expectedPeriod() {
  return _expectedPeriod;
}
void TimeSeries::setExpectedPeriod(time_t seconds) {
  _expectedPeriod = seconds;
}


bool TimeSeries::hasUpstreamSeries(TimeSeries::_sp other) {
  return false;
}


void TimeSeries::filterDidAddSource(TimeSeriesFilter::_sp filter) {
  _sinks.insert(filter);
}
void TimeSeries::filterDidRemoveSource(TimeSeriesFilter::_sp filter) {
  _sinks.erase(filter);
}
bool TimeSeries::isSink(TimeSeriesFilter::_sp filter) {
  return _sinks.count(filter) > 0;
}
std::set<TimeSeriesFilter::_sp> TimeSeries::sinks() {
  return _sinks;
}

bool TimeSeries::supportsQualifiedQuery() {
  return (_points && _points->supportsQualifiedQuery());
}


#pragma mark Protected Methods

std::ostream& TimeSeries::toStream(std::ostream &stream) {
  stream << "Time Series: \"" << _name << "\"\n";
  stream << "Units: " << _units << std::endl;
  stream << "Cached Points:" << std::endl;
  stream << *_points;
  return stream;
}

// chaning
