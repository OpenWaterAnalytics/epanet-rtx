//
//  TimeSeries.h
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See Readme.txt and license.txt for more information
//  https://sourceforge.net/projects/epanet-rtx/

#ifndef epanet_rtx_timeseries_h
#define epanet_rtx_timeseries_h

#include <vector>
#include <map>
#include <iostream>

#include "rtxMacros.h"
#include "Point.h"
#include "PointContainer.h"
#include "PointRecord.h"
#include "Clock.h"
#include "IrregularClock.h"
#include "Units.h"

namespace RTX {

  
  //! Responsible for keeping a container of points - also adding, retrieving, and getting information about points.
  class TimeSeries {
    // override the ostream operator for pretty printing
    friend std::ostream& operator<< (std::ostream &out, TimeSeries &ts);
    
  public:
    //! \var typedef boost::shared_ptr< TimeSeries > sharedPointer
    //! \brief Type definition for a shared pointer to a TimeSeries object.
    RTX_SHARED_POINTER(TimeSeries);
    
    // ctor & dtor
    TimeSeries();
    ~TimeSeries();
    
    // methods
    virtual void insert(Point::sharedPointer aPoint);
    virtual void insertPoints(std::vector<Point::sharedPointer>);  /// option to add lots of (un)ordered points all at once.
    
    // getters
    virtual Point::sharedPointer point(time_t time);
    virtual Point::sharedPointer pointBefore(time_t time);
    virtual Point::sharedPointer pointAfter(time_t time);
    virtual std::vector< Point::sharedPointer > points(time_t start, time_t end); // points in range
    virtual std::pair< Point::sharedPointer, Point::sharedPointer > adjacentPoints(time_t time); // adjacent points
    virtual double value(time_t time);
    virtual Point::Qual_t quality(time_t time);
    virtual time_t period();                              //! 1/frequency (# seconds between data points)
    virtual std::string name();
    
    // setters
    virtual void setName(const std::string& name);
    void setCache(PointContainer::sharedPointer cache);
    void newCacheWithPointRecord(PointRecord::sharedPointer pointRecord);
    PointContainer::sharedPointer cache();
    void resetCache();
    void setClock(Clock::sharedPointer clock);
    Clock::sharedPointer clock();
    virtual void setUnits(Units newUnits);
    Units units();
    
    // tests
    virtual bool isPointAvailable(time_t time);


  protected:
    // methods which may be needed by subclasses but shouldn't be public:
    virtual std::ostream& toStream(std::ostream &stream);
    virtual bool isCompatibleWith(TimeSeries::sharedPointer withTimeSeries);
    
    
  private:
    PointContainer::sharedPointer _points;
    std::string _name;
    int _cacheSize;
    bool _hasClock;
    Clock::sharedPointer _clock;
    //TimeSeries::sharedPointer _source;
    // TODO -- units
    Units _units;
  
  };



}

#endif