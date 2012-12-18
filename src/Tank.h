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
#include "FirstDerivative.h"

namespace RTX {
  
  
  class Tank : public Junction {
  public:
    RTX_SHARED_POINTER(Tank);
    Tank(const std::string& name);
    virtual ~Tank();
    
    void setGeometry(std::vector< std::pair<double,double> > levelVolumePoints, Units levelUnits, Units volumeUnits); 
    void setElevation(double elevation);
    bool doesResetLevel();
    void setLevelResetClock(Clock::sharedPointer clock);
    Clock::sharedPointer levelResetClock();
    
    // parameters
    void setLevelMeasure(TimeSeries::sharedPointer level);
    TimeSeries::sharedPointer levelMeasure();
    // override parameters
    virtual void setHeadMeasure(TimeSeries::sharedPointer head);
    
    // states
    TimeSeries::sharedPointer level(); // directly related to head
    TimeSeries::sharedPointer volumeMeasure(); // based on tank geometry
    TimeSeries::sharedPointer flowMeasure();  // flow into the tank (computed)
    
  private:
    OffsetTimeSeries::sharedPointer _level;
    OffsetTimeSeries::sharedPointer _levelMeasure;
    CurveFunction::sharedPointer _volumeMeasure;
    FirstDerivative::sharedPointer _flowMeasure;
    Clock::sharedPointer _resetLevel;
    double _minimumHead, _maximumHead;
    bool _doesResetLevel;
    
  }; // Tank
  
  
}

#endif
