//
//  Pipe.h
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#ifndef epanet_rtx_Pipe_h
#define epanet_rtx_Pipe_h

#include "Link.h"

namespace RTX {
  //!   Pipe Class
  /*!
   A Pipe is a hydraulic element that can carry water from one place to another.
   */
  class Pipe : public Link {
  public:
    typedef enum {OPEN = true, CLOSED = false} status_t;
    RTX_SHARED_POINTER(Pipe);
    Pipe(const std::string& name, Node::sharedPointer startNode, Node::sharedPointer endNode);
    virtual ~Pipe();
    
    virtual void setRecord(PointRecord::sharedPointer record);
    double length();
    double diameter();
    void setLength(double length);
    void setDiameter(double diameter);
    
    // states
    TimeSeries::sharedPointer flow();
    
    // parameters
    bool doesHaveStatusParameter();
    TimeSeries::sharedPointer statusParameter();
    void setStatusParameter(TimeSeries::sharedPointer status);
    
    // measurements
    bool doesHaveFlowMeasure();
    TimeSeries::sharedPointer flowMeasure();
    void setFlowMeasure(TimeSeries::sharedPointer flow);
    
  private:
    double _length;
    double _diameter;
    TimeSeries::sharedPointer _flowState;
    TimeSeries::sharedPointer _flowMeasure;
    TimeSeries::sharedPointer _status;
    bool _doesHaveStatusParameter;
    bool _doesHaveFlowMeasure;
  };
  
}

#endif
