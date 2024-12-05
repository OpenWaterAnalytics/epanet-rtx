//
//  Element.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#include "Element.h"

using namespace RTX;
using namespace TSF;

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

void Element::setRecord(PointRecord::_sp record) {
  std::cerr << "base class called! error!" << std::endl;
}

template <typename T>
void Element::setMetadataValue(const std::string& name, const T& value) {
    _metadata[name] = value;
}

void Element::removeMetadataValue(const std::string& name) {
  _metadata.erase(name);
}

const RTX::Element::MetadataValueType& Element::getMetadataValue(const std::string& name) const {
  return _metadata.at(name);
}

std::vector<std::string> Element::getMetadataKeys() {
  std::vector<std::string> keys;
  for (auto kv : _metadata) {
    keys.push_back(kv.first);
  }
  return keys;
}

std::ostream& RTX::operator<< (std::ostream &out, Element &e) {
  return e.toStream(out);
}

std::ostream& Element::toStream(std::ostream &stream) {
  stream << "Element: \"" << this->name() << "\"\n";
  return stream;
}

template void Element::setMetadataValue(const std::string& name, const double&);
template void Element::setMetadataValue(const std::string& name, const std::string&);
template void Element::setMetadataValue(const std::string& name, const std::shared_ptr<TimeSeries>&);
