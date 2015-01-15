//
//  ModularTimeSeries.h
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#ifndef epanet_rtx_ModularTimeSeries_h
#define epanet_rtx_ModularTimeSeries_h

#include "TimeSeries.h"

namespace RTX {
  
  /*! 
   \class ModularTimeSeries
   \brief A Time Series class that allows a single upstream TimeSeries.
   
   This class generalizes some basic functionality for the "modular" design. This implementation serves as a "pass-through", performing no analysis or modification of the data, but caching requested Points in this object's PointRecord.
   
   */
  /*! 
   \fn TimeSeries::_sp ModularTimeSeries::source() 
   \brief Get the source of this time series.
   \return A shared pointer to a TimeSeries (or derived) object.
   \sa TimeSeries
   */
  /*! 
   \fn void ModularTimeSeries::setSource(TimeSeries::_sp source)
   \brief Set the upstream TimeSeries object
   \param source A shared pointer to the intended upstream TimeSeries object.
   */
  /*! 
   \fn bool ModularTimeSeries::doesHaveSource() 
   \brief Does this Modular time series have an upstream source?
   \return true / false
   */

  
  class ModularTimeSeries : public TimeSeries {
    
  public:
    RTX_SHARED_POINTER(ModularTimeSeries);
    ModularTimeSeries();
    virtual ~ModularTimeSeries();
    
    // class-specific methods
    virtual TimeSeries::_sp source();
    virtual void setSource(TimeSeries::_sp source);
    virtual bool doesHaveSource();
    
    virtual TimeSeries::_sp rootTimeSeries();
    
    // overridden methods from parent class
    //virtual bool isPointAvailable(time_t time);
    virtual Point point(time_t time);
    virtual Point pointBefore(time_t time);
    virtual Point pointAfter(time_t time);
    virtual std::vector< Point > points(time_t start, time_t end);
    virtual void setUnits(Units newUnits);
    
    virtual void resetCache();
    
    virtual std::ostream& toStream(std::ostream &stream);
    
    // essentially these are delegate methods, up the the subclass to override
    virtual bool canAlterDimension() { return false; };
    virtual bool canAlterClock() { return false; };
    
  protected:
    
    virtual std::vector<Point> filteredPoints(TimeSeries::_sp sourceTs, time_t fromTime, time_t toTime);
    virtual bool isCompatibleWith(TimeSeries::_sp withTimeSeries);

  private:
    TimeSeries::_sp _source;
    bool _doesHaveSource;
  };
  
}

#endif
