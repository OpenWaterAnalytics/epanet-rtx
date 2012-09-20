//
//  Valve.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See Readme.txt and license.txt for more information
//  https://sourceforge.net/projects/epanet-rtx/

#include <iostream>

#include "Valve.h"

using namespace RTX;

Valve::Valve(const std::string& name, Node::sharedPointer startNode, Node::sharedPointer endNode) : Pipe(name, startNode, endNode){
setType(VALVE);
_doesHaveSettingParameter = false;
}
Valve::~Valve() {

}

bool Valve::doesHaveSettingParameter() {
return _doesHaveSettingParameter;
}

TimeSeries::sharedPointer Valve::settingParameter() {
return _setting;
}

void Valve::setSettingParameter(TimeSeries::sharedPointer setting) {
_doesHaveSettingParameter = (setting ? true : false);
_setting = setting;
}
