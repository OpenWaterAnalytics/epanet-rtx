#include "LinkService.hpp"

using namespace std;
using namespace RTX;
using namespace web;
using namespace http;
using namespace utility;
using namespace http::experimental::listener;
using std::placeholders::_1;
using std::placeholders::_2;

using JSV = json::value;

#include <map>

LinkService::Responders LinkService::_LinkService_GET_responders() {
  Responders r;
  r["series"] = bind(&LinkService::_get_timeseries, this, _1);
  r["run"]    = bind(&LinkService::_get_runState,   this, _1);
  r["source"] = bind(&LinkService::_get_source,     this, _1);
  r["odbc"] = bind(&LinkService::_get_odbc_drivers,       this, _1);
  return r;
}

LinkService::Responders LinkService::_LinkService_POST_responders() {
  Responders r;
  r["series"] = bind(&LinkService::_post_timeseries, this, _1);
  r["run"]    = bind(&LinkService::_post_runState,   this, _1);
  r["source"] = bind(&LinkService::_post_source,     this, _1);
  return r;
}

LinkService::LinkService(uri uri) : _listener(uri) {
  _listener.support(methods::GET, std::bind(&LinkService::_get, this, std::placeholders::_1));
  _listener.support(methods::PUT, std::bind(&LinkService::_put, this, std::placeholders::_1));
  _listener.support(methods::POST, std::bind(&LinkService::_post, this, std::placeholders::_1));
  _listener.support(methods::DEL, std::bind(&LinkService::_delete, this, std::placeholders::_1));
  
  
  TimeSeries::_sp ts1(new TimeSeries);
  ts1->setName("FR.FR_LP_FLOW_1");
  ts1->setUnits(RTX_MILLION_GALLON_PER_DAY);
  
  TimeSeries::_sp ts2(new TimeSeries);
  ts2->setName("LP.LP_LL_LEV_FT");
  ts2->setUnits(RTX_FOOT);
  
  _tsList = {ts1,ts2};
  
}



pplx::task<void> LinkService::open() {
  return _listener.open();
}

pplx::task<void> LinkService::close() {
  return _listener.close();
}


void LinkService::_get(http_request message) {
  
  auto paths = http::uri::split_path(http::uri::decode(message.relative_uri().path()));
  
  if (paths.size() == 0) {
    message.reply(status_codes::NoContent);
    return;
  }
  
  string entryPoint = paths[0];
  
  Responders res = this->_LinkService_GET_responders();
  if (res.count(entryPoint)) {
    res[entryPoint](message);
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

void LinkService::_put(http_request message) {
  
}

void LinkService::_post(http_request message) {
  auto paths = http::uri::split_path(http::uri::decode(message.relative_uri().path()));
  if (paths.size() == 0) {
    message.reply(status_codes::Forbidden);
    return;
  }
  string entry = paths[0];
  Responders res = this->_LinkService_POST_responders();
  if (res.count(entry)) {
    res[entry](message);
    return;
  }
  message.reply(status_codes::OK);
}

void LinkService::_delete(http_request message) {
  
}



#pragma mark - TS

void LinkService::_get_timeseries(http_request message) {
  
  TimeSeries::_sp tsF2( new TimeSeriesFilter() );
  tsF2->setName("ts filter 2");
  
  vector<RTX_object::_sp> tsVec = {tsF2};
  
  // return the list of time series:
  // [ {series:"ts1",units:"MGD"} , {series:"ts2",units:"FT"} ]
  
  json::value v = SerializerJson::to_json(tsVec);
  message.reply(status_codes::OK, v);
  
  return;
}

void LinkService::_get_runState(web::http::http_request message) {
  
  
  
}

void LinkService::_get_source(http_request message) {
  SqlitePointRecord::_sp pr(new SqlitePointRecord());
  pr->setConnectionString("/Users/sam/Desktop/pr.sqlite");
  pr->setName("my test pr");
  
  json::value v = SerializerJson::to_json(pr);
  
  message.reply(status_codes::OK, v);
}

void LinkService::_get_odbc_drivers(http_request message) {
  list<string> drivers = OdbcPointRecord::driverList();
  JSV d = JSV::array(drivers.size());
  int i = 0;
  for(auto driver : drivers) {
    d.as_array()[i++] = JSV(driver);
  }
  message.reply(status_codes::OK, d);
}



void LinkService::_post_timeseries(http_request message) {
  JSV js = message.extract_json().get();
  RTX_object::_sp ts = DeserializerJson::from_json(js);
  message.reply(status_codes::OK);
}

void LinkService::_post_runState(web::http::http_request message) {
  
}

void LinkService::_post_source(web::http::http_request message) {
  
}







