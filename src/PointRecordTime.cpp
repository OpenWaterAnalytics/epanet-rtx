#include "PointRecordTime.h"
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/algorithm/string/replace.hpp>


using namespace RTX;
using namespace std;
using namespace boost::local_time;
using boost::posix_time::ptime;


string PointRecordTime::localDateStringFromUnix(time_t unixTime, const boost::local_time::time_zone_ptr& localtz) {
  
  boost::posix_time::ptime pt = boost::posix_time::from_time_t(unixTime);
  
  local_date_time localTime(pt,localtz);
  
  string s;
  ostringstream datetime_ss;
  boost::posix_time::time_facet * p_time_output = new boost::posix_time::time_facet;
  // locale takes ownership of the p_time_output facet
  datetime_ss.imbue (locale(datetime_ss.getloc(), p_time_output));
  (*p_time_output).format("%Y-%m-%d %H:%M:%S"); // date time
  datetime_ss << localTime.local_time();
  s = datetime_ss.str().c_str();
  // don't explicitly delete p_time_output
  return s;
  
}


string PointRecordTime::utcDateStringFromUnix(time_t unixTime, const char *fmt) {
  
  boost::posix_time::ptime pt = boost::posix_time::from_time_t(unixTime);
  string s;
  ostringstream datetime_ss;
  boost::posix_time::time_facet * p_time_output = new boost::posix_time::time_facet;
  locale special_locale (std::locale(""), p_time_output);
  // special_locale takes ownership of the p_time_output facet
  datetime_ss.imbue (special_locale);
  (*p_time_output).format(fmt); // date time
  datetime_ss << pt;
  s = datetime_ss.str().c_str();
  // don't explicitly delete p_time_output
  return s;
  
}


time_t PointRecordTime::timeFromIso8601(const std::string& tstr) {
  
  string t_str(tstr);
  boost::replace_all(t_str,"Z","");
  
  ptime pTimeEpoch = boost::posix_time::ptime(boost::gregorian::date(1970,1,1));
  ptime dateTime = boost::date_time::parse_delimited_time<ptime>(t_str, 'T');
  boost::posix_time::time_duration::sec_type x = (dateTime - pTimeEpoch).total_seconds();
  time_t uTime = time_t(x);
  return uTime;
}




