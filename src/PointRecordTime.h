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

#ifdef check
#undef check
#endif  // this is to prevent mac debugger core from conflicting with boost macros
#include <boost/date_time/local_time/local_time.hpp>

namespace RTX {
  class PointRecordTime {
    
  public:
    typedef enum { UTC = 0, LOCAL = 1 } time_format_t;
    static tm tmFromSql(SQL_TIMESTAMP_STRUCT sqlTime);
    static time_t time(SQL_TIMESTAMP_STRUCT sqlTime);
    static time_t timeFromZone(SQL_TIMESTAMP_STRUCT sqlTime, const boost::local_time::time_zone_ptr& localtz);
    static SQL_TIMESTAMP_STRUCT sqlTime(time_t uTime, time_format_t format = UTC);
    static std::string localDateStringFromUnix(time_t unixTime, const boost::local_time::time_zone_ptr& localtz);
    static std::string utcDateStringFromUnix(time_t unixTime);
  };
}





#endif /* defined(__epanet_rtx__PointRecordTime__) */
