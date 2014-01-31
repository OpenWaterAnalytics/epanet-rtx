//
//  validator.cpp
//  rtx-validator
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#include <iostream>
#include <time.h>
#include <boost/foreach.hpp>

#include "ConfigProject.h"

#include "EpanetModel.h"

using namespace std;
using namespace RTX;

//void runSimulationUsingConfig(const string& path, time_t someTime, long duration);

int main (int argc, const char * argv[])
{
  string forwardSimulationConfig(""), realtimeConfig("");
  if (argc > 1) {
    forwardSimulationConfig = string( argv[1] );
  }
  if (argc > 2) {
    realtimeConfig = string( argv[2] );
  }
  
  time_t someTime = 1222873200; // unix-time 2008-10-01 15:00:00 GMT
  long duration = 60 * 60 * 24; // 1 day
  
  forwardSimulationConfig = "/Users/sam/Copy/Code/epanet-rtx/examples/validator/sampletown_synthetic.cfg";
  
  EpanetModel::sharedPointer model( new EpanetModel );
  model->loadModelFromFile("/Users/sam/Copy/Code/epanet-rtx/examples/validator/sampletown.inp");
  
  vector<Pipe::sharedPointer> pipes = model->pipes();
  BOOST_FOREACH(Pipe::sharedPointer p, pipes) {
    if (RTX_STRINGS_ARE_EQUAL(p->name(), "4")) {
      TimeSeries::sharedPointer flow( new TimeSeries );
      flow->setUnits(RTX_GALLON_PER_MINUTE);
      p->setFlowMeasure(flow);
    }
  }
  
  model->initDMAs();
  
  // test the forward synthetic simulation
  //runSimulationUsingConfig(forwardSimulationConfig, someTime-3600, duration+7200);
  
  realtimeConfig = "/Users/sam/Copy/Code/epanet-rtx/examples/validator/sampletown_realtime.cfg";
  
  // test the real-time methods
//  runSimulationUsingConfig(realtimeConfig, someTime, duration);
  
  
  return 0;
}



//
//void runSimulationUsingConfig(const string& filePath, time_t someTime, long duration) {
//  
//  ConfigProject config;
//  Model::sharedPointer model;
//  
//  try {
//    config.loadConfigFile(filePath);
//    
//    model = config.model();
//    
//    cout << "RTX: Running simulation for..." << endl;
//    cout << *model;
//    
//    PointRecord::sharedPointer record = config.defaultRecord();
//  
//    model->runExtendedPeriod(someTime, someTime + duration);
//    
//    cout << "... done" << endl;
//    
//  } catch (string err) {
//    cerr << err << endl;
//  }
//}



