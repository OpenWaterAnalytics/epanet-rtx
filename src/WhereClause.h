//
//  WhereClause.hpp
//  epanet-rtx static deps
//
//  Created by Sam Hatchett on 4/14/20.
//

#ifndef WhereClause_hpp
#define WhereClause_hpp

#include <stdio.h>
#include <map>
#include <time.h>

#include "Point.h"

namespace RTX {

class WhereClause {
public:
  typedef enum {
    gt, gte,
    lt, lte
  } clause_t;
  
  std::map<clause_t,double> clauses;
  
  bool filter(const Point& p);
  
};

}
#endif /* WhereClause_hpp */
