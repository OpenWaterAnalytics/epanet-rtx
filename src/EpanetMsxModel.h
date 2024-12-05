//
//  EpanetMsxModel.h
//  epanet-rtx
//
//  Open Water Analytics [wateranalytics.org]
//  See README.md and license.txt for more information
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
    TSF_BASE_PROPS(EpanetMsxModel);
    EpanetMsxModel();
    virtual ~EpanetMsxModel();
    
    virtual bool solveSimulation(time_t time);
//    virtual time_t nextHydraulicStep(time_t time);
    virtual void stepSimulation(time_t time);
    
    void loadMsxFile(std::string file);
    std::string msxFile();
    
  private:
    std::string _msxFile;
    
  };
  
}



#endif /* defined(__epanet_rtx__EpanetMsxModel__) */
