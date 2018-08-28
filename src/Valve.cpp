//
//  Valve.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#include <iostream>

#include "Valve.h"

using namespace RTX;

Valve::Valve(const std::string& name) : Pipe(name){
  setType(VALVE);
}
Valve::~Valve() {

}
