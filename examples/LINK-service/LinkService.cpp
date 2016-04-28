#include "LinkService.hpp"

using namespace std;
using namespace RTX;
using namespace web;
using namespace http;
using namespace utility;
using namespace http::experimental::listener;


using JSV = json::value;

#include <map>

void _link_callback(LinkService* svc, const char* msg);
void _link_callback(LinkService* svc, const char* msg) {
  string myLine(msg);
  size_t loc = myLine.find("\n");
  if (loc == string::npos) {
    myLine += "\n";
  }
  const char *logmsg = myLine.c_str();
  fprintf(stdout, "%s", logmsg);
  
  svc->_statusMessage = myLine;
};


http_response _link_empty_response(status_code code = status_codes::OK);
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
  return message.reply(response);
}


LinkService::LinkService(uri uri) : _listener(uri) {
  _listener.support(methods::GET,  std::bind(&LinkService::_get,    this, std::placeholders::_1));
  _listener.support(methods::PUT,  std::bind(&LinkService::_put,    this, std::placeholders::_1));
  _listener.support(methods::POST, std::bind(&LinkService::_post,   this, std::placeholders::_1));
  _listener.support(methods::DEL,  std::bind(&LinkService::_delete, this, std::placeholders::_1));
  
  TimeSeriesDuplicator::RTX_Duplicator_log_callback cb = std::bind(&_link_callback, this, std::placeholders::_1);
  
  _duplicator.setLoggingFunction(cb);
}


pplx::task<void> LinkService::open() {
  return _listener.open();
}

pplx::task<void> LinkService::close() {
  return _listener.close();
}


void LinkService::_get(http_request message) {
  
  //cout << "GET: " << message.relative_uri().to_string() << endl;
  
  auto paths = http::uri::split_path(http::uri::decode(message.relative_uri().path()));
  
  if (paths.size() == 0) {
    _link_respond(message, JSV::object(), status_codes::NoContent);
    return;
  }
  
  string entryPoint = paths[0];
  
  const map< string, function<void(http_request)> > res = {
    {"ping",   std::bind(&LinkService::_get_ping,         this, std::placeholders::_1)},
    {"series", std::bind(&LinkService::_get_timeseries,   this, std::placeholders::_1)},
    {"run",    std::bind(&LinkService::_get_runState,     this, std::placeholders::_1)},
    {"source", std::bind(&LinkService::_get_source,       this, std::placeholders::_1)},
    {"destination", std::bind(&LinkService::_get_destination, this, std::placeholders::_1)},
    {"odbc",   std::bind(&LinkService::_get_odbc_drivers, this, std::placeholders::_1)},
    {"units",  std::bind(&LinkService::_get_units,        this, std::placeholders::_1)},
    {"options",std::bind(&LinkService::_get_options,      this, std::placeholders::_1)},
    {"config", std::bind(&LinkService::_get_config,       this, std::placeholders::_1)}
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
  cout << message.to_string() << endl;
  JSV js = message.extract_json().get();
  
  const map< string, std::function<http_response(JSV)> > responders = {
    {"series",      bind(&LinkService::_post_timeseries, this, placeholders::_1)},
    {"run",         bind(&LinkService::_post_runState,   this, placeholders::_1)},
    {"options",     bind(&LinkService::_post_options,    this, placeholders::_1)},
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


#pragma mark - ping

void LinkService::_get_ping(web::http::http_request message) {
  message.reply(_link_empty_response(status_codes::NoContent));
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
    state.as_object()["message"] = JSV(_statusMessage);
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

void LinkService::_get_destination(http_request message) {
  if (_destinationRecord) {
    json::value v = SerializerJson::to_json(_destinationRecord);
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

void LinkService::_get_options(http_request message) {
  JSV o = JSV::object();
  
  o.as_object()["interval"] = JSV((int)(_frequency / 60)); // minutes
  o.as_object()["window"] =   JSV((int)(_window / (60 * 60))); // hours
  o.as_object()["backfill"] = JSV((int)(_backfill / (60 * 60 * 24))); // days
  
  _link_respond(message, o);
  
}

void LinkService::_get_config(http_request message) {
  JSV config = JSV::object();
  
  auto tsList = _duplicator.series();
  vector<RTX_object::_sp> tsVec;
  for (auto ts : tsList) {
    tsVec.push_back(ts);
  }
  
  config.as_object()["series"] = SerializerJson::to_json(tsVec);
  config.as_object()["source"] = SerializerJson::to_json(_sourceRecord);
  config.as_object()["destination"] = SerializerJson::to_json(_destinationRecord);
  
  JSV opts = JSV::object();
  opts.as_object()["interval"] = JSV((int)(_frequency / 60)); // minutes
  opts.as_object()["window"] =   JSV((int)(_window / (60 * 60))); // hours
  opts.as_object()["backfill"] = JSV((int)(_backfill / (60 * 60 * 24))); // days
  config.as_object()["fetch"] = opts;
  
  _link_respond(message, config);
}


http_response LinkService::_post_config(JSV json) {
  json::object o = json.as_object();
  
  JSV series = o["series"];
  JSV source = o["source"];
  JSV destination = o["destination"];
  JSV fetch = o["fetch"];
  for( auto fn : {
    bind(&LinkService::_post_source,     this, source),
    bind(&LinkService::_post_destination,this, destination),
    bind(&LinkService::_post_timeseries, this, series),
    bind(&LinkService::_post_options,    this, fetch)
  }) {
    http_response res = fn();
    if (res.status_code() != status_codes::OK) {
      cout << "POST FAILED:" << endl;
      return res;
    }
  }
  
  return _link_empty_response();
}

http_response LinkService::_post_timeseries(JSV js) {
  http_response r;
  cout << "=====================================\n";
  cout << "== SETTING TIMESERIES\n";
  
  if (js.is_array()) {
    vector<RTX_object::_sp> oList;
    try {
      oList = DeserializerJson::from_json_array(js);
    }
    catch (const web::json::json_exception &e) {
      cerr << e.what() << endl;
      return _link_error_response(status_codes::NotAcceptable, "Invalid: " + string(e.what()));
    }
    list<TimeSeries::_sp> tsList;
    for (auto o : oList) {
      TimeSeries::_sp ts = static_pointer_cast<TimeSeries>(o);
      ts->setRecord(_sourceRecord); // no record set by client. set it here.
      tsList.push_back(ts);
      cout << "== " << ts->name() << '\n';
    }
    _duplicator.setSeries(tsList);
    r = _link_empty_response();
  }
  else {
    r = _link_error_response(status_codes::MethodNotAllowed, "Series must be specified as an Array");
    cout << "== Series must be specified as an array\n";
  }
  
  cout << "=====================================" << endl;
  return r;
}

http_response LinkService::_post_runState(JSV js) {
  
  // expect obj with keys: run(bool), window(int), frequency(int)
  json::object o = js.as_object();
  
  if (o["run"].as_bool()) {
    // run
    this->runDuplication(_window,_frequency,_backfill);
  }
  else {
    // stop
    this->stopDuplication();
  }
  return _link_empty_response();
}

http_response LinkService::_post_source(JSV js) {
  
  http_response r;
  cout << "================================\n";
  cout << "== SETTING DUPLICATION SOURCE\n";
  
  try {
    RTX_object::_sp o = DeserializerJson::from_json(js);
    if (!o) {
      cout << "== JSON not recognized\n";
      r = _link_error_response(status_codes::BadRequest, "JSON not recognized");
    }
    else {
      _sourceRecord = static_pointer_cast<PointRecord>(o);
      cout << "== " << _sourceRecord->name() << '\n';
      r = _link_empty_response();
    }
  }
  catch (const web::json::json_exception &e) {
    cerr << e.what() << endl;
    r = _link_error_response(status_codes::NotAcceptable, "Invalid: " + string(e.what()));
  }
  
  cout << "================================" << endl;
  return r;
}

http_response LinkService::_post_destination(JSV js) {
  
  http_response r;
  cout << "=====================================\n";
  cout << "== SETTING DUPLICATION DESTINATION\n";
  
  RTX_object::_sp d;
  
  try {
    d = DeserializerJson::from_json(js);
  }
  catch (const web::json::json_exception &e) {
    cerr << e.what() << endl;
    r = _link_error_response(status_codes::NotAcceptable, "Invalid: " + string(e.what()));
    return r;
  }
  
  if (!d) {
    cout << "== ERROR: JSON not recognized\n==\n";
    r = _link_error_response(status_codes::BadRequest, "JSON not recognized");
    return r;
  }
  
  _destinationRecord = static_pointer_cast<PointRecord>(d);
  
  DbPointRecord::_sp dbRecord = dynamic_pointer_cast<DbPointRecord>(_destinationRecord);
  if (dbRecord) {
    dbRecord->setReadonly(false);
    cout << "== " << dbRecord->name() << '\n';
    dbRecord->dbConnect();
    if (dbRecord->isConnected()) {
      cout << "== connection successful\n";
      r = _link_empty_response();
      _duplicator.setDestinationRecord(dbRecord);
    }
    else {
      string err = "Destination Record: " + dbRecord->errorMessage;
      cout << "== err: " << err << '\n';
      r = _link_error_response(status_codes::NotAcceptable, err);
    }
  }
  else {
    // failed cast to dbpointrecord
    cout << "== not a database\n";
    r = _link_empty_response();
  }
  
  cout << "=====================================" << endl;
  return r;
}


http_response LinkService::_post_options(JSV js) {
  
  http_response r;
  cout << "=====================================\n";
  cout << "== SETTING OPTIONS\n";
  
  
  // expecting :
  // {'interval': ###, 'window': ###, 'backfill': ###}
  
  if (!js.is_object()) {
    r = _link_error_response(status_codes::ExpectationFailed, "data is not a json object");
    cout << "== err: JSON not recognized.\n";
  }
  else {
    json::object o = js.as_object();
    
    // quick anonymous function to handle setting key values by reference
    function<bool (string,int*)> setParam = [&](string key, int* v) {
      if (o.find(key) != o.end()) {
        *v = o[key].as_integer();
        return true;
      } else {
        return false;
      }
    };
    
    int freqMinutes, windowHours, backfillDays;
    
    // map the required keys to the time_t ivars
    map<string,int*> defs = {
      {"interval",&freqMinutes},
      {"window",&windowHours},
      {"backfill",&backfillDays}
    };
    
    // execute the setter function for each pair of key/value-ref definitions
    // and test for key-not-found error
    for (auto def : defs) {
      if (!setParam(def.first, def.second)) {
        r = _link_error_response(status_codes::MethodNotAllowed, "key not found: " + def.first);
        cout << "== key not found: " << def.first << '\n';
        break;
      }
      else {
        cout << "== " << def.first << " :: " << *def.second << '\n';
      }
    }
    
    _frequency = freqMinutes * 60;
    _window = windowHours * 60 * 60;
    _backfill = backfillDays * 60 * 60 * 24;
    
    
    r = _link_empty_response();
  }
  
  cout << "=====================================" << endl;
  return r;
}



void LinkService::runDuplication(time_t win, time_t freq, time_t backfill) {
  if (_duplicator.isRunning()) {
    return;
  }
  
  if (backfill > 0) {
    _duplicator.catchUpAndRun(win, freq, backfill);
  }
  else {
    _duplicator.run(win, freq);
  }
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

