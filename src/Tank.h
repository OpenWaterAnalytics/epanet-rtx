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
    void setLevelResetClock(Clock::_sp clock);
    Clock::_sp levelResetClock();
    void setResetLevelNextTime(bool reset);
    bool resetLevelNextTime();
    
    // parameters
    void setLevelMeasure(TimeSeries::_sp level);
    TimeSeries::_sp levelMeasure();
    // override parameters
    virtual void setHeadMeasure(TimeSeries::_sp head);
    
    // states
    TimeSeries::_sp level(); // directly related to head
    TimeSeries::_sp volumeMeasure(); // based on tank geometry
    TimeSeries::_sp flowMeasure();  // flow into the tank (computed)
    
  private:
    OffsetTimeSeries::_sp _level;
    OffsetTimeSeries::_sp _levelMeasure;
    CurveFunction::_sp _volumeMeasure;
    FirstDerivative::_sp _flowMeasure;
    Clock::_sp _resetLevel;
    double _minLevel, _maxLevel;
    bool _doesResetLevel;
    std::vector< std::pair<double,double> > _geometry;
    Units _geometryLevelUnits;
    Units _geometryVolumeUnits;
    bool _resetLevelNextTime;
    
  }; // Tank
  
  
}

#endif
