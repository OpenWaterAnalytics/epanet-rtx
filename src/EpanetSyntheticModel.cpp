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


void EpanetSyntheticModel::overrideControls() {
  // make sure we do nothing.
  std::cerr << "ignoring control override" << std::endl;
}



// mostly copied from the epanetmodel class, but altered epanet clock-setting
// so that the simulation evolves with its builtin controlls and patterns.

bool EpanetSyntheticModel::solveSimulation(time_t time) {
  bool success = true;
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
  //EN_API_CHECK(ENsettimeparam(EN_HTIME, 0), "ENsettimeparam(EN_HTIME)");
  
  // solve the hydraulics
  EN_API_CHECK(EN_runH(_enModel, &timestep), "EN_runH");
  if (this->shouldRunWaterQuality()) {
    EN_API_CHECK(EN_runQ(_enModel, &timestep), "EN_runQ");
  }
  
  return success;
}

// adjust duration of epanet toolkit simulation if necessary.
time_t EpanetSyntheticModel::nextHydraulicStep(time_t time) {
  
  long duration = 0;
  EN_API_CHECK(EN_gettimeparam(_enModel, EN_DURATION, &duration), "EN_gettimeparam(EN_DURATION)");
  
  if (duration <= (time - _startTime) ) {
    //std::cerr << "had to adjust the sim duration to accomodate the requested step" << std::endl;
    EN_API_CHECK(EN_settimeparam(_enModel, EN_DURATION, (duration + hydraulicTimeStep()) ), "EN_settimeparam(EN_DURATION)");
  }
  
  // call base class method
  return EpanetModel::nextHydraulicStep(time);
  
}
