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
#include <OffsetTimeSeries.h>
#include <CurveFunction.h>
#include <FirstDerivative.h>
#include <Curve.h>

using TSF::TimeSeries;
using TSF::Curve;
using TSF::FirstDerivative;
using TSF::CurveFunction;

namespace RTX {
  
  class Tank : public Junction {
  public:
    TSF_BASE_PROPS(Tank);
    Tank(const std::string& name);
    virtual ~Tank();
    
    void setMinMaxLevel(double minLevel, double maxLevel);
    double minLevel();
    double maxLevel();

    void setEnProperties(double initLevel, double diameter);
    double initLevel();
    double diameter();
    
    // public ivars for temporary (that is, steady-state) solutions
    double state_level;
    
    void setGeometry(Curve::_sp curve);
    Curve::_sp geometry();
    
    void setElevation(double elevation);
    
    void setNeedsReset(bool reset);
    bool needsReset();
    
    // parameters
    void setLevelMeasure(TimeSeries::_sp level);
    TimeSeries::_sp levelMeasure();
    
    void setDmaLevelMeasure(TimeSeries::_sp level);
    TimeSeries::_sp dmaLevelMeasure();
    
    void setDmaHeadMeasure(TimeSeries::_sp level);
    TimeSeries::_sp dmaHeadMeasure();
    
    // override parameters
    virtual void setHeadMeasure(TimeSeries::_sp head);
    
    // states
    TimeSeries::_sp level(); // directly related to head
    TimeSeries::_sp volume();
    TimeSeries::_sp flow();
    TimeSeries::_sp inletQuality();
    
    TimeSeries::_sp volumeCalc(); // based on tank geometry
    TimeSeries::_sp flowCalc();  // calculated flow into the tank
    
    TimeSeries::_sp dmaVolumeCalc(); // based on tank geometry
    TimeSeries::_sp dmaFlowCalc();  // calculated flow into the tank
    
    
  private:
    TimeSeries::_sp _level;
    TimeSeries::_sp _levelMeasure, _dmaLevelMeasure, _dmaHeadMeasure;
    CurveFunction::_sp _volumeCalc, _dmaVolumeCalc;
    FirstDerivative::_sp _flowCalc, _dmaFlowCalc;
    TimeSeries::_sp _volume,_flow;
    TimeSeries::_sp _inletQualityState;
    double _minLevel, _maxLevel, _initLevel, _diameter;
    Curve::_sp _geometry;
    bool _willResetLevel;
    
  }; // Tank
  
  
}

#endif
