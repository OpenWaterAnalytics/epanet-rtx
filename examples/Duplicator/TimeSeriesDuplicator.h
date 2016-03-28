#ifndef TimeSeriesDuplicator_h
#define TimeSeriesDuplicator_h

#include "rtxMacros.h"
#include "PointRecord.h"
#include "TimeSeries.h"
#include <boost/signals2/mutex.hpp>

#include <stdio.h>
#include <list>

#define RTX_DUPLICATOR_LOGLEVEL_ERROR   0
#define RTX_DUPLICATOR_LOGLEVEL_WARN    1
#define RTX_DUPLICATOR_LOGLEVEL_INFO    2
#define RTX_DUPLICATOR_LOGLEVEL_VERBOSE 3


namespace RTX {
  class TimeSeriesDuplicator : RTX_object {
    
  public:
    RTX_BASE_PROPS(TimeSeriesDuplicator);
    typedef void (^RTX_Duplicator_Logging_Callback_Block)(const char *msg);
    
    TimeSeriesDuplicator();
    
    PointRecord::_sp destinationRecord();
    void setDestinationRecord(PointRecord::_sp record);
    
    std::list<TimeSeries::_sp> series();
    void setSeries(std::list<TimeSeries::_sp> series); /// source series !
    
    // view / change state
    void run(time_t fetchWindow, time_t frequency); /// run starting now
    void runRetrospective(time_t start, time_t retroChunkSize, time_t rateLimit = 0); // catch up to current and stop
    void stop();
    bool isRunning();
    double pctCompleteFetch();
    
    // logging
    void setLoggingFunction(RTX_Duplicator_Logging_Callback_Block);
    RTX_Duplicator_Logging_Callback_Block loggingFunction();
    
    int logLevel;
    volatile bool _shouldRun;
    
  private:
    boost::signals2::mutex _mutex;
    RTX_Duplicator_Logging_Callback_Block _loggingFn;
    PointRecord::_sp _destinationRecord;
    std::list<TimeSeries::_sp> _sourceSeries, _destinationSeries;
    std::pair<time_t,int> _fetchAll(time_t start, time_t end);
    double _pctCompleteFetch;
    bool _isRunning;
    void _refreshDestinations();
    void _logLine(const std::string& str, int level);
  };
}



#endif /* TimeSeriesDuplicator_h */
