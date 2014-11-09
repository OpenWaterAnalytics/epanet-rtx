//
//  Element.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#include "Element.h"

using namespace RTX;

Element::Element(const std::string& name) {
  setName(name);
  setUserDescription("");
}
Element::~Element() {
  
}

std::string Element::name() {
  return _name;
}

void Element::setName(const std::string& name) {
  _name = name;
}

void Element::setUserDescription(const std::string &description) {
  _userDescription = description;
}
std::string Element::userDescription() {
  return _userDescription;
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

std::ostream& RTX::operator<< (std::ostream &out, Element &e) {
  return e.toStream(out);
}

std::ostream& Element::toStream(std::ostream &stream) {
  stream << "Element: \"" << this->name() << "\"\n";
  return stream;
}