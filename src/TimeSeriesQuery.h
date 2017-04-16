#ifndef TimeSeriesQuery_h
#define TimeSeriesQuery_h

#include <stdio.h>

#include "TimeSeries.h"
#include "DbPointRecord.h"


namespace RTX {
  class TimeSeriesQuery : public TimeSeries {
  public:
    RTX_BASE_PROPS(TimeSeriesQuery);
    TimeSeriesQuery();
    
    Point pointBefore(time_t time);
    Point pointAfter(time_t time);
    std::vector< Point > points(TimeRange range);
    
    PointRecord::_sp record();
    void setRecord(PointRecord::_sp record);
    
    std::string query();
    void setQuery(const std::string& query);
    
  private:
    DbPointRecord::_sp _qRecord;
    std::string _query;
  };
}


#endif /* TimeSeriesQuery_h */
