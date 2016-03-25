//
#ifndef FluxService_hpp
#define FluxService_hpp

#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <fstream>
#include <random>

#include <stdio.h>

#include <cpprest/json.h>
#include <cpprest/http_listener.h>
#include <cpprest/uri.h>
#include <cpprest/asyncrt_utils.h>

#include "TimeSeries.h"
#include "InfluxdbPointRecord.h"
#include "OdbcPointRecord.h"

using web::http::http_request;
using web::http::experimental::listener::http_listener;

namespace RTX {
  class FluxService {
    
  public:
    FluxService(web::uri address);
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
    
    http_listener _listener;
    
    std::vector<RTX::TimeSeries::_sp> _tsList;
    
    typedef std::function<void(http_request)> responderFunction;
    std::map<std::string, responderFunction> _FluxService_responders();
    
    
    
    
  };
}


#endif /* FluxService_hpp */
