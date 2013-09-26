//
//  main.cpp
//  rtxNetSim
//
//  Created by Sam Hatchett on 9/26/13.
//
//

#include <iostream>
#include "ConfigFactory.h"

using namespace std;
using namespace RTX;

int main(int argc, const char * argv[])
{
  
  time_t someTime = 1353330000; // unix-time 2012-11-19 8:00:00 EST
  long duration = 60 * 60 * 24; // 1 day
  
  string forwardSimulationConfig("");
  if (argc > 1) {
    forwardSimulationConfig = string( argv[1] );
  }
  
  
  
  ConfigFactory config;
  Model::sharedPointer model;
  
  try {
    config.loadProjectFile(forwardSimulationConfig);
    
    model = config.model();
    
    cout << "RTX: Running simulation for..." << endl;
    cout << *model;
    
    PointRecord::sharedPointer record = config.defaultRecord();
    
    model->runExtendedPeriod(someTime, someTime + duration);
    
    cout << "... done" << endl;
    
  } catch (string err) {
    cerr << err << endl;
  }
  
}

