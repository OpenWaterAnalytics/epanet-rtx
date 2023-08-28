//
#ifndef LinkService_hpp
#define LinkService_hpp

#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <fstream>
#include <random>

#include <stdio.h>

#include <boost/thread/thread_only.hpp>

#include <cpprest/json.h>
#include <cpprest/http_listener.h>
#include <cpprest/uri.h>
#include <cpprest/asyncrt_utils.h>



#include "TimeSeries.h"
#include "TimeSeriesFilter.h"
#include "ConcreteDbRecords.h"

#include "LinkJsonSerialization.hpp"

#include "AutoRunner.hpp"

using web::http::http_request;
using web::http::http_response;
using web::http::experimental::listener::http_listener;

namespace RTX {
  class LinkService {
    
  public:    
    LinkService(web::uri address);
    pplx::task<void> open();
    pplx::task<void> close();
    std::string _statusMessage;
    void post_log(std::string metric, std::string field, std::string value);
    
  private:
    void _get(http_request message);
    void _put(http_request message);
    void _post(http_request message);
    void _delete(http_request message);
    
    http_response _get_ping(http_request message);
    http_response _get_timeseries(http_request message);
    http_response _get_runState(http_request message);
    http_response _get_source(http_request message);
    http_response _get_destination(http_request message);
    http_response _get_odbc_drivers(http_request message);
    http_response _get_units(http_request message);
    http_response _get_options(http_request message);
    http_response _get_config(http_request message);
    
    http_response _post_config(web::json::value json);
    http_response _post_timeseries(web::json::value json);
    http_response _post_runState(web::json::value json);
    http_response _post_options(web::json::value json);
    http_response _post_source(web::json::value json);
    http_response _post_destination(web::json::value json);
    http_response _post_analytics(web::json::value json);
    http_response _post_logmessage(web::json::value json);
    
    void refreshDestinationSeriesRecords();

    http_listener _listener;
    
    std::vector<RTX::TimeSeries::_sp> _sourceSeries;
    std::vector<RTX::TimeSeriesFilter::_sp> _destinationSeries;
    std::map<RTX::TimeSeries::_sp, RTX::TimeSeriesFilter::_sp> _translation;
        
    AutoRunner _runner;
    DbPointRecord::_sp _sourceRecord, _destinationRecord;
    
    void runDuplication();
    void stopDuplication();
    
    struct Options {
      int window, frequency, backfill, throttle;
      bool smart;
    } _options;
    
    //boost::signals2::mutex _dupeMutex;
    std::vector<TimeSeries::_sp> _analytics;
    
    void setDuplicatorSeries();
    
    struct MetricsSeries {
      TimeSeries::_sp count;
      TimeSeries::_sp time;
    } _metrics;
    
  };
}



#endif /* LinkService_hpp */
