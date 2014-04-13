//
//  EpanetSyntheticModel.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#include <iostream>

#include "EpanetSyntheticModel.h"

using namespace RTX;

EpanetSyntheticModel::EpanetSyntheticModel() {
  _startTime = 0;
}

std::ostream& EpanetSyntheticModel::toStream(std::ostream &stream) {
  // epanet-specific printing
  stream << "Synthetic model (no exogenous inputs)" << std::endl;
  EpanetModel::toStream(stream);
  return stream;
}


void EpanetSyntheticModel::overrideControls() throw(RtxException) {
  // make sure we do nothing.
  std::cerr << "ignoring control override" << std::endl;
}



// mostly copied from the epanetmodel class, but altered epanet clock-setting
// so that the simulation evolves with its builtin controlls and patterns.

void EpanetSyntheticModel::solveSimulation(time_t time) {
  // this should only fire once, at the first solution period.
  if (_startTime == 0) {
    _startTime = time;
  }
  
  // TODO -- 
  /*
   determine how to perform this function. maybe keep track of relative error for each simulation period
   so we know what simulation periods have been solved - as in a master simulation clock?
   */
  long timestep;
  
  setCurrentSimulationTime( time );
  
  // don't do this. only the real-time epanetmodel does this.
  //ENcheck(ENsettimeparam(EN_HTIME, 0), "ENsettimeparam(EN_HTIME)");
  
  // solve the hydraulics
  ENcheck(ENrunH(&timestep), "ENrunH");
  if (this->shouldRunWaterQuality()) {
    ENcheck(ENrunQ(&timestep), "ENrunQ");
  }
}

// adjust duration of epanet toolkit simulation if necessary.
time_t EpanetSyntheticModel::nextHydraulicStep(time_t time) {
  
  long duration = 0;
  ENcheck(ENgettimeparam(EN_DURATION, &duration), "ENgettimeparam(EN_DURATION)");
  
  if (duration <= (time - _startTime) ) {
    //std::cerr << "had to adjust the sim duration to accomodate the requested step" << std::endl;
    ENcheck(ENsettimeparam(EN_DURATION, (duration + hydraulicTimeStep()) ), "ENsettimeparam(EN_DURATION)");
  }
  
  // call base class method
  return EpanetModel::nextHydraulicStep(time);
  
}
