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
#include "ThresholdTimeSeries.h"
#include "SqlitePointRecord.h"




class SerializerJson : public BaseVisitor,
public Visitor<TimeSeries>,
public Visitor<TimeSeriesFilter>,
public Visitor<ThresholdTimeSeries>,
public Visitor<SqlitePointRecord>
{
public:
  static json::value to_json(RTX_object::_sp obj) {
    SerializerJson js;
    obj.get()->accept(js);
    return js.json();
  };
  
  static json::value to_json(std::vector<RTX_object::_sp> objVector) {
    json::value v = json::value::array(objVector.size());
    int i = 0;
    for (auto ts : objVector) {
      v.as_array()[i] = to_json(ts);
      ++i;
    }
    return v;
  };
  
  SerializerJson() : _jsonValue(json::value::object()) {};
  
  void visit(TimeSeries &ts) {
    _jsonValue["name"] = json::value(ts.name());
    _jsonValue["type"] = json::value("Timeseries");
  };
  void visit(TimeSeriesFilter &ts) {
    TimeSeries &tsBase = ts;
    this->visit(tsBase);
    _jsonValue["type"] = json::value("Filter");
  };
  void visit(ThresholdTimeSeries &ts) {
    TimeSeriesFilter &tsBase = ts;
    this->visit(tsBase);
    _jsonValue["type"] = json::value("Threshold");
  }
  
  void visit(SqlitePointRecord &pr) {
    _jsonValue["name"] = json::value(pr.name());
    _jsonValue["type"] = json::value("sqlite");
    _jsonValue["connectionString"] = json::value(pr.connectionString());
    _jsonValue["readonly"] = json::value::boolean(pr.readonly());
  }
  
  json::value json() {return _jsonValue;};
  
private:
  json::value _jsonValue;
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
  
  TimeSeries::_sp tsF( new ThresholdTimeSeries() );
  tsF->setName("blah ts filter");
  
  TimeSeries::_sp tsF2( new TimeSeriesFilter() );
  tsF2->setName("ts filter 2");
  
  vector<RTX_object::_sp> tsVec = {tsF, tsF2};
  
  // return the list of time series:
  // [ {series:"ts1",units:"MGD"} , {series:"ts2",units:"FT"} ]
  
  json::value v = SerializerJson::to_json(tsVec);
  message.reply(status_codes::OK, v);
  
  return;
}

void FluxService::_get_runState(web::http::http_request message) {
  
  
  
}

void FluxService::_get_source(web::http::http_request message) {
  SqlitePointRecord::_sp pr(new SqlitePointRecord());
  pr->setConnectionString("/Users/sam/Desktop/pr.sqlite");
  pr->setName("my test pr");
  
  json::value v = SerializerJson::to_json(pr);
  
  message.reply(status_codes::OK, v);
}




