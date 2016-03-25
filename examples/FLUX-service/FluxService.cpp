#include "FluxService.hpp"

using namespace std;
using namespace RTX;
using namespace web;
using namespace http;
using namespace utility;
using namespace http::experimental::listener;
using std::placeholders::_1;

#include <map>

map<string, FluxService::responderFunction> FluxService::_FluxService_responders() {
  map<string, responderFunction> r;
  r["series"] = bind(&FluxService::_get_timeseries, this, _1);
  r["run"]    = bind(&FluxService::_get_runState,   this, _1);
  r["source"] = bind(&FluxService::_get_source,     this, _1);
  return r;
}



#include "TimeSeriesFilter.h"


#include "Visitor.h"

class SerializerJson : public BaseVisitor, public Visitor<TimeSeries, int>,
                                           public Visitor<TimeSeriesFilter, int>
{
public:
  int visit(TimeSeries &ts) {
    cout << ts.name() << endl;
    return 0;
  };
  int visit(TimeSeriesFilter &ts) {
    cout << "derived" << endl;
    cout << ts.name() << endl;
    cout << ts.clock() << endl;
    return 0;
  };
};




FluxService::FluxService(uri uri) : _listener(uri) {
  _listener.support(methods::GET, std::bind(&FluxService::_get, this, std::placeholders::_1));
  _listener.support(methods::PUT, std::bind(&FluxService::_put, this, std::placeholders::_1));
  _listener.support(methods::POST, std::bind(&FluxService::_post, this, std::placeholders::_1));
  _listener.support(methods::DEL, std::bind(&FluxService::_delete, this, std::placeholders::_1));
  
  
  TimeSeries::_sp ts1(new TimeSeries);
  ts1->setName("FR.FR_LP_FLOW_1");
  ts1->setUnits(RTX_MILLION_GALLON_PER_DAY);
  
  TimeSeries::_sp ts2(new TimeSeries);
  ts2->setName("LP.LP_LL_LEV_FT");
  ts2->setUnits(RTX_FOOT);
  
  _tsList = {ts1,ts2};
  
}



pplx::task<void> FluxService::open() {
  return _listener.open();
}

pplx::task<void> FluxService::close() {
  return _listener.close();
}


void FluxService::_get(http_request message) {
  
  auto paths = http::uri::split_path(http::uri::decode(message.relative_uri().path()));
  
  if (paths.size() == 0) {
    message.reply(status_codes::NoContent);
    return;
  }
  
  string entryPoint = paths[0];
  
  map<string, responderFunction> responders = this->_FluxService_responders();
  if (responders.count(entryPoint)) {
    responders[entryPoint](message);
    return;
  }
  
  
  
  
  json::value pathsArrayV = json::value::array(paths.size());
  int i = 0;
  for (string p : paths) {
    pathsArrayV.as_array()[i] = json::value(p);
    ++i;
  }
  
  
  message.reply(status_codes::OK, pathsArrayV);
  
}

void FluxService::_put(http_request message) {
  
}

void FluxService::_post(http_request message) {
  
}

void FluxService::_delete(http_request message) {
  
}






#pragma mark - TS

void FluxService::_get_timeseries(http_request message) {
  
  TimeSeriesFilter::_sp tsF( new TimeSeriesFilter() );
  tsF->setName("blah ts filter");
  
  // return the list of time series:
  // [ {series:"ts1",units:"MGD"} , {series:"ts2",units:"FT"} ]
  
  SerializerJson jsVis;
  
  tsF.get()->accept(jsVis);
  
  
  int i=0;
  for (auto tsIt = _tsList.begin(); tsIt != _tsList.end(); ++tsIt, ++i) {
    TimeSeries::_sp ts = *tsIt;
    ts.get()->accept(jsVis);
  }
  message.reply(status_codes::OK, "ok");
  return;
  
  
  json::value tsListV = json::value::array();
//  int i=0;
  for (auto tsIt = _tsList.begin(); tsIt != _tsList.end(); ++tsIt, ++i) {
    json::value obj = json::value::object( { {"series",json::value((*tsIt)->name())}, {"units",json::value((*tsIt)->units().unitString())} } );
    tsListV.as_array()[i] = obj;
  }
  
  message.reply(status_codes::OK, tsListV).wait();
  return;
}

void FluxService::_get_runState(web::http::http_request message) {
  
  
  
}

void FluxService::_get_source(web::http::http_request message) {
  InfluxDbPointRecord::_sp inf(new InfluxDbPointRecord);
  PointRecord::_sp pr = inf;
  pr->setName("new influx record");
  
  
  
}




