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

#ifdef check
#undef check
#endif  // this is to prevent mac debugger core from conflicting with boost macros
#include <boost/date_time/local_time/local_time.hpp>

namespace RTX {
  class PointRecordTime {
    
  public:
    typedef enum { UTC = 0, LOCAL = 1 } time_format_t;
    static std::string localDateStringFromUnix(time_t unixTime, const boost::local_time::time_zone_ptr& localtz);
    static std::string utcDateStringFromUnix(time_t unixTime, const char *format = "%Y-%m-%d %H:%M:%S");
    
    static time_t timeFromIso8601(const std::string& tstr);
    
  };
}





#endif /* defined(__epanet_rtx__PointRecordTime__) */
