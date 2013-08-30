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
    
    void setMinMaxLevel(double minLevel, double maxLevel);
    double minLevel();
    double maxLevel();
    
    void setGeometry(std::vector< std::pair<double,double> > levelVolumePoints, Units levelUnits, Units volumeUnits);
    std::vector< std::pair<double,double> > geometry();
    std::pair<Units,Units> geometryUnits();
    std::string geometryName;
    
    void setElevation(double elevation);
    bool doesResetLevelUsingClock();
    void setResetLevelNextTime(bool reset);
    bool resetLevelNextTime();
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
    double _minLevel, _maxLevel;
    bool _doesResetLevel;
    std::vector< std::pair<double,double> > _geometry;
    Units _geometryLevelUnits;
    Units _geometryVolumeUnits;
    bool _resetLevelNextTime;
    
  }; // Tank
  
  
}

#endif
