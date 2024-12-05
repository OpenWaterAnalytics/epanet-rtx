//
//  EpanetMsxModel.cpp
//  epanet-rtx
//
//  Open Water Analytics [wateranalytics.org]
//  See README.md and license.txt for more information
//


#include "EpanetMsxModel.h"

using namespace RTX;
using namespace TSF;
using namespace std;

EpanetMsxModel::EpanetMsxModel() {
  
}

EpanetMsxModel::~EpanetMsxModel() {
  
}

void EpanetMsxModel::loadMsxFile(std::string file) {
  
  _msxFile = file;
  MSXopen((char*)file.c_str());
  
}


bool EpanetMsxModel::solveSimulation(time_t time) {
  bool success = EpanetModel::solveSimulation(time);
  return success;  
  
}

void EpanetMsxModel::stepSimulation(time_t time) {
  EpanetModel::stepSimulation(time);
  
  long t, tleft;
  MSXstep(&t, &tleft);
  
  
}

