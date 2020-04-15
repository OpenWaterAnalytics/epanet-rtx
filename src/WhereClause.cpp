//
//  WhereClause.cpp
//  epanet-rtx static deps
//
//  Created by Sam Hatchett on 4/14/20.
//

#include "WhereClause.h"

using namespace std;
using namespace RTX;

bool filterWithComparison(const double &value, const double &reference_value, const WhereClause::clause_t &clause_type);
bool filterWithComparison(const double &value, const double &reference_value, const WhereClause::clause_t &clause_type) {
  switch (clause_type) {
    case WhereClause::gt:
      if (value <= reference_value) return false;
      break;
    case WhereClause::gte:
      if (value < reference_value) return false;
      break;
    case WhereClause::lt:
      if (value >= reference_value) return false;
      break;
    case WhereClause::lte:
      if (value > reference_value) return false;
      break;
    default:
      break;
  }
  return true;
}

bool WhereClause::filter(const Point &p) {
  const double v = p.value;
  auto i = this->clauses.cbegin();
  while (i != this->clauses.cend()) {
    const double ref = i->second;
    if (!filterWithComparison(v, ref, i->first))
      return false;
    ++i;
  }
  return true;
}
