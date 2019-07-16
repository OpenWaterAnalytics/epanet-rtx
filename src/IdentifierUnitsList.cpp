#include "IdentifierUnitsList.h"


using namespace std;
using namespace RTX;


IdentifierUnitsList::IdentifierUnitsList() {
  _d.reset(new map<string,pair<Units,string> >);
}
bool IdentifierUnitsList::hasIdentifierAndUnits(const std::string &identifier, const RTX::Units &units) {
  auto pr = this->doesHaveIdUnits(identifier, units);
  return pr.first && pr.second;
}
pair<bool,bool> IdentifierUnitsList::doesHaveIdUnits(const string& identifier, const Units& units) {
  bool nameExists = false, unitsMatch = false;
  map<string,pair<Units,string> >::const_iterator found = _d->find(identifier);
  if (found != _d->end()) {
    nameExists = true;
    const Units existingUnits = found->second.first;
    if (existingUnits == units) {
      unitsMatch = true;
    }
  }
  return make_pair(nameExists,unitsMatch);
}


map<string, pair<Units,string> >* IdentifierUnitsList::get() {
  return _d.get();
}
void IdentifierUnitsList::set(const std::string &identifier, const RTX::Units &units) {
  (*_d.get())[identifier] = {units, units.to_string()};
}
void IdentifierUnitsList::clear() {
  _d.reset(new map<string,pair<Units,string> >);
}
size_t IdentifierUnitsList::count() {
  return _d->size();
}
bool IdentifierUnitsList::empty() {
  return _d->empty();
}
