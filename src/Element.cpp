//
//  Element.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See Readme.txt and license.txt for more information
//  http://tinyurl.com/epanet-rtx

#include "Element.h"

using namespace RTX;

Element::Element(const std::string& name) {
  setName(name);
}
Element::~Element() {
  
}

std::string Element::name() {
  return _name;
}

void Element::setName(const std::string& name) {
  _name = name;
}

Element::element_t Element::type() {
  return _type;
}

void Element::setType(element_t type) {
  _type = type;
}

void Element::setRecord(PointRecord::sharedPointer record) {
  std::cerr << "base class called! error!" << std::endl;
}
