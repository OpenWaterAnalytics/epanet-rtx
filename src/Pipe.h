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
    Pipe(const std::string& name, Node::_sp startNode, Node::_sp endNode);
    virtual ~Pipe();
    
    virtual void setRecord(PointRecord::_sp record);
    double length();
    double diameter();
    void setLength(double length);
    void setDiameter(double diameter);
    
    status_t fixedStatus();
    void setFixedStatus(status_t status);
    
    // states
    TimeSeries::_sp flow();
    
    // public ivars for temporary (that is, steady-state) solutions
    double state_flow;
    
    // parameters
    TimeSeries::_sp statusParameter();
    void setStatusParameter(TimeSeries::_sp status);
    TimeSeries::_sp settingParameter();
    void setSettingParameter(TimeSeries::_sp setting);

    // measurements
    TimeSeries::_sp flowMeasure();
    void setFlowMeasure(TimeSeries::_sp flow);
    
  private:
    status_t _fixedStatus;
    double _length;
    double _diameter;
    TimeSeries::_sp _flowState;
    TimeSeries::_sp _flowMeasure;
    TimeSeries::_sp _status;
    TimeSeries::_sp _setting;
  };
  
}

#endif
