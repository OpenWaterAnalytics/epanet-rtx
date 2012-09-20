//
//  tank.h
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See Readme.txt and license.txt for more information
//  https://sourceforge.net/projects/epanet-rtx/

#ifndef epanet_rtx_tank_h
#define epanet_rtx_tank_h

#include "junction.h"

namespace RTX {
  
  
  class Tank : public Junction {
  public:
    RTX_SHARED_POINTER(Tank);
    Tank(const std::string& name);
    virtual ~Tank();
    
    void setElevation(double elevation);
    bool doesResetLevel();
    void setLevelResetClock(Clock::sharedPointer clock);
    Clock::sharedPointer levelResetClock();
    
    // states
    TimeSeries::sharedPointer level(); // directly related to head
    
  private:
    AggregatorTimeSeries::sharedPointer _level;
    ConstantSeries::sharedPointer _elevationOffset;
    Clock::sharedPointer _resetLevel;
    double _minimumHead, _maximumHead;
    bool _doesResetLevel;
    
  }; // Tank
  
  
}

#endif
