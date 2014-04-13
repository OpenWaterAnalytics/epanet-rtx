//
//  OdbcPreparedPointRecord.h
//  epanet-rtx
//
//  Open Water Analytics [wateranalytics.org]
//  See README.md and license.txt for more information
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
