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
    RTX_SHARED_POINTER(Valve);
    Valve(const std::string& name, Node::_sp startNode, Node::_sp endNode);
    virtual ~Valve();
    int valveType;
    double fixedSetting;
  };
}

#endif
