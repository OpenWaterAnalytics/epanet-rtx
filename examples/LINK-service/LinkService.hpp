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
using web::http::experimental::listener::http_listener;

namespace RTX {
  class LinkService {
    
  public:
    typedef std::map< std::string, std::function<void(http_request)> > Responders;
    
    LinkService(web::uri address);
    pplx::task<void> open();
    pplx::task<void> close();
    
  private:
    void _get(http_request message);
    void _put(http_request message);
    void _post(http_request message);
    void _delete(http_request message);
    
    void _get_timeseries(http_request message);
    void _get_runState(http_request message);
    void _get_source(http_request message);
    void _get_odbc_drivers(http_request message);
    
    void _post_timeseries(http_request message);
    void _post_runState(http_request message);
    void _post_source(http_request message);
    
    http_listener _listener;
    
    std::vector<RTX::TimeSeries::_sp> _tsList;
    
    Responders _LinkService_GET_responders();
    Responders _LinkService_POST_responders();
    
    TimeSeriesDuplicator _duplicator;
    PointRecord::_sp _sourceRecord;
    
    void runDuplication(time_t win, time_t freq);
    void stopDuplication();
        
    std::string _statusMessage;
    
  };
}



#endif /* LinkService_hpp */
