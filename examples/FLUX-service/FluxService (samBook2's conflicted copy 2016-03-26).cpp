#include "FluxService.hpp"

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



FluxService::Responders FluxService::_FluxService_GET_responders() {
  Responders r;
  r["series"] = bind(&FluxService::_get_timeseries, this, _1);
  r["run"]    = bind(&FluxService::_get_runState,   this, _1);
  r["source"] = bind(&FluxService::_get_source,     this, _1);
  return r;
}

FluxService::Responders FluxService::_FluxService_POST_responders() {
  Responders r;
  r["series"] = bind(&FluxService::_post_timeseries, this, _1);
  r["run"]    = bind(&FluxService::_post_runState,   this, _1);
  r["source"] = bind(&FluxService::_post_source,     this, _1);
  return r;
}


#include "TimeSeriesFilter.h"
#include "ThresholdTimeSeries.h"
#include "SqlitePointRecord.h"
#include "OdbcDirectPointRecord.h"



class SerializerJson : public BaseVisitor,
public Visitor<Units>,
public Visitor<TimeSeries>,
public Visitor<TimeSeriesFilter>,
public Visitor<ThresholdTimeSeries>,
public Visitor<PointRecord>,
public Visitor<DbPointRecord>,
public Visitor<SqlitePointRecord>
{
public:
  static json::value to_json(RTX_object::_sp obj) {
    RTX_object *o = obj.get();
    return to_json(*o);
  };
  static JSV to_json(RTX_object& obj) {
    SerializerJson js;
    obj.accept(js);
    return js.json();
  }
  
  static json::value to_json(std::vector<RTX_object::_sp> objVector) {
    json::value v = json::value::array(objVector.size());
    int i = 0;
    for (auto obj : objVector) {
      v.as_array()[i] = to_json(obj);
      ++i;
    }
    return v;
  };
  
  SerializerJson() : _v(json::value::object()) {};
  
  
  void visit(Units &u) {
    _v["_class"] = JSV("units");
    _v["unitString"] = JSV(u.unitString());
    _v["rawUnits"] = JSV(u.rawUnitString());
  }
  
  
  void visit(TimeSeries &ts) {
    _v["name"] = JSV(ts.name());
    _v["_class"] = JSV("timeseries");
    Units u = ts.units();
    _v["units"] = SerializerJson::to_json(u);
  };
  void visit(TimeSeriesFilter &ts) {
    this->visit((TimeSeries&)ts);
    _v["_class"] = JSV("filter");
  };
  void visit(ThresholdTimeSeries &ts) {
    TimeSeriesFilter &tsBase = ts;
    this->visit(tsBase);
    _v["_class"] = JSV("threshold");
  }
  
  
  void visit(PointRecord &pr) {
    _v["_class"] = JSV("record");
    _v["name"] = JSV(pr.name());
  }
  void visit(DbPointRecord &pr) {
    this->visit((PointRecord&)pr);
    _v["connectionString"] = JSV(pr.connectionString());
    _v["readonly"] = JSV(pr.readonly());
  }
  void visit(SqlitePointRecord &pr) {
    this->visit((DbPointRecord&)pr);
    _v["_class"] = JSV("sqlite");
  }
  
  json::value json() {return _v;};
  
private:
  json::value _v;
};


class JsonDeserializer : public BaseVisitor,
public Visitor<Units>,
public Visitor<Clock>,
public Visitor<TimeSeries>,
public Visitor<TimeSeriesFilter>,
public Visitor<PointRecord>,
public Visitor<DbPointRecord>,
public Visitor<SqlitePointRecord>,
public Visitor<InfluxDbPointRecord>,
public Visitor<OdbcDirectPointRecord>
{
public:
  JsonDeserializer(JSV withJson) : _v(withJson) { };
  
  template<typename T> static RTX_object::_sp createRtxObjShared() { RTX_object::_sp o( new T ); return o; };
  
  // static function from_json:
  // takes json value and routes the request to a function that will handle
  // the actual creation of the correct RTX object type and subclass (ts, clock, record, ...)
  // and then cause the object to accept an instantiated deserializer as a visitor.
  static RTX_object::_sp from_json(JSV json) {
    map< string, std::function<RTX_object::_sp()> > c; // c is for "CREATOR"
    
    c["sqlite"]     = &JsonDeserializer::createRtxObjShared<SqlitePointRecord>;
    c["influx"]     = &JsonDeserializer::createRtxObjShared<InfluxDbPointRecord>;
    c["odbc"]       = &JsonDeserializer::createRtxObjShared<OdbcDirectPointRecord>;
    
    c["timeseries"] = &JsonDeserializer::createRtxObjShared<TimeSeries>;
    c["filter"]     = &JsonDeserializer::createRtxObjShared<TimeSeriesFilter>;
    c["threshold"]  = &JsonDeserializer::createRtxObjShared<ThresholdTimeSeries>;
    
    c["units"]      = &JsonDeserializer::createRtxObjShared<Units>;
    c["clock"]      = &JsonDeserializer::createRtxObjShared<Clock>;
    
    // create the concrete object
    string type = json.as_object()["_class"].as_string();
    if (c.count(type)) {
      RTX_object::_sp obj = c[type]();
      JsonDeserializer d(json);
      obj->accept(d);
      return obj;
    }
    return RTX_object::_sp();
  };
  
  
  void visit(TimeSeries& ts) {
    
  };
  void visit(TimeSeriesFilter& ts) {
    this->visit((TimeSeries&)ts);
    
  };
  
  void visit(Units &u) {
    string uStr = _v.as_object()["unitString"].as_string();
    u = Units::unitOfType(uStr);
  };
  
  void visit(Clock &c) {
    
  };
  
  
  void visit(PointRecord& pr) {
    
  };
  void visit(DbPointRecord &pr) {
    this->visit((PointRecord&)pr);
    
  };
  void visit(SqlitePointRecord &pr) {
    this->visit((DbPointRecord&)pr);
    
  };
  void visit(InfluxDbPointRecord &pr) {
    this->visit((DbPointRecord&)pr);
    
  };
  void visit(OdbcDirectPointRecord &pr) {
    this->visit((DbPointRecord&)pr);
    
  };
  
  
  
  
  JSV value() { return _v; };
  
private:
  JSV _v;
  
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
  
  Responders res = this->_FluxService_GET_responders();
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

void FluxService::_put(http_request message) {
  
}

void FluxService::_post(http_request message) {
  auto paths = http::uri::split_path(http::uri::decode(message.relative_uri().path()));
  if (paths.size() == 0) {
    message.reply(status_codes::Forbidden);
    return;
  }
  string entry = paths[0];
  Responders res = this->_FluxService_POST_responders();
  if (res.count(entry)) {
    res[entry](message);
    return;
  }
  message.reply(status_codes::OK);
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




void FluxService::_post_timeseries(http_request message) {
  
  JSV js = message.extract_json().get();
  
  RTX_object::_sp ts = JsonDeserializer::from_json(js);
  
  cout << ts;
  
  message.reply(status_codes::OK);
  
}

void FluxService::_post_runState(web::http::http_request message) {
  
}

void FluxService::_post_source(web::http::http_request message) {
  
}







