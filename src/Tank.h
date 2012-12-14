//
//  tank.h
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#ifndef epanet_rtx_tank_h
#define epanet_rtx_tank_h

#include "Junction.h"
#include "OffsetTimeSeries.h"
#include "CurveFunction.h"

namespace RTX {
  
  
  class Tank : public Junction {
  public:
    RTX_SHARED_POINTER(Tank);
    Tank(const std::string& name);
    virtual ~Tank();
    
    void setLevelMeasure(TimeSeries::sharedPointer level);
    
    void setElevation(double elevation);
    bool doesResetLevel();
    void setLevelResetClock(Clock::sharedPointer clock);
    Clock::sharedPointer levelResetClock();
    
    // states
    TimeSeries::sharedPointer level(); // directly related to head
    TimeSeries::sharedPointer volumeMeasure(); // based on tank geometry
    TimeSeries::sharedPointer flowMeasure();  // flow into the tank (computed)
    
  private:
    OffsetTimeSeries::sharedPointer _level;
    CurveFunction::sharedPointer _volume;
    TimeSeries::sharedPointer _flow;
    Clock::sharedPointer _resetLevel;
    double _minimumHead, _maximumHead;
    bool _doesResetLevel;
    
  }; // Tank
  
  
}

#endif
