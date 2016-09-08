//
//  TimeSeriesFilter.h
//  epanet-rtx
//
#ifndef __epanet_rtx__TimeSeriesFilter__
#define __epanet_rtx__TimeSeriesFilter__

#include <stdio.h>

#include "TimeSeries.h"

namespace RTX {
  
  /*!
   \class TimeSeriesFilter
   \brief A modular time series operation. Filter base class provides resampling, unit conversion, and caching.
   
   The base TimeSeriesFilter class doesn't do very much. Derive for added flavor.
   */
  
  
  /*!
   \fn virtual std::set<time_t> TimeSeriesFilter::timeValuesInRange(TimeRange range)
   \brief Allow derived classes to specify the occurence of points in time. Optional.
   \param range The time range over which to report time values.
   \return A set of time values where the Time Series may provide points.
  
   Overriding this method is optional. Base functionality reports clock ticks if there is a clock, or the time values for this object's source points, if a source is set.
   */
  /*!
   \fn virtual PointCollection TimeSeriesFilter::filterPointsAtTimes(std::set<time_t> times)
   \brief Generate time series values at given points.
   \param times The list of times for which to provide filtered data.
   \return A PointCollection containing the filtered data.
  
   This is where the magic happens. You must override this to add any meaningful functionality for a derived class. Important: derived classes are responsible for converting units.
   */
  /*!
   \fn virtual bool TimeSeriesFilter::canSetSource(TimeSeries::_sp ts)
   \brief Allows a derived class to refuse a source. Base implmentation enforces pass-through dimensional consistency.
   \param ts The potential new source time series.
   
   Overriding this method is optional. Base implementation enforces dimensonal consistency.
   */
  /*!
   \fn virtual void TimeSeriesFilter::didSetSource(TimeSeries::_sp ts)
   \brief Notify derived class that a new source was set.
   \param ts The new source which was set.
   
   Overriding this method is optional. Base implementation updates units and clock (if the new source has one).
   */
  /*!
   \fn bool TimeSeriesFilter::canChangeToUnits(Units units)
   \brief Allows a derived class to refuse changing units. This can be called when testing for a new source, or when trying to manually set units, or for information purposes like populating a list of available units.
   \param units The new units.
   
   Overriding this method is optional. Base implementation enforces dimensonal consistency.
   */

  
  
  class TimeSeriesFilter : public TimeSeries {
  public:
    RTX_BASE_PROPS(TimeSeriesFilter);
    TimeSeriesFilter();
    // explicit constructor
    
    virtual TimeSeries::_sp source();
    virtual void setSource(TimeSeries::_sp ts);
    
    Clock::_sp clock();
    void setClock(Clock::_sp clock);
    
    TimeSeriesResampleMode resampleMode();
    void setResampleMode(TimeSeriesResampleMode mode);
    
    std::vector< Point > points(TimeRange range);
    virtual Point pointBefore(time_t time);
    virtual Point pointAfter(time_t time);
    
    virtual bool willResample();
    virtual void setUnits(Units newUnits); // filter ts objects can invalidate their backing store.
    
    virtual bool canDropPoints() { return false; };
    
    virtual TimeSeries::_sp rootTimeSeries();
    
    // methods you must override to provide info to the base class
    virtual PointCollection filterPointsInRange(TimeRange range);
    
    virtual std::set<time_t> timeValuesInRange(TimeRange range);
    virtual time_t timeAfter(time_t t);
    virtual time_t timeBefore(time_t t);
    
    virtual bool canSetSource(TimeSeries::_sp ts);
    virtual void didSetSource(TimeSeries::_sp ts);
    virtual bool canChangeToUnits(Units units);
    
    // chainable
    TimeSeriesFilter::_sp resample(TimeSeriesResampleMode mode) {this->setResampleMode(mode); return share_me(this);};
    TimeSeriesFilter::_sp source(TimeSeries::_sp source) {this->setSource(source); return share_me(this);};
    
  private:
    TimeSeries::_sp _source;
    Clock::_sp _clock;
    TimeSeriesResampleMode _resampleMode;
    
  };
}

#endif /* defined(__epanet_rtx__TimeSeriesFilter__) */
