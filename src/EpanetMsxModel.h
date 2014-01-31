//
//  EpanetMsxModel.h
//  epanet-rtx
//
//  Created by Sam Hatchett on 12/20/13.
//
//

#ifndef __epanet_rtx__EpanetMsxModel__
#define __epanet_rtx__EpanetMsxModel__

#include <iostream>

#include "EpanetModel.h"

extern "C" {
#include <epanetmsx.h>
}



namespace RTX {
  
  class EpanetMsxModel : public EpanetModel {
    
  public:
    RTX_SHARED_POINTER(EpanetMsxModel);
    EpanetMsxModel();
    virtual ~EpanetMsxModel();
    
    virtual void solveSimulation(time_t time);
//    virtual time_t nextHydraulicStep(time_t time);
    virtual void stepSimulation(time_t time);
    
    void loadMsxFile(std::string file);
    std::string msxFile();
    
  private:
    std::string _msxFile;
    
  };
  
}



#endif /* defined(__epanet_rtx__EpanetMsxModel__) */
