//
//  PointRecordTime.h
//  epanet-rtx
//
//  Open Water Analytics [wateranalytics.org]
//  See README.md and license.txt for more information
//

#ifndef __epanet_rtx__PointRecordTime__
#define __epanet_rtx__PointRecordTime__

#include <iostream>
#include <time.h>
#include <sqltypes.h>

namespace RTX {
  class PointRecordTime {
    
  public:
    typedef enum { LOCAL, UTC } time_format_t;
    static time_t time(SQL_TIMESTAMP_STRUCT sqlTime);
    static SQL_TIMESTAMP_STRUCT sqlTime(time_t uTime, time_format_t format = UTC);
    static std::string utcDateStringFromUnix(time_t unixTime);
  };
}





#endif /* defined(__epanet_rtx__PointRecordTime__) */
