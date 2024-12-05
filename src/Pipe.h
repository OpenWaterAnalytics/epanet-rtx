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

using TSF::TimeSeries;

namespace RTX {
//!   Pipe Class
/*!
 A Pipe is a hydraulic element that can carry water from one place to another.
 */
class Pipe : public Link {
public:
  typedef enum { OPEN = true, CLOSED = false } status_t;
  TSF_BASE_PROPS(Pipe);
  Pipe();
  Pipe(const std::string &name);
  virtual ~Pipe();

  virtual void setRecord(PointRecord::_sp record);
  double length();
  double diameter();
  void setLength(double length);
  void setDiameter(double diameter);
  
  std::vector<Node::location_t> vertices; /// lon,lat

  double roughness();
  void setRoughness(double roughness);
  
  double minorLoss();
  void setMinorLoss(double minorLoss);

  status_t fixedStatus();
  void setFixedStatus(status_t status);

  // states
  TimeSeries::_sp flow();
  TimeSeries::_sp quality();

  // public ivars for temporary (that is, steady-state) solutions
  double state_flow, state_setting, state_status, state_energy;
  double state_quality();
  
  
  // parameters
  TimeSeries::_sp statusBoundary();
  void setStatusBoundary(TimeSeries::_sp status);
  TimeSeries::_sp settingBoundary();
  void setSettingBoundary(TimeSeries::_sp setting);

  TimeSeries::_sp status();
  TimeSeries::_sp setting();
  
  // measurements
  TimeSeries::_sp flowMeasure();
  void setFlowMeasure(TimeSeries::_sp flow);

private:
  status_t _fixedStatus;
  double _length;
  double _diameter;
  double _roughness;
  double _minorLoss;
  TimeSeries::_sp _flowState, _qualityState;
  TimeSeries::_sp _flowMeasure;
  TimeSeries::_sp _statusBoundary;
  TimeSeries::_sp _settingBoundary;
  TimeSeries::_sp _status, _setting;
};
}

#endif
