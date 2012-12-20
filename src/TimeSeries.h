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

  
  //! Responsible for keeping a container of points - also adding, retrieving, and getting information about points.
  class TimeSeries {    
  public:
    //! \var typedef boost::shared_ptr< TimeSeries > sharedPointer
    //! \brief Type definition for a shared pointer to a TimeSeries object.
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
    virtual std::vector< Point > points(time_t start, time_t end); // points in range
    virtual std::pair< Point, Point > adjacentPoints(time_t time); // adjacent points
    virtual time_t period();                              //! 1/frequency (# seconds between data points)
    virtual std::string name();
    
    // setters
    virtual void setName(const std::string& name);
    void setRecord(PointRecord::sharedPointer record);
    void newCacheWithPointRecord(PointRecord::sharedPointer pointRecord);
    PointRecord::sharedPointer record();
    void resetCache();
    void setClock(Clock::sharedPointer clock);
    Clock::sharedPointer clock();
    virtual void setUnits(Units newUnits);
    Units units();
    
    // tests
    virtual bool isPointAvailable(time_t time);
    
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
  
  };

  std::ostream& operator<< (std::ostream &out, TimeSeries &ts);

}

#endif