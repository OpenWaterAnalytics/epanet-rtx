//
//  EpanetSyntheticModel.h
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#ifndef epanet_rtx_EpanetSyntheticModel_h
#define epanet_rtx_EpanetSyntheticModel_h

#include "EpanetModel.h"

namespace RTX {
  
  /*!
   \class EpanetSyntheticModel
   \brief Forward-simluation model object, based on epanet engine
   
   Provides an epanet-based simulation engine and methods to perform a forward simulation without overriding rules and controls.
   
   */
  
  class EpanetSyntheticModel : public EpanetModel {
    
  public:
    EpanetSyntheticModel();
    virtual void overrideControls() throw(RtxException);
    virtual std::ostream& toStream(std::ostream &stream);
    
  protected:
    virtual void solveSimulation(time_t time);
    virtual time_t nextHydraulicStep(time_t time);
  private:
    time_t _startTime;
  };
} // namespace RTX

#endif
