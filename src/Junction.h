//
//  junction.h
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#ifndef epanet_rtx_junction_h
#define epanet_rtx_junction_h

#include "Node.h"

using TSF::TimeSeries;

namespace RTX {
  
  //!   Junction Class
  /*!
   A Junction is a Hydraulic element - any convergence of at least one link which can have a head value associated therewith.
   */
  class Junction : public Node {
  public:
    TSF_BASE_PROPS(Junction);
    Junction(const std::string& name);
    virtual ~Junction();
    
    virtual void setRecord(PointRecord::_sp record);
    
    // states
    double baseDemand();
    void setBaseDemand(double demand);
    
    TimeSeries::_sp head();
    TimeSeries::_sp pressure();
    TimeSeries::_sp demand();
    TimeSeries::_sp quality();
    
    // public ivars for temporary (that is, steady-state) solutions
    double state_head, state_pressure, state_demand, state_quality, state_inlet_quality, state_volume, state_flow;
    
    
    // parameters
    TimeSeries::_sp qualitySource();
    void setQualitySource(TimeSeries::_sp quality);
    
    TimeSeries::_sp boundaryFlow(); // metered demand (+) or flow input (-)
    void setBoundaryFlow(TimeSeries::_sp flow);
    
    TimeSeries::_sp headMeasure();
    TimeSeries::_sp pressureMeasure();
    virtual void setHeadMeasure(TimeSeries::_sp head);
    virtual void setPressureMeasure(TimeSeries::_sp pressure);
    
    TimeSeries::_sp qualityMeasure();
    void setQualityMeasure(TimeSeries::_sp quality);
    
  private:
    // Parameters
    TimeSeries::_sp _qualityBoundary;
    TimeSeries::_sp _boundaryFlow;
    // Measurements
    TimeSeries::_sp _qualityMeasure;
    TimeSeries::_sp _headMeasure, _pressureMeasure;
    // States
    TimeSeries::_sp _demandState;
    TimeSeries::_sp _headState;
    TimeSeries::_sp _pressureState;
    TimeSeries::_sp _qualityState;
    // properties
    double _baseDemand;
    
  }; // Junction
  
}

#endif
