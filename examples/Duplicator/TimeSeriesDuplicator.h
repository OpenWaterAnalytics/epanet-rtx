#ifndef TimeSeriesDuplicator_h
#define TimeSeriesDuplicator_h


#include "PointRecord.h"
#include "TimeSeries.h"
#include <boost/signals2/mutex.hpp>

#include <stdio.h>
#include <list>

namespace RTX {
  class TimeSeriesDuplicator {
    
  public:
    typedef void (^RTX_Duplicator_Logging_Callback_Block)(const char *msg);
    
    PointRecord::_sp destinationRecord();
    void setDestinationRecord(PointRecord::_sp record);
    
    std::list<TimeSeries::_sp> series();
    void setSeries(std::list<TimeSeries::_sp> series);
    
    // view / change state
    void run();
    void runWithRetrospective(time_t start, time_t chunkSize);
    void stop();
    bool isRunning();
    double pctCompleteFetch();
    
    // fetch parameters
    time_t fetchWindow();
    void setFetchWindow(time_t seconds);
    time_t fetchFrequency();
    void setFetchFrequency(time_t seconds);
    
    // logging
    void setLoggingFunction(RTX_Duplicator_Logging_Callback_Block);
    RTX_Duplicator_Logging_Callback_Block loggingFunction();
    
    
  private:
    boost::signals2::mutex _mutex;
    time_t _fetchWindow, _fetchFrequency;
    RTX_Duplicator_Logging_Callback_Block _loggingFn;
    PointRecord::_sp _destinationRecord;
    std::list<TimeSeries::_sp> _sourceSeries, _destinationSeries;
    double _pctCompleteFetch;
    bool _isRunning;
    void _refreshDestinations();
  };
}



#endif /* TimeSeriesDuplicator_h */
