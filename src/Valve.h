//
//  Valve.h
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#ifndef epanet_rtx_Valve_h
#define epanet_rtx_Valve_h

#include "Pipe.h"

namespace RTX {
  class Valve : public Pipe {
  public:
    TSF_BASE_PROPS(Valve);
    Valve(const std::string& name);
    virtual ~Valve();
    int valveType; // for epanet2, this is EN_LinkType
    double fixedSetting;
  };
}

#endif
