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

#include "SqliteProjectFile.h"

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
  
  forwardSimulationConfig = "/Users/sam/Desktop/gcww.rtx";
  
  
  SqliteProjectFile::_sp config(new SqliteProjectFile);
  config->loadProjectFile(forwardSimulationConfig);
  
  
  
  // test the real-time methods
//  runSimulationUsingConfig(realtimeConfig, someTime, duration);
  
  
  return 0;
}



//
//void runSimulationUsingConfig(const string& filePath, time_t someTime, long duration) {
//  
//  ConfigProject config;
//  Model::_sp model;
//  
//  try {
//    config.loadConfigFile(filePath);
//    
//    model = config.model();
//    
//    cout << "RTX: Running simulation for..." << endl;
//    cout << *model;
//    
//    PointRecord::_sp record = config.defaultRecord();
//  
//    model->runExtendedPeriod(someTime, someTime + duration);
//    
//    cout << "... done" << endl;
//    
//  } catch (string err) {
//    cerr << err << endl;
//  }
//}



