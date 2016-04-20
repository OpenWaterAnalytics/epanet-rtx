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
#include "InfluxdbPointRecord.h"
#include "OdbcPointRecord.h"

#include "LinkJsonSerialization.hpp"

#include "TimeSeriesDuplicator.h"

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
    
  private:
    void _get(http_request message);
    void _put(http_request message);
    void _post(http_request message);
    void _delete(http_request message);
    
    void _get_ping(http_request message);
    void _get_timeseries(http_request message);
    void _get_runState(http_request message);
    void _get_source(http_request message);
    void _get_destination(http_request message);
    void _get_odbc_drivers(http_request message);
    void _get_units(http_request message);
    void _get_options(http_request message);
    void _get_config(http_request message);
    
    http_response _post_config(web::json::value json);
    http_response _post_timeseries(web::json::value json);
    http_response _post_runState(web::json::value json);
    http_response _post_options(web::json::value json);
    http_response _post_source(web::json::value json);
    http_response _post_destination(web::json::value json);
    
    http_listener _listener;
    
    std::vector<RTX::TimeSeries::_sp> _tsList;
        
    TimeSeriesDuplicator _duplicator;
    PointRecord::_sp _sourceRecord, _destinationRecord;
    
    void runDuplication(time_t win, time_t freq, time_t backfill);
    void stopDuplication();
    time_t _window, _frequency, _backfill;
    
    
  };
}



#endif /* LinkService_hpp */
