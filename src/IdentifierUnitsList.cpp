#include "IdentifierUnitsList.h"


using namespace std;
using namespace RTX;


IdentifierUnitsList::IdentifierUnitsList() {
  _d.reset(new map<string,Units>);
}
bool IdentifierUnitsList::hasIdentifierAndUnits(const std::string &identifier, const RTX::Units &units) {
  auto pr = this->doesHaveIdUnits(identifier, units);
  return pr.first && pr.second;
}
pair<bool,bool> IdentifierUnitsList::doesHaveIdUnits(const string& identifier, const Units& units) {
  bool nameExists = false, unitsMatch = false;
  map<string,Units>::const_iterator found = _d->find(identifier);
  if (found != _d->end()) {
    nameExists = true;
    const Units existingUnits = found->second;
    if (existingUnits == units) {
      unitsMatch = true;
    }
  }
  return make_pair(nameExists,unitsMatch);
}


map<string,Units>* IdentifierUnitsList::get() {
  return _d.get();
}
void IdentifierUnitsList::set(const std::string &identifier, const RTX::Units &units) {
  (*_d.get())[identifier] = units;
}
void IdentifierUnitsList::clear() {
  _d.reset(new map<string,Units>);
}
size_t IdentifierUnitsList::count() {
  return _d->size();
}
bool IdentifierUnitsList::empty() {
  return _d->empty();
}
