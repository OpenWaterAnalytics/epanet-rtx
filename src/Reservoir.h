//
//  Reservoir.h
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#ifndef epanet_rtx_reservoir_h
#define epanet_rtx_reservoir_h

#include "Junction.h"
using TSF::TimeSeries;

namespace RTX {
  
  class Reservoir : public Junction {
  public:
    TSF_BASE_PROPS(Reservoir);
    Reservoir(const std::string& name);
    virtual ~Reservoir();
    
    void setFixedLevel(double level);
    double fixedLevel();
    
    // maybe?
    TimeSeries::_sp boundaryHead();
    void setBoundaryHead(TimeSeries::_sp head);
    TimeSeries::_sp boundaryQuality();
    void setBoundaryQuality(TimeSeries::_sp quality);

    
  private:
    double _fixedLevel;
    
  }; // class Reservoir
  
}

#endif
