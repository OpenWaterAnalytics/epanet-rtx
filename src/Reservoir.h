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

namespace RTX {
  
  class Reservoir : public Junction {
  public:
    RTX_SHARED_POINTER(Reservoir);
    Reservoir(const std::string& name);
    virtual ~Reservoir();
    
    void setFixedLevel(double level);
    double fixedLevel();
    
    // maybe?
    bool doesHaveBoundaryHead();
    TimeSeries::sharedPointer boundaryHead();
    void setBoundaryHead(TimeSeries::sharedPointer head);
    
  private:
    double _fixedLevel;
    
  }; // class Reservoir
  
}

#endif
