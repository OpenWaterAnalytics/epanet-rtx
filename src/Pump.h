//
//  Pump.h
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#ifndef epanet_rtx_Pump_h
#define epanet_rtx_Pump_h

#include "Pipe.h"
#include <Curve.h>

using TSF::TimeSeries;
using TSF::Curve;

namespace RTX {
  class Pump : public Pipe {
  public:
    TSF_BASE_PROPS(Pump);
    Pump(const std::string& name);
    virtual ~Pump();
    
    virtual void setRecord(PointRecord::_sp record);
    
    // states
    TimeSeries::_sp energy();
    
    // parameters
    TimeSeries::_sp curveParameter();
    void setCurveParameter(TimeSeries::_sp curve);
    
    TimeSeries::_sp energyMeasure();
    void setEnergyMeasure(TimeSeries::_sp energy);
    
    Curve::_sp headCurve();
    void setHeadCurve(Curve::_sp curve);
    
    Curve::_sp efficiencyCurve();
    void setEfficiencyCurve(Curve::_sp curve);
    
    
  private:
    TimeSeries::_sp _energyState;        // state
    TimeSeries::_sp _energyMeasure; // parameter
    Curve::_sp _headCurve, _efficiencyCurve;
  };

}

#endif
