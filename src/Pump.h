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

namespace RTX {
  class Pump : public Pipe {
  public:
    RTX_SHARED_POINTER(Pump);
    Pump(const std::string& name, Node::sharedPointer startNode, Node::sharedPointer endNode);
    virtual ~Pump();
    
    virtual void setRecord(PointRecord::sharedPointer record);
    
    // states
    TimeSeries::sharedPointer energy();
    
    // parameters
    bool doesHaveCurveParameter();
    TimeSeries::sharedPointer curveParameter();
    void setCurveParameter(TimeSeries::sharedPointer curve);
    
    bool doesHaveEnergyMeasure();
    TimeSeries::sharedPointer energyMeasure();
    void setEnergyMeasure(TimeSeries::sharedPointer energy);
    
  private:
    TimeSeries::sharedPointer _energyState;        // state
    TimeSeries::sharedPointer _energyMeasure; // parameter
    TimeSeries::sharedPointer _curve;
    bool _doesHaveCurveParameter;
    bool _doesHaveEnergyParameter;
  };

}

#endif
