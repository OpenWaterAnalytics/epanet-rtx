//
//  PointRecordTime.cpp
//  epanet-rtx
//
//  Created by Sam Hatchett on 1/6/14.
//
//

#include "PointRecordTime.h"
#include <boost/date_time/posix_time/posix_time.hpp>

using namespace RTX;
using namespace std;


time_t PointRecordTime::time(SQL_TIMESTAMP_STRUCT sqlTime) {
  
  boost::posix_time::ptime pTimeEpoch = boost::posix_time::ptime(boost::gregorian::date(1970,1,1));
  
  time_t uTime;
  struct tm tmTime;
  
  tmTime.tm_year = sqlTime.year - 1900;
  tmTime.tm_mon = sqlTime.month -1;
  tmTime.tm_mday = sqlTime.day;
  tmTime.tm_hour = sqlTime.hour;
  tmTime.tm_min = sqlTime.minute;
  tmTime.tm_sec = sqlTime.second;
  
  // Portability note: mktime is essentially universally available. timegm is rather rare. fortunately, boost has some capabilities here.
  // uTime = timegm(&tmTime);
  
  boost::posix_time::ptime pt = boost::posix_time::ptime_from_tm(tmTime);
  boost::posix_time::time_duration::sec_type x = (pt - pTimeEpoch).total_seconds();
  uTime = time_t(x);
  
  // back convert for a check
  SQL_TIMESTAMP_STRUCT sqlTimeCheck = PointRecordTime::sqlTime(uTime);
  if ( sqlTimeCheck.year == sqlTime.year &&
      sqlTimeCheck.month == sqlTime.month &&
      sqlTimeCheck.day == sqlTime.day &&
      sqlTimeCheck.hour == sqlTime.hour &&
      sqlTimeCheck.minute == sqlTime.minute &&
      sqlTimeCheck.second == sqlTime.second) {
  }
  else {
    cerr << "time not formed correctly" << endl;
  }
  
  return uTime;
}



SQL_TIMESTAMP_STRUCT PointRecordTime::sqlTime(time_t uTime, time_format_t format) {
  
  SQL_TIMESTAMP_STRUCT sqlTimestamp;
  struct tm myTMstruct;
  struct tm* pTMstruct = &myTMstruct;
  
  // time format (local/utc)
  if (format == PointRecordTime::UTC) {
    pTMstruct = gmtime(&uTime);
  }
  else if (format == PointRecordTime::LOCAL) {
    pTMstruct = localtime(&uTime);
  }
	
	sqlTimestamp.year = pTMstruct->tm_year + 1900;
	sqlTimestamp.month = pTMstruct->tm_mon + 1;
	sqlTimestamp.day = pTMstruct->tm_mday;
	sqlTimestamp.hour = pTMstruct->tm_hour;
  sqlTimestamp.minute = pTMstruct->tm_min;
	sqlTimestamp.second = pTMstruct->tm_sec;
	sqlTimestamp.fraction = (SQLUINTEGER)0;
  
  return sqlTimestamp;
}



string PointRecordTime::utcDateStringFromUnix(time_t unixTime) {
  
  boost::posix_time::ptime pt = boost::posix_time::from_time_t(unixTime);
  string s;
  ostringstream datetime_ss;
  boost::posix_time::time_facet * p_time_output = new boost::posix_time::time_facet;
  locale special_locale (std::locale(""), p_time_output);
  // special_locale takes ownership of the p_time_output facet
  datetime_ss.imbue (special_locale);
  (*p_time_output).format("%Y-%m-%d %H:%M:%S"); // date time
  datetime_ss << pt;
  s = datetime_ss.str().c_str();
  // don't explicitly delete p_time_output
  return s;
  
}



