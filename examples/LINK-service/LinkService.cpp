#include "LinkService.hpp"

using namespace std;
using namespace RTX;
using namespace web;
using namespace http;
using namespace utility;
using namespace http::experimental::listener;


using JSV = json::value;

#include <map>

LinkService::Responders LinkService::_LinkService_GET_responders() {
  Responders r;
  r["series"] = std::bind(&LinkService::_get_timeseries, this, std::placeholders::_1);
  r["run"]    = std::bind(&LinkService::_get_runState,   this, std::placeholders::_1);
  r["source"] = std::bind(&LinkService::_get_source,     this, std::placeholders::_1);
  r["odbc"]   = std::bind(&LinkService::_get_odbc_drivers, this, std::placeholders::_1);
  return r;
}

LinkService::Responders LinkService::_LinkService_POST_responders() {
  Responders r;
  r["series"] = std::bind(&LinkService::_post_timeseries, this, std::placeholders::_1);
  r["run"]    = std::bind(&LinkService::_post_runState,   this, std::placeholders::_1);
  r["source"] = std::bind(&LinkService::_post_source,     this, std::placeholders::_1);
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
  
  _duplicator.setLoggingFunction([&](const char* msg){ _statusMessage = string(msg); });
  
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
  
  auto tsList = _duplicator.series();
  vector<RTX_object::_sp> tsVec;
  for (auto ts : tsList) {
    tsVec.push_back(ts);
  }
  
  json::value v = SerializerJson::to_json(tsVec);
  message.reply(status_codes::OK, v);
  
  return;
}

void LinkService::_get_runState(web::http::http_request message) {
  
  JSV state = JSV::object();
  
  string stateStr;
  if (_duplicator.isRunning()) {
    state.as_object()["run"] = JSV(true);
    state.as_object()["progress"] = JSV(_duplicator.pctCompleteFetch());
  }
  else {
    state.as_object()["run"] = JSV(false);
  }
  
  message.reply(status_codes::OK, state);
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
  if (js.is_array()) {
    vector<RTX_object::_sp> oList = DeserializerJson::from_json_array(js);
    list<TimeSeries::_sp> tsList;
    for (auto o : oList) {
      TimeSeries::_sp ts = static_pointer_cast<TimeSeries>(o);
      tsList.push_back(ts);
    }
    _duplicator.setSeries(tsList);
    message.reply(status_codes::OK);
    return;
  }
  else {
    message.reply(status_codes::MethodNotAllowed);
  }
  
}

void LinkService::_post_runState(web::http::http_request message) {
  
  // expect obj with keys: run(bool), window(int), frequency(int)
  
  JSV v = message.extract_json().get();
  json::object o = v.as_object();
  
  if (o["run"].as_bool()) {
    // run
    time_t win = o.find("window") == o.end() ? 3600 : o["window"].as_integer();
    time_t freq = o.find("frequency") == o.end() ? 300 : o["frequency"].as_integer();
    this->runDuplication(win,freq);
  }
  else {
    // stop
    this->stopDuplication();
  }
  
  message.reply(status_codes::OK);
  
}

void LinkService::_post_source(web::http::http_request message) {
  JSV v = message.extract_json().get();
  RTX_object::_sp o = DeserializerJson::from_json(v);
  _sourceRecord = static_pointer_cast<PointRecord>(o);
  
  if (o && _sourceRecord == o) {
    message.reply(status_codes::OK);
  }
  else {
    message.reply(status_codes::BadRequest);
  }
}




void LinkService::runDuplication(time_t win, time_t freq) {
  if (_duplicator.isRunning()) {
    return;
  }
  _duplicator.run(win, freq);
}

void LinkService::stopDuplication() {
  if (_duplicator.isRunning()) {
    _duplicator.stop();
    
    boost::thread waitOnStop([&]() {
      _duplicator.wait();
      _statusMessage = "Idle";
    });
  }
  
}

