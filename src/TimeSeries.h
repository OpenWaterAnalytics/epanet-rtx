//
//  TimeSeries.h
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//

#ifndef epanet_rtx_timeseries_h
#define epanet_rtx_timeseries_h

#include <vector>
#include <set>
#include <map>
#include <iostream>
#include <atomic>

#include "rtxMacros.h"
#include "Point.h"
#include "PointCollection.h"
#include "PointRecord.h"
#include "Clock.h"
#include "Units.h"
#include "TimeRange.h"
#include "WhereClause.h"

namespace RTX {
  /*!
   \class TimeSeries
   \brief An abstraction of Points ordered in time.

   The base TimeSeries class doesn't do much. Derive for added flavor.
   */

  /*!
   \fn virtual Point TimeSeries::point(time_t time)
   \brief Get a Point at a specific time.
   \param time The requested time.
   \return The requested Point, or the Point just prior to the passed time value.
   \sa Point
   */
  /*!
   \fn virtual std::vector<Point> TimeSeries::points(time_t start, time_t end)
   \brief Get a vector of Points within a specific time range.
   \param start The beginning of the requested time range.
   \param end The end of the requested time range.
   \return The requested Points (as a vector)

   The base class provides some brute-force logic to retrieve points, by calling Point() repeatedly. For more efficient access, you may wish to override this method.

   \sa Point
   */

  class TimeSeriesFilter;
  typedef std::shared_ptr<TimeSeriesFilter> TimeSeriesFilter_sp;


  class TimeSeries : public RTX_object {
  public:
    RTX_BASE_PROPS(TimeSeries);

    TimeSeries();
    TimeSeries(const std::string& name, const RTX::Units& units);
    ~TimeSeries();

    bool valid(time_t t);
    void setValid(bool v);

    virtual Clock::_sp clock() { return Clock::_sp(); };
    virtual void setClock(Clock::_sp clock) { };

    virtual void insert(Point aPoint);
    virtual void insertPoints(std::vector<Point>);  /// option to add lots of (un)ordered points all at once.

    virtual Point point(time_t time);
    virtual Point pointBefore(time_t time);
    virtual Point pointAfter(time_t time);
    virtual Point pointBefore(time_t time, WhereClause q);
    virtual Point pointAfter(time_t time, WhereClause q);
    virtual Point pointAtOrBefore(time_t time);
    PointCollection pointCollection(TimeRange range);
    virtual std::vector< Point > points(TimeRange range); // points in range

    virtual std::set<time_t> timeValuesInRange(TimeRange range);
    virtual time_t timeAfter(time_t t);
    virtual time_t timeBefore(time_t t);

    virtual std::string name();
    virtual void setName(const std::string& name);

    std::string userDescription();
    void setUserDescription(const std::string& desc);

    virtual PointRecord::_sp record();
    virtual void setRecord(PointRecord::_sp record);

    Units units();
    virtual void setUnits(Units newUnits);
    virtual bool canChangeToUnits(Units units) {return true;};

    virtual std::vector<TimeSeries::_sp> rootTimeSeries() { return std::vector<TimeSeries::_sp> {this->sp()}; };
    virtual void resetCache();
    virtual void invalidate();

    virtual std::ostream& toStream(std::ostream &stream);

    time_t expectedPeriod();
    void setExpectedPeriod(time_t seconds);

    virtual bool hasUpstreamSeries(TimeSeries::_sp other);

    virtual void filterDidAddSource(TimeSeriesFilter_sp filter);
    virtual void filterDidRemoveSource(TimeSeriesFilter_sp filter);
    virtual bool isSink(TimeSeriesFilter_sp filter);
    std::set<TimeSeriesFilter_sp> sinks();

    virtual bool supportsQualifiedQuery();

    // chainable
    TimeSeries::_sp units(Units u) {this->setUnits(u); return this->sp();};
    TimeSeries::_sp c(Clock::_sp c) {this->setClock(c); return this->sp();};
    TimeSeries::_sp name(const std::string& n) {this->setName(n); return share_me(this);};
    TimeSeries::_sp userDescription(const std::string& d) {this->setUserDescription(d); return share_me(this);};
    TimeSeries::_sp record(PointRecord::_sp record) {this->setRecord(record); return share_me(this);};

    template<class T>
    std::shared_ptr<T> append(T* newFilter){
      std::shared_ptr<T> sp(newFilter);
      newFilter->setSource(this->sp());
      return sp;
    };


  protected:
    std::atomic<bool> _valid;

  private:
    PointRecord::_sp _points;
    std::string _name, _userDescription;
    Units _units;
    std::pair<time_t, time_t> _validTimeRange;
    time_t _expectedPeriod;
    std::set<TimeSeriesFilter_sp> _sinks;

  };

  std::ostream& operator<< (std::ostream &out, TimeSeries &ts);

}

#endif
