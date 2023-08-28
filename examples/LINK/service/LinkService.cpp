#include "LinkService.hpp"

#include "Curve.h"
#include "CurveFunction.h"

#include <cpprest/uri.h>
#include <cpprest/json.h>
#include <cpprest/http_client.h>
#include <map>

using namespace std;
using namespace RTX;
using namespace web;
using namespace http;
using namespace utility;
using namespace http::experimental::listener;
using JSV = json::value;


static string kFromNameKey = "renamed_from_name";
static string kFromUnitsKey = "renamed_from_units";

void _link_callback(LinkService* svc, const std::string& msg);
void _link_callback(LinkService* svc, const std::string& msg) {
  string myLine(msg);
  size_t loc = myLine.find("\n");
  if (loc == string::npos) {
    myLine += "\n";
  }
  const char *logmsg = myLine.c_str();
  fprintf(stdout, "%s", logmsg);
  svc->_statusMessage = myLine;
  svc->post_log("log", "message", myLine);
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
  _runner.setLogging(std::bind(&_link_callback, this, std::placeholders::_1), RTX_AUTORUNNER_LOGLEVEL_VERBOSE);
  
  _runner.setMetricsCallback([&](int n, int t){
    _metrics.time->insert(Point(time(NULL), double(t)));
    _metrics.count->insert(Point(time(NULL), double(n)));
  });
  
  _options.backfill = 0;
  _options.frequency = 0;
  _options.throttle = 0;
  _options.window = 0;
  _options.smart = false;
  
  _metrics.count.reset(new TimeSeries());
  _metrics.time.reset(new TimeSeries());
  
  _metrics.count->name("count,generator=link")->units(RTX_DIMENSIONLESS);
  _metrics.time->name("time,generator=link")->units(RTX_SECOND);
  
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
  
  const map< string, function<http_response(http_request)> > responders = {
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
  
  if (responders.count(entryPoint)) {
    http_response response = responders.at(entryPoint)(message);
    message.reply(response);
    return;
  }
  
  _link_respond(message, JSV::object(), status_codes::NotFound);
}

void LinkService::_put(http_request message) {
  
}

void LinkService::_post(http_request message) {
//  cout << message.to_string() << endl;
  JSV js = message.extract_json().get();
  
  const map< string, std::function<http_response(JSV)> > responders = {
    {"series",      bind(&LinkService::_post_timeseries, this, placeholders::_1)},
    {"run",         bind(&LinkService::_post_runState,   this, placeholders::_1)},
    {"options",     bind(&LinkService::_post_options,    this, placeholders::_1)},
    {"source",      bind(&LinkService::_post_source,     this, placeholders::_1)},
    {"destination", bind(&LinkService::_post_destination,this, placeholders::_1)},
    {"analytics",   bind(&LinkService::_post_analytics,  this, placeholders::_1)},
    {"config",      bind(&LinkService::_post_config,     this, placeholders::_1)},
    {"log",         bind(&LinkService::_post_logmessage, this, placeholders::_1)}
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

void LinkService::post_log(std::string metric, std::string field, std::string value) {
  
  JSV js = JSV::object();
  
  js.as_object()["metric"] = JSV(metric);
  js.as_object()["field"] = JSV(field);
  js.as_object()["value"] = JSV(value);
  this->_post_logmessage(js);
}

void LinkService::_delete(http_request message) {
  
}


#pragma mark - ping

http_response LinkService::_get_ping(web::http::http_request message) {
  return _link_empty_response(status_codes::NoContent);
}

#pragma mark - GET

http_response LinkService::_get_timeseries(http_request message) {
  
  JSV v = JSV::array(_sourceSeries.size());
  int i = 0;
  for (auto src_obj : _sourceSeries) {
    auto obj = _translation[src_obj];
    auto e = SerializerJson::to_json(obj);
    e[kFromNameKey] = JSV(src_obj->name());
    Units::_sp u = make_shared<Units>(src_obj->units());
    e[kFromUnitsKey] = SerializerJson::to_json(u);
    v.as_array()[i] = e;
    ++i;
  }
  
  http_response response = _link_empty_response();
  response.set_body(v);
  return response;
}

http_response LinkService::_get_runState(web::http::http_request message) {
  http_response response = _link_empty_response();
  JSV state = JSV::object();
  
  string stateStr;
  if (_runner.isRunning()) {
    state.as_object()["run"] = JSV(true);
    state.as_object()["progress"] = JSV(_runner.pctCompleteFetch());
    state.as_object()["message"] = JSV(_statusMessage);
  }
  else {
    state.as_object()["run"] = JSV(false);
    state.as_object()["progress"] = JSV(0);
    state.as_object()["message"] = JSV(_statusMessage);
  }

  response.set_body(state);
  return response;
}

http_response LinkService::_get_source(http_request message) {
  http_response response = _link_empty_response();
  
  auto paths = http::uri::split_path(http::uri::decode(message.relative_uri().path()));
  if (paths.size() > 1) {
    string sourcePath = paths[1];
    
    if (sourcePath.compare("series") == 0) {
      // try connection first
      DbPointRecord::_sp rec = dynamic_pointer_cast<DbPointRecord>(_sourceRecord);
      if (rec) {
        rec->dbConnect();
        if (!rec->isConnected()) {
          auto msg = rec->errorMessage;
          return _link_error_response(status_codes::ExpectationFailed, "Could not connect to record: " + msg);
        }
      }
      
      // respond with list of source series.
      vector<RTX_object::_sp> series;
      if (_sourceRecord) {
        auto ids = _sourceRecord->identifiersAndUnits();
        for (auto id : *ids.get()) {
          string name = id.first;
          Units units = id.second.first;
          TimeSeries::_sp ts(new TimeSeries);
          ts->setName(name);
          ts->setUnits(units);
          series.push_back(ts);
        }
      }
      JSV js = SerializerJson::to_json(series);
      response.set_body(js);
      return response;
    } // GET series list
  }
  
  if (_sourceRecord) {
    json::value v = SerializerJson::to_json(_sourceRecord);
    response.set_body(v);
    return response;
  }
  else {
    response.set_body(JSV::object());
    return response;
  }
}

http_response LinkService::_get_destination(http_request message) {
  http_response response = _link_empty_response();
  if (_destinationRecord) {
    json::value v = SerializerJson::to_json(_destinationRecord);
    response.set_body(v);
  }
  else {
    response.set_body(JSV::object());
  }
  return response;
}

http_response LinkService::_get_odbc_drivers(http_request message) {
  http_response response = _link_empty_response();
  list<string> drivers = OdbcPointRecord::driverList();
  JSV d = JSV::array(drivers.size());
  int i = 0;
  for(auto driver : drivers) {
    d.as_array()[i++] = JSV(driver);
  }
  response.set_body(d);
  return response;
}

http_response LinkService::_get_units(http_request message) {
  http_response response = _link_empty_response();
  auto unitsMap = Units::unitStrings;
  vector<RTX_object::_sp> units;
  for (auto unitTypes : unitsMap) {
    Units::_sp usp = make_shared<Units>(unitTypes.second);
    units.push_back(usp);
  }
  JSV u = SerializerJson::to_json(units);
  response.set_body(u);
  return response;
}

http_response LinkService::_get_options(http_request message) {
  JSV o = JSV::object();
  http_response response = _link_empty_response();
  
  o["interval"] = JSV(_options.frequency / 60); // minutes
  o["window"] =   JSV(_options.window / 60); // minutes
  o["backfill"] = JSV(_options.backfill / (60 * 60 * 24)); // days
  o["throttle"] = JSV(_options.throttle); // seconds
  o["smart"] = JSV(_options.smart);
  
  response.set_body(o);
  return response;
}

http_response LinkService::_get_config(http_request message) {
  JSV config = JSV::object();
  http_response response = _link_empty_response();

  config["series"] = this->_get_timeseries(message).extract_json().get();
  config["source"] = this->_get_source(message).extract_json().get();
  config["destination"] = this->_get_destination(message).extract_json().get();
  config["options"] = this->_get_options(message).extract_json().get();
  config["run"] = this->_get_runState(message).extract_json().get();
  
  response.set_body(config);
  return response;
}


#pragma mark - POST

http_response LinkService::_post_config(JSV json) {
  json::object o = json.as_object();
  http_response res = _link_empty_response();
  JSV series = o["series"];
  JSV source = o["source"];
  JSV destination = o["destination"];
  JSV analytics = o["analytics"];
  JSV options = o["options"];
  for( auto fn : {
    bind(&LinkService::_post_source,     this, source),
    bind(&LinkService::_post_timeseries, this, series),
    bind(&LinkService::_post_options,    this, options),
    bind(&LinkService::_post_destination,this, destination),
    bind(&LinkService::_post_analytics,  this, analytics)
  }) {
    http_response thisres = fn();
    if (thisres.status_code() != status_codes::OK) {
      cout << "POST FAILED:" << thisres.reason_phrase() << endl;
      res = thisres;
    }
  }
  
  // optional run parameter passed? if so, get going.
  if (o.find("run") != o.end()) {
    JSV runState = o["run"];
    http_response runRes = this->_post_runState(runState);
    if (runRes.status_code() != status_codes::OK) {
      res = runRes;
    }
  }
  
  if (res.status_code() != status_codes::OK) {
    return res;
  }
  
  return res;
}

http_response LinkService::_post_timeseries(JSV js) {
  _statusMessage = "configuring tag list";
  http_response r;
  cout << "=====================================\n";
  cout << "== SETTING TIMESERIES\n";
  
  _sourceSeries.clear();
  _destinationSeries.clear();
  _translation.clear();
  
  if (!_sourceRecord) {
    r = _link_error_response(status_codes::MethodNotAllowed, "Must specify a source record first");
    return r;
  }
  
  if (js.is_array() && js.size() > 0) {
    _sourceRecord->beginBulkOperation();
    for (auto jsts : js.as_array()) {
      RTX_object::_sp o = DeserializerJson::from_json(jsts);
      if (o) {
        auto filter = dynamic_pointer_cast<TimeSeriesFilter>(o);
        if (!filter) {
          r = _link_error_response(status_codes::ExpectationFailed, "Source series must be a filter");
          cerr << "Source series must be a filter" << EOL;
          return r;
        }
        
        if (jsts.has_field(kFromNameKey) && jsts.has_field(kFromUnitsKey)) {
          // map the source for this filter.
          TimeSeries::_sp ts(new TimeSeries);
          ts->setName(jsts[kFromNameKey].as_string());
          Units::_sp u = static_pointer_cast<Units>(DeserializerJson::from_json(jsts[kFromUnitsKey]));
          ts->setUnits(*u.get());
          ts->setRecord(_sourceRecord);
          _sourceSeries.push_back(ts);
          
          filter->setSource(ts);
          _destinationSeries.push_back(filter);
          _translation[ts] = filter;
        }
      }
    }
    _sourceRecord->endBulkOperation();
    r = _link_empty_response();
    vector<RTX::TimeSeries::_sp> series;
    series.reserve(_destinationSeries.size());
    std::copy(std::begin(_destinationSeries), std::end(_destinationSeries), std::back_inserter(series));
    _runner.setSeries(series);
  }
  else {
    r = _link_error_response(status_codes::MethodNotAllowed, "Series must be specified as an Array");
    cout << "== Series must be specified as an array\n";
  }
  
  cout << "=====================================" << endl;
  return r;
}

http_response LinkService::_post_runState(JSV js) {
  _statusMessage = "starting run";
  // expect obj with keys: run(bool)
  json::object o = js.as_object();
  
  if (o["run"].as_bool()) {
    // run
    this->runDuplication();
  }
  else {
    // stop
    this->stopDuplication();
  }
  return _link_empty_response();
}

http_response LinkService::_post_source(JSV js) {
  _statusMessage = "setting duplication source";
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
      _sourceRecord = static_pointer_cast<DbPointRecord>(o);
      _sourceRecord->dbConnect();
//      cout << "== " << _sourceRecord->name() << '\n';
      r = _link_empty_response();
    }
  }
  catch (const web::json::json_exception &e) {
    cerr << e.what() << endl;
    r = _link_error_response(status_codes::NotAcceptable, "Invalid: " + string(e.what()));
  }
  cout << "== DONE SETTING DUPLICATION SOURCE\n";
  cout << "===================================" << endl;
  _statusMessage = "";
  return r;
}

http_response LinkService::_post_destination(JSV js) {
  _statusMessage = "configuring destination record";
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
    cout << "== empty destination specified.\n==\n";
    _destinationRecord.reset();
    r = _link_empty_response();
    return r;
  }
  
  _destinationRecord = static_pointer_cast<DbPointRecord>(d);
  
  if (_destinationRecord) {
    _destinationRecord->setReadonly(false);
    cout << "== " << _destinationRecord->name() << '\n';
    _destinationRecord->dbConnect();
    if (_destinationRecord->isConnected()) {
      cout << "== connection successful\n";
      r = _link_empty_response();
      this->refreshDestinationSeriesRecords();
    }
    else {
      string err = "Destination Record: " + _destinationRecord->errorMessage;
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
  _statusMessage = "";
  return r;
}

void LinkService::refreshDestinationSeriesRecords() {
  _destinationRecord->beginBulkOperation();
  for (auto ts : _destinationSeries) {
    ts->setRecord(_destinationRecord);
  }
  _metrics.count->setRecord(_destinationRecord);
  _metrics.time->setRecord(_destinationRecord);
  _destinationRecord->endBulkOperation();
}

http_response LinkService::_post_analytics(web::json::value json) {
  
  // supported analytics: tank turnover!
  
  /*
   expected data format:
   "analytics": [
  	{
      "type": "tank",
      "geometry": {
        "tankId": "tank1_name",
        "inputUnits": "ft",
        "outputUnits": "ft3",
        "inletDiameter": 12,
        "inletUnits": "in",
        "data": [
          [1,1],
          [2,2],
          [3,3]
        ]
      }
  	},
  	...
  	}
   ]
   */
//  
//  
//  _analytics.clear();
//  
//  if (!json.is_array()) {
//    return _link_error_response(status_codes::BadRequest, "JSON not recognized");
//  }
//  
//  json::array analytics = json.as_array();
//  
//  for (auto a : analytics) {
//    if (!a.is_object() ||
//        a.as_object().find("type") == a.as_object().end() ||
//        !a.as_object()["type"].is_string() ) {
//      return _link_error_response(status_codes::BadRequest, "JSON not recognized"); // not an object or type member not found, or member not a string
//    }
//    try {
//      TimeSeries::_sp ts = DeserializerJson::analyticWithJson(a, _duplicator.series());
//      if (ts) {
//        ts->setRecord(_destinationRecord);
//        _analytics.push_back(ts);
//      }
//    } catch (const web::json::json_exception &e) {
//      cerr << e.what() << endl;
//      return _link_error_response(status_codes::NotAcceptable, "Invalid: " + string(e.what()));
//    }
//  }
////  this->setDuplicatorSeries();
  
  return _link_empty_response();
}


http_response LinkService::_post_options(JSV js) {
  _statusMessage = "posting options";
  http_response r;
  cout << "=====================================\n";
  cout << "== SETTING OPTIONS\n";
  
  
  // expecting :
  // {'interval': ###, 'window': ###, 'backfill': ###, 'smart': bool}
  
  if (!js.is_object()) {
    r = _link_error_response(status_codes::ExpectationFailed, "data is not a json object");
    cout << "== err: JSON not recognized.\n";
  }
  else {
    json::object o = js.as_object();

    try {
      _options.frequency = o["interval"].as_integer() * 60;
      _options.window = o["window"].as_integer() * 60;
      _options.backfill = o["backfill"].as_integer() * 60 * 60 * 24;
      _options.throttle = o["throttle"].as_integer(); // seconds
      _options.smart = o["smart"].as_bool();
      r = _link_empty_response();
    } catch (std::exception &e) {
      r = _link_error_response(status_codes::MethodNotAllowed, e.what());
    }
  }
  cout << "=====================================" << endl;
  _statusMessage = "";
  return r;
}



http_response LinkService::_post_logmessage(web::json::value js) {
  http_response r;
  cout << "LINK SERVICE sending log entry \n";

  if (!js.is_object()) {
    r = _link_error_response(status_codes::ExpectationFailed, "data is not a json object");
    cerr << "err: JSON log object not recognized.\n";
  }
  else if (_destinationRecord) {
    json::object o = js.as_object();
    string metric = js["metric"].as_string();
    string field = js["field"].as_string();
    string logValue = js["value"].as_string();

    stringstream ss;
    ss << field << "=\"" << logValue << "\" ";
    string valueStr = ss.str();
    
    auto influxdb = dynamic_pointer_cast<InfluxDbPointRecord>(_destinationRecord);
    if (influxdb) {
      influxdb->sendInfluxString(time(NULL), metric, valueStr);
    }
    else {
      auto influxUdp = dynamic_pointer_cast<InfluxUdpPointRecord>(_destinationRecord);
      if (influxUdp) {
        influxUdp->sendInfluxString(time(NULL), metric, valueStr);
      }
    }
    
    r = _link_empty_response();
    cout << "Sent Log Entry: " << metric << " --> " << valueStr << endl << flush;
  }
  return r;
}

void LinkService::runDuplication() {
  if (_runner.isRunning() || !_sourceRecord || !_destinationRecord) {
    return;
  }
  time_t since = time(NULL) - _options.backfill;
  
  _runner.setParams(_options.smart, _options.window, _options.frequency, _options.throttle);
  _runner.run(since);
}

void LinkService::stopDuplication() {
  if (_runner.isRunning()) {
    _runner.cancel();
    
    boost::thread trackStopTask([&]() {
      _statusMessage = "Waiting for Stop";
      _runner.wait();
      _statusMessage = "Idle";
    });
  }
  
}


void LinkService::setDuplicatorSeries() {
//  vector<TimeSeries::_sp> dupeSeries = _tsList;
//  for (auto a : _analytics) {
//    dupeSeries.push_back(a);
//  }
//  _duplicator.setSeries(dupeSeries);
}


