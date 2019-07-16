#ifndef IdentifierUnitsList_hpp
#define IdentifierUnitsList_hpp

#include <stdio.h>

#include <string>
#include <map>
#include "Units.h"

namespace RTX {
  class IdentifierUnitsList {
  public:
    IdentifierUnitsList();
    bool hasIdentifierAndUnits(const std::string& identifier, const Units& units);
    std::pair<bool,bool> doesHaveIdUnits(const std::string& identifier, const Units& units);
    void set(const std::string& identifier, const Units& units);
    std::map< std::string, std::pair<Units, std::string> > *get();
    void clear();
    size_t count();
    bool empty();
  private:        // ID => (units, units_str)
    std::shared_ptr< std::map< std::string, std::pair<Units, std::string> > > _d;
  };
}

#endif /* IdentifierUnitsList_hpp */
