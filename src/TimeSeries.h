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
#include <map>
#include <iostream>

#include "rtxMacros.h"
#include "Point.h"
#include "PointRecord.h"
#include "Clock.h"
#include "Units.h"

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
  
  
  
  
  class TimeSeries {    
  public:
    
    // public internal class description for the summary
    class Summary {
    public:
      std::vector<Point> points;
      std::vector<Point> gaps;
      double mean;
      double variance;
      size_t count;
      double min;
      double max;
    };
    
    
    RTX_SHARED_POINTER(TimeSeries);
    
    // ctor & dtor
    TimeSeries();
    ~TimeSeries();
    
    // methods
    virtual void insert(Point aPoint);
    virtual void insertPoints(std::vector<Point>);  /// option to add lots of (un)ordered points all at once.
    
    // getters
    virtual Point point(time_t time);
    virtual Point pointBefore(time_t time);
    virtual Point pointAfter(time_t time);
    virtual Point pointAtOrBefore(time_t time);
    virtual std::vector< Point > points(time_t start, time_t end); // points in range
    virtual std::pair< Point, Point > adjacentPoints(time_t time); // adjacent points
    virtual time_t period();                              //! 1/frequency (# seconds between data points)
    virtual std::string name();
    PointRecord::sharedPointer record();
    
    TimeSeries::Summary summary(time_t start, time_t end);
    
    // setters
    virtual void setName(const std::string& name);
    void setRecord(PointRecord::sharedPointer record);
    void resetCache();
    virtual void setClock(Clock::sharedPointer clock);
    Clock::sharedPointer clock();
    
    virtual void setUnits(Units newUnits);
    Units units();
    
    
    void setFirstTime(time_t time);
    void setLastTime(time_t time);
    time_t firstTime();
    time_t lastTime();
    
    // tests
    //virtual bool isPointAvailable(time_t time);
    
    virtual std::ostream& toStream(std::ostream &stream);

  protected:
    // methods which may be needed by subclasses but shouldn't be public:
    virtual bool isCompatibleWith(TimeSeries::sharedPointer withTimeSeries);
    
  private:
    PointRecord::sharedPointer _points;
    std::string _name;
    int _cacheSize;
    Clock::sharedPointer _clock;
    //TimeSeries::sharedPointer _source;
    // TODO -- units
    Units _units;
    std::pair<time_t, time_t> _validTimeRange;
  
  };

  std::ostream& operator<< (std::ostream &out, TimeSeries &ts);

}

#endif