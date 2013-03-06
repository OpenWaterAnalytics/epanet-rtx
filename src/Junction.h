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

namespace RTX {
  
  //!   Junction Class
  /*!
   A Junction is a Hydraulic element - any convergence of at least one link which can have a head value associated therewith.
   */
  class Junction : public Node {
  public:
    RTX_SHARED_POINTER(Junction);
    Junction(const std::string& name);
    virtual ~Junction();
    
    virtual void setRecord(PointRecord::sharedPointer record);
    
    // states
    double baseDemand();
    void setBaseDemand(double demand);
    
    TimeSeries::sharedPointer head();
    TimeSeries::sharedPointer pressure();
    TimeSeries::sharedPointer demand();
    TimeSeries::sharedPointer quality();
    
    // parameters
    bool doesHaveQualitySource();
    TimeSeries::sharedPointer qualitySource();
    void setQualitySource(TimeSeries::sharedPointer quality);
    
    bool doesHaveBoundaryFlow();
    TimeSeries::sharedPointer boundaryFlow(); // metered demand (+) or flow input (-)
    void setBoundaryFlow(TimeSeries::sharedPointer flow);
    
    bool doesHaveHeadMeasure();
    TimeSeries::sharedPointer headMeasure();
    virtual void setHeadMeasure(TimeSeries::sharedPointer head);
    
    bool doesHaveQualityMeasure();
    TimeSeries::sharedPointer qualityMeasure();
    void setQualityMeasure(TimeSeries::sharedPointer quality);
    
  private:
    // Parameters
    TimeSeries::sharedPointer _qualityBoundary;
    TimeSeries::sharedPointer _boundaryFlow;
    // Measurements
    TimeSeries::sharedPointer _qualityMeasure;
    TimeSeries::sharedPointer _headMeasure;
    // States
    TimeSeries::sharedPointer _demandState;
    TimeSeries::sharedPointer _headState;
    TimeSeries::sharedPointer _qualityState;
    // properties
    bool _doesHaveQualitySource;
    bool _doesHaveBoundaryFlow;
    bool _doesHaveHeadMeasure;
    bool _doesHaveQualityMeasure;
    double _baseDemand;
    
  }; // Junction
  
}

#endif
