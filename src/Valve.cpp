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

Valve::Valve(const std::string& name, Node::sharedPointer startNode, Node::sharedPointer endNode) : Pipe(name, startNode, endNode){
setType(VALVE);
}
Valve::~Valve() {

}
