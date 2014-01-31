//
//  OdbcPreparedPointRecord.h
//  epanet-rtx
//
//  Created by Sam Hatchett on 12/31/13.
//
//

#ifndef __epanet_rtx__OdbcPreparedPointRecord__
#define __epanet_rtx__OdbcPreparedPointRecord__

#include <iostream>
#include "OdbcPreparedPointRecord.h"

namespace RTX {
  class OdbcPreparedPointRecord : public OdbcPointRecord {
    
  private:
    void bindOutputColumns(SQLHSTMT statement, ScadaRecord* record);

    
  };
}



#endif /* defined(__epanet_rtx__OdbcPreparedPointRecord__) */
