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
    std::map< std::string, Units> *get();
    void clear();
    size_t count();
    bool empty();
  private:
    std::shared_ptr< std::map< std::string, Units> > _d;
  };
}

#endif /* IdentifierUnitsList_hpp */
