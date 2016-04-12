#include "LinkService.hpp"

using namespace std;
using namespace RTX;
using namespace web;
using namespace http;
using namespace utility;
using namespace http::experimental::listener;


using JSV = json::value;

#include <map>


http_response _link_empty_response(status_code code);
http_response _link_empty_response(status_code code) {
  http_response r(code);
  r.headers().add(U("Access-Control-Allow-Origin"), U("*"));
  return r;
}

http_response _link_error_response(status_code code, string errMsg);
http_response _link_error_response(status_code code, string errMsg) {
  http_response r = _link_empty_response(code);
  JSV err = JSV::object();
  err.as_object()["error"] = JSV(errMsg);
  r.set_body(err);
  return r;
}

pplx::task<void> _link_respond(http_request message, json::value js, status_code code = status_codes::OK);
pplx::task<void> _link_respond(http_request message, json::value js, status_code code) {
  http_response response = _link_empty_response(code);
  response.set_body(js);
  return message.reply(response);         // reply is done here
}


LinkService::LinkService(uri uri) : _listener(uri) {
  _listener.support(methods::GET,  std::bind(&LinkService::_get,    this, std::placeholders::_1));
  _listener.support(methods::PUT,  std::bind(&LinkService::_put,    this, std::placeholders::_1));
  _listener.support(methods::POST, std::bind(&LinkService::_post,   this, std::placeholders::_1));
  _listener.support(methods::DEL,  std::bind(&LinkService::_delete, this, std::placeholders::_1));
  
  
  TimeSeries::_sp ts1(new TimeSeries);
  ts1->setName("FR.FR_LP_FLOW_1");
  ts1->setUnits(RTX_MILLION_GALLON_PER_DAY);
  
  TimeSeries::_sp ts2(new TimeSeries);
  ts2->setName("LP.LP_LL_LEV_FT");
  ts2->setUnits(RTX_FOOT);
  
  _tsList = {ts1,ts2};
  
  _duplicator.setLoggingFunction([&](const char* msg){ this->_statusMessage = string(msg); });
  
}



pplx::task<void> LinkService::open() {
  return _listener.open();
}

pplx::task<void> LinkService::close() {
  return _listener.close();
}


void LinkService::_get(http_request message) {
  
  cout << "GET: " << message.relative_uri().to_string() << endl;
  
  auto paths = http::uri::split_path(http::uri::decode(message.relative_uri().path()));
  
  if (paths.size() == 0) {
    _link_respond(message, JSV::object(), status_codes::NoContent);
    return;
  }
  
  string entryPoint = paths[0];
  
  const map< string, function<void(http_request)> > res = {
    {"series", std::bind(&LinkService::_get_timeseries,   this, std::placeholders::_1)},
    {"run",    std::bind(&LinkService::_get_runState,     this, std::placeholders::_1)},
    {"source", std::bind(&LinkService::_get_source,       this, std::placeholders::_1)},
    {"odbc",   std::bind(&LinkService::_get_odbc_drivers, this, std::placeholders::_1)},
    {"units",  std::bind(&LinkService::_get_units,        this, std::placeholders::_1)}
  };
  
  if (res.count(entryPoint)) {
    res.at(entryPoint)(message);
    return;
  }
  
  _link_respond(message, JSV::object(), status_codes::NotFound);
}

void LinkService::_put(http_request message) {
  
}

void LinkService::_post(http_request message) {
  
  // posted content will be of content-type: undefined because of CORS,
  // so we have to get the string content, and parse into json
  string content = message.extract_string().get();
  JSV js = JSV::parse(content);
  cout << "POST: content: " << js.serialize() << endl;
  
  const map< string, std::function<http_response(JSV)> > responders = {
    {"series",      bind(&LinkService::_post_timeseries, this, placeholders::_1)},
    {"run",         bind(&LinkService::_post_runState,   this, placeholders::_1)},
    {"source",      bind(&LinkService::_post_source,     this, placeholders::_1)},
    {"destination", bind(&LinkService::_post_destination,this, placeholders::_1)},
    {"config",      bind(&LinkService::_post_config,     this, placeholders::_1)}
  };
  
  auto paths = uri::split_path(uri::decode(message.relative_uri().path()));
  if(paths.size() == 0) {
    _link_respond(message, JSV::object(), status_codes::BadRequest);
    return;
  }
  string entry = paths[0];
  if (responders.count(entry)) {
    http_response response = responders.at(entry)(js);
    message.reply(response).wait(); // block since this may change things
    return;
  }
  else {
    message.reply(_link_error_response(status_codes::BadRequest,
                                       "Invalid Endpoint(" + entry + ")"));
  }
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
  _link_respond(message, v);
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
  _link_respond(message, state);
  
}

void LinkService::_get_source(http_request message) {
  
  auto paths = http::uri::split_path(http::uri::decode(message.relative_uri().path()));
  if (paths.size() > 1) {
    string sourcePath = paths[1];
    
    if (sourcePath.compare("series") == 0) {
      // respond with list of source series.
      vector<RTX_object::_sp> series;
      if (_sourceRecord) {
        auto ids = _sourceRecord->identifiersAndUnits();
        for (auto id : ids) {
          string name = id.first;
          Units units = id.second;
          TimeSeries::_sp ts(new TimeSeries);
          ts->setName(name);
          ts->setUnits(units);
          series.push_back(ts);
        }
      }
      JSV js = SerializerJson::to_json(series);
      _link_respond(message, js);
      return;
    } // GET series list
    
  }
  
  if (_sourceRecord) {
    json::value v = SerializerJson::to_json(_sourceRecord);
    _link_respond(message, v);
  }
  else {
    _link_respond(message, JSV::object());
  }
  
}

void LinkService::_get_odbc_drivers(http_request message) {
  list<string> drivers = OdbcPointRecord::driverList();
  JSV d = JSV::array(drivers.size());
  int i = 0;
  for(auto driver : drivers) {
    d.as_array()[i++] = JSV(driver);
  }
  _link_respond(message, d);
}

void LinkService::_get_units(http_request message) {
  auto unitsMap = Units::unitStringMap;
  vector<RTX_object::_sp> units;
  for (auto unitTypes : unitsMap) {
    Units::_sp usp = make_shared<Units>(unitTypes.second);
    units.push_back(usp);
  }
  JSV u = SerializerJson::to_json(units);
  _link_respond(message, u);
}


http_response LinkService::_post_config(JSV json) {
  json::object o = json.as_object();
  
  JSV series = o["series"];
  JSV source = o["source"];
  JSV destination = o["destination"];
  JSV fetch = o["fetch"];
  for( auto fn : {
    bind(&LinkService::_post_source,     this, source),
    bind(&LinkService::_post_timeseries, this, series)
  }) {
    http_response res = fn();
    if (res.status_code() != status_codes::OK) {
      return res;
    }
  }
  
  return _link_empty_response(status_codes::OK);
}

http_response LinkService::_post_timeseries(JSV js) {
  if (js.is_array()) {
    vector<RTX_object::_sp> oList = DeserializerJson::from_json_array(js);
    list<TimeSeries::_sp> tsList;
    for (auto o : oList) {
      TimeSeries::_sp ts = static_pointer_cast<TimeSeries>(o);
      tsList.push_back(ts);
    }
    _duplicator.setSeries(tsList);
    return _link_empty_response(status_codes::OK);
  }
  else {
    return _link_error_response(status_codes::MethodNotAllowed, "Series must be specified as an Array");
  }
  
}

http_response LinkService::_post_runState(JSV js) {
  
  // expect obj with keys: run(bool), window(int), frequency(int)
  json::object o = js.as_object();
  
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
  return _link_empty_response(status_codes::OK);
}

http_response LinkService::_post_source(JSV js) {
  RTX_object::_sp o = DeserializerJson::from_json(js);
  
  if (!o) {
    return _link_error_response(status_codes::BadRequest, "JSON not recognized");
  }
  
  _sourceRecord = static_pointer_cast<PointRecord>(o);
  return _link_empty_response(status_codes::OK);
}

http_response LinkService::_post_destination(JSV js) {
  RTX_object::_sp d = DeserializerJson::from_json(js);
  if (!d) {
    return _link_error_response(status_codes::BadRequest, "JSON not recognized");
  }
  _destinationRecord = static_pointer_cast<PointRecord>(d);
  
  DbPointRecord::_sp dbRecord = dynamic_pointer_cast<DbPointRecord>(_destinationRecord);
  if (dbRecord) {
    dbRecord->dbConnect();
    if (dbRecord->isConnected()) {
      
    }
    else {
      string err = dbRecord->errorMessage;
      return _link_error_response(status_codes::NotAcceptable, err);
    }
  }
  
  return _link_empty_response(status_codes::OK);
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

