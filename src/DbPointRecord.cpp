//
//  DbPointRecord.cpp
//  epanet-rtx
//
//  Created by Sam Hatchett on 1/31/13.
//
//

#include "DbPointRecord.h"

using namespace RTX;

DbPointRecord::DbPointRecord() {
  _timeFormat = UTC;
}

DbPointRecord::Hint_t::Hint_t() {
  this->clear();
}

void DbPointRecord::Hint_t::clear() {
  //std::cout << "clearing scada point cache" << std::endl;
  identifier = "";
  range.first = 0;
  range.second = 0;
}