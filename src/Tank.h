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
    
    // public ivars for temporary (that is, steady-state) solutions
    double state_level;
    
    void setGeometry(std::vector< std::pair<double,double> > levelVolumePoints, Units levelUnits, Units volumeUnits, const std::string& curveName);
    std::vector< std::pair<double,double> > geometry();
    std::pair<Units,Units> geometryUnits();
    std::string geometryName;
    
    void setElevation(double elevation);
    
    void setNeedsReset(bool reset);
    bool needsReset();
    
    // parameters
    void setLevelMeasure(TimeSeries::_sp level);
    TimeSeries::_sp levelMeasure();
    // override parameters
    virtual void setHeadMeasure(TimeSeries::_sp head);
    
    // states
    TimeSeries::_sp level(); // directly related to head
    TimeSeries::_sp volume();
    TimeSeries::_sp flow();
    
    TimeSeries::_sp volumeMeasure(); // based on tank geometry
    TimeSeries::_sp flowMeasure();  // flow into the tank (computed)
    
    
  private:
    TimeSeries::_sp _level;
    TimeSeries::_sp _levelMeasure;
    CurveFunction::_sp _volumeMeasure;
    FirstDerivative::_sp _flowMeasure;
    TimeSeries::_sp _volume,_flow;
    double _minLevel, _maxLevel;
    std::vector< std::pair<double,double> > _geometry;
    Units _geometryLevelUnits;
    Units _geometryVolumeUnits;
    bool _willResetLevel;
    
  }; // Tank
  
  
}

#endif
