//
//  clock.h
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#ifndef epanet_rtx_clock_h
#define epanet_rtx_clock_h

#include <time.h>
#include <vector>
#include "rtxMacros.h"

namespace RTX {
  
  
  /*! 
   \class Clock
   \brief A simple clock class for keeping time synchronized.
   
   Provides methods for comparing clocks to eachother for compatibility and testing the validity of a time value (e.g., if it falls on the regular intervals described by Clock. This class is meant to work with the TimeSeries class family to ensure that regular time series can be linked to eachother properly, and that irregular time series are only connected to TimeSeries classes that can resample (like Resampler).
   
   Every Time Series object needs a clock. Whether the clock is based on an (offset,period) or based on a PointRecord, this clock needs to exist. By default, a TimeSeries' clock is irregular, and gets its pattern from the TS's PointRecord - the result of this is that the clock will show a valid time only for points that exist in the record. The other way to do this is to set a (offset,period) for the clock, and the result will be that the TimeSeries will only retrieve / store points on valid regular timesteps. In the first case, the PointRecord is the master synchronizer, and in the second case the Clock is the master synchronizer.
   
   A reason for structuring it this (perhaps confusing) way is that it simplifies the implementation of TimeSeries, which is already quite complex. A TimeSeries-derived class can just depend on its clock to help it retrieve points. Whether the clock is synchronized manually or by a PointRecord is transparent to the user.
      
   */
  
  /*! 
   \fn Clock::Clock(int period, time_t start)
   \brief Constructor for Clock. Takes a period and offset time.
   \param period The regular period for the clock.
   \param start The start time (offset).
   
   \fn bool Clock::isCompatibleWith(Clock::sharedPointer clock)
   \brief Test for compatibility (passed clock parameter may be faster, but must fall on even steps)
   \param clock A shared pointer to another clock object.
   \return Boolean true/false value.
   
   \fn bool Clock::isValid(time_t time)
   \brief Test for validity of time value (if the passed value falls on this clock's regular steps)
   \param time A time value.
   \return Boolean true/false value
   
   \fn bool Clock::isRegular()
   \brief Test for regularity (period greater than zero)
   \return Boolean true/false value
   
   \fn int Clock::period()
   \brief The period of the time series
   \return The period (in seconds) of the time series. Zero if irregular.
   
   \fn time_t Clock::start()
   \brief The starting point for the clock.
   \return A unix-time value for the clock's start position (offset from zero-time).
   
   \fn time_t Clock::timeAfter(time_t time)
   \brief The next time step in this clock's regular pattern.
   \param time A time value.
   \return A unix-time value representing the next step in the pattern.
   \sa http://www.youtube.com/watch?v=VdQY7BusJNU
   
   \fn time_t Clock::timeBefore(time_t time)
   \brief The previous time step in this clock's regular pattern.
   \param time A time value.
   \return A unix-time value representing the previous step in the pattern.
   
   \fn std::vector<time_t> Clock::timeValuesInRange(time_t start, time_t end)
   \brief Get a list of time values that are valid within a range.
   \param start A range start time.
   \param end A range end time.
   \return A vector of time_t values that are valid for this clock within the specified range.
   
   */
  
  
  
  
  class Clock {
    
  public:
    RTX_SHARED_POINTER(Clock);
    Clock(int period, time_t start = 0);
    virtual ~Clock();
    
    virtual bool isCompatibleWith(Clock::sharedPointer clock);
    virtual bool isValid(time_t time);
    virtual time_t validTime(time_t time);
    virtual time_t timeAfter(time_t time);
    virtual time_t timeBefore(time_t time);
    
    bool isRegular();
    int period();
    time_t start();
    virtual std::vector< time_t > timeValuesInRange(time_t start, time_t end);
    virtual std::ostream& toStream(std::ostream &stream);
    
  private:
    time_t timeOffset(time_t time);
    time_t _start;
    int _period;
    bool _isRegular;
    
  };
  
  std::ostream& operator<< (std::ostream &out, Clock &clock);
}

#endif
