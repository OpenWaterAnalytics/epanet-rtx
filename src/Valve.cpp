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

Valve::Valve(const std::string& name, Node::_sp startNode, Node::_sp endNode) : Pipe(name, startNode, endNode){
setType(VALVE);
}
Valve::~Valve() {

}
