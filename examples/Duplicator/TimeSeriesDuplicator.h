#ifndef TimeSeriesDuplicator_h
#define TimeSeriesDuplicator_h

#include "rtxMacros.h"
#include "PointRecord.h"
#include "TimeSeries.h"
#include <boost/signals2/mutex.hpp>
#include <boost/thread/thread_only.hpp>
#include <boost/any.hpp>

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
    typedef std::function<void(const char*)> RTX_Duplicator_log_callback;
    
    TimeSeriesDuplicator();
    
    PointRecord::_sp destinationRecord();
    void setDestinationRecord(PointRecord::_sp record);
    
    std::list<TimeSeries::_sp> series();
    void setSeries(std::list<TimeSeries::_sp> series); /// source series !
    
    // view / change state
    void catchUpAndRun(time_t fetchWindow, time_t frequency, time_t backfill, time_t chunkSize = 24*60*60, time_t rateLimit = 0);
    void run(time_t fetchWindow, time_t frequency); /// run starting now
    void runRetrospective(time_t start, time_t retroChunkSize, time_t rateLimit = 0); // catch up to current and stop
    void stop();
    bool isRunning();
    double pctCompleteFetch();
    void wait();
    
    // logging
    void setLoggingFunction(RTX_Duplicator_log_callback);
    RTX_Duplicator_log_callback loggingFunction();
    
    int logLevel;
    volatile bool _shouldRun;
    
  private:
    boost::signals2::mutex _mutex;
    RTX_Duplicator_log_callback _logFn;
    PointRecord::_sp _destinationRecord;
    std::list<TimeSeries::_sp> _sourceSeries, _destinationSeries;
    std::pair<time_t,int> _fetchAll(time_t start, time_t end, bool updatePctComplete = true);
    double _pctCompleteFetch;
    bool _isRunning;
    void _refreshDestinations();
    void _logLine(const std::string& str, int level);
    
    void _dupeLoop(time_t win, time_t freq);
    void _backfillLoop(time_t start, time_t chunk, time_t rateLimit = 0);
    void _catchupLoop(time_t fetchWindow, time_t frequency, time_t backfill, time_t chunkSize = 24*60*60, time_t rateLimit = 0);
    boost::thread _dupeBackground;
    
    void sendMetric(const std::string &metric, const std::string &field, const boost::any &value);
    
  };
}



#endif /* TimeSeriesDuplicator_h */
