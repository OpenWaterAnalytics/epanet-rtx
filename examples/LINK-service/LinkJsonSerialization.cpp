#include "LinkJsonSerialization.hpp"
#include <cpprest/json.h>

#include "Curve.h"
#include "CurveFunction.h"

#include "ConstantTimeSeries.h"
#include "SineTimeSeries.h"
#include "TimeSeriesFilter.h"
#include "MovingAverage.h"
#include "FirstDerivative.h"
#include "IntegratorTimeSeries.h"
#include "ValidRangeTimeSeries.h"
#include "OffsetTimeSeries.h"
#include "GainTimeSeries.h"
#include "ThresholdTimeSeries.h"
#include "MultiplierTimeSeries.h"
#include "InversionTimeSeries.h"
#include "FailoverTimeSeries.h"
#include "LagTimeSeries.h"
#include "MathOpsTimeSeries.h"
#include "MetaTimeSeries.h"
#include "StatsTimeSeries.h"
#include "CorrelatorTimeSeries.h"
#include "OutlierExclusionTimeSeries.h"
#include "AggregatorTimeSeries.h"

using JSV = web::json::value;
using namespace RTX;
using namespace std;

static string _c("_class");

void _checkKey(web::json::value v, string key);
void _checkKeys(web::json::value v, vector<string> keys);

void _checkKeys(web::json::value v, vector<string> keys) {
  for(auto key : keys) {
    _checkKey(v, key);  // throws if not found
  }
}
void _checkKey(web::json::value v, string key) {
  auto found = v.as_object().find(key); // throws if not found
  if (found == v.as_object().end()) {
    string err = "Key not found: " + key;
    throw web::json::json_exception(_XPLATSTR(err.c_str()));
  }
}

map<string, PointRecordTime::time_format_t> _odbc_zone_strings();
map<string, PointRecordTime::time_format_t> _odbc_zone_strings() {
  return {{"local",PointRecordTime::LOCAL},{"utc",PointRecordTime::UTC}};
}

template<typename T> RTX_object::_sp _newRtxObj() {
  RTX_object::_sp o( new T );
  return o;
};


#pragma mark Serializer

JSV SerializerJson::to_json(RTX_object::_sp obj) {
  if (obj) {
    RTX_object *o = obj.get();
    return SerializerJson::to_json(*o);
  }
  else {
    return JSV::object();
  }
};
JSV SerializerJson::to_json(RTX_object& obj) {
  SerializerJson js;
  obj.accept(js);
  return js.json();
}

JSV SerializerJson::to_json(std::vector<RTX_object::_sp> objVector) {
  JSV v = JSV::array(objVector.size());
  int i = 0;
  for (auto obj : objVector) {
    v.as_array()[i] = to_json(obj);
    ++i;
  }
  return v;
};

SerializerJson::SerializerJson() : _v(JSV::object()) {
  //**//
};


void SerializerJson::visit(Units &u) {
  _v[_c] = JSV("units");
  _v["unitString"] = JSV(u.to_string());
  _v["rawUnits"] = JSV(u.rawUnitString());
}

void SerializerJson::visit(Clock &c) {
  
}

void SerializerJson::visit(TimeSeries &ts) {
  _v["name"] = JSV(ts.name());
  _v[_c] = JSV("timeseries");
  Units u = ts.units();
  _v["units"] = SerializerJson::to_json(u);
};
void SerializerJson::visit(TimeSeriesFilter &ts) {
  this->visit((TimeSeries&)ts);
  _v[_c] = JSV("filter");
};


/******* POINTRECORD *******/
void SerializerJson::visit(PointRecord &pr) {
  _v[_c] = JSV("record");
  _v["name"] = JSV(pr.name());
}
void SerializerJson::visit(DbPointRecord &pr) {
  this->visit((PointRecord&)pr);
  _v["connectionString"] = JSV(pr.connectionString());
  _v["readonly"] = JSV(pr.readonly());
}
void SerializerJson::visit(SqlitePointRecord &pr) {
  this->visit((DbPointRecord&)pr);
  _v[_c] = JSV("sqlite");
}
void SerializerJson::visit(InfluxDbPointRecord &pr) {
  this->visit((DbPointRecord&)pr);
  _v[_c] = JSV("influx");
}
void SerializerJson::visit(InfluxUdpPointRecord &pr) {
  this->visit((DbPointRecord&)pr);
  _v[_c] = JSV("influx_udp");
}
void SerializerJson::visit(OdbcDirectPointRecord &pr) {
  this->visit((DbPointRecord&)pr);
  _v[_c] = JSV("odbc");
  _v["driver"] = JSV(pr.connection.driver);
  _v["meta"] = JSV(pr.querySyntax.metaSelect);
  _v["range"] = JSV(pr.querySyntax.rangeSelect);
  
  for (auto z : _odbc_zone_strings()) {
    if (z.second == pr.timeFormat()) {
      _v["zone"] = JSV(z.first);
    }
  }
}

JSV SerializerJson::json() {
  return _v;
};



#pragma mark Deserializer

DeserializerJson::DeserializerJson(JSV withJson) : _v(withJson) {
  //**//
};



// static function from_json:
// takes json value and routes the request to a function that will handle
// the actual creation of the correct RTX object type and subclass (ts, clock, record, ...)
// and then cause the object to accept an instantiated deserializer as a visitor.
RTX_object::_sp DeserializerJson::from_json(JSV json) {
  if (json.is_object()) {
    map< string, std::function<RTX_object::_sp()> > c = { // c is for "CREATOR"
      { "sqlite",     &_newRtxObj<SqlitePointRecord>},
      { "influx",     &_newRtxObj<InfluxDbPointRecord>},
      { "influx_udp", &_newRtxObj<InfluxUdpPointRecord>},
      { "odbc",       &_newRtxObj<OdbcDirectPointRecord>},
      { "timeseries", &_newRtxObj<TimeSeries>},
      { "filter",     &_newRtxObj<TimeSeriesFilter>},
      { "units",      &_newRtxObj<Units>},
      { "clock",      &_newRtxObj<Clock>},
      { "units",      &_newRtxObj<Units>}
    };
    // create the concrete object
    if (json.as_object().find(_c) != json.as_object().end()) {
      string type = json.as_object()[_c].as_string();
      if (c.count(type)) {
        RTX_object::_sp obj = c[type]();
        DeserializerJson d(json);
        obj->accept(d);
        return obj;
      }
      else {
        throw web::json::json_exception(_XPLATSTR("Object type not recognized"));
      }
    }
  }
  return RTX_object::_sp();
};

vector<RTX_object::_sp> DeserializerJson::from_json_array(JSV json) {
  vector<RTX_object::_sp> objs;
  for (auto jsts : json.as_array()) {
    RTX_object::_sp o = DeserializerJson::from_json(jsts);
    if (o) {
      objs.push_back(o);
    }
  }
  return objs;
}

/******* TIMESERIES *******/
void DeserializerJson::visit(TimeSeries& ts) {
  _checkKeys(_v, {"units","name"});  // throws if not found
  JSV jsU = _v.as_object()["units"];
  Units::_sp u = static_pointer_cast<Units>(from_json(jsU));
  ts.setUnits(*u.get());
  ts.setName(_v.as_object()["name"].as_string());
};
void DeserializerJson::visit(TimeSeriesFilter& ts) {
  this->visit((TimeSeries&)ts);
  _checkKeys(_v, {"clock"});  // throws if not found
  JSV jsC = _v.as_object()["clock"];
  Clock::_sp c = static_pointer_cast<Clock>(from_json(jsC));
  ts.setClock(c);
};
/******* UNITS *******/
void DeserializerJson::visit(Units &u) {
  _checkKey(_v, "unitString");   // throws if not found
  string uStr = _v.as_object()["unitString"].as_string();
  u = Units::unitOfType(uStr);
};
/******* CLOCK *******/
void DeserializerJson::visit(Clock &c) {
  _checkKeys(_v, {"period","offset"});  // throws if not found
  int p = _v.as_object()["period"].as_integer();
  int o = _v.as_object()["offset"].as_integer();
  c.setPeriod(p);
  c.setStart(o);
};

/******* POINTRECORD *******/
void DeserializerJson::visit(PointRecord& pr) {
  _checkKey(_v, "name");   // throws if not found
  pr.setName(_v.as_object().at("name").as_string());
};
void DeserializerJson::visit(DbPointRecord &pr) {
  this->visit((PointRecord&)pr);
  _checkKeys(_v, {"connectionString"});
  web::json::object o = _v.as_object();
  if (o.find("readonly") != o.end()) {
    pr.setReadonly(o["readonly"].as_bool());
  }
  else {
    pr.setReadonly(true);
  }
  pr.setConnectionString(o.at("connectionString").as_string());
};
void DeserializerJson::visit(SqlitePointRecord &pr) {
  this->visit((DbPointRecord&)pr);
};
void DeserializerJson::visit(InfluxDbPointRecord &pr) {
  this->visit((DbPointRecord&)pr);
};
void DeserializerJson::visit(InfluxUdpPointRecord &pr) {
  this->visit((DbPointRecord&)pr);
};
void DeserializerJson::visit(OdbcDirectPointRecord &pr) {
  this->visit((DbPointRecord&)pr);
  _checkKeys(_v, {"driver","meta","range","zone"});   // throws if not found
  web::json::object o = _v.as_object();
  pr.connection.driver = o.at("driver").as_string();
  pr.querySyntax.metaSelect = o.at("meta").as_string();
  pr.querySyntax.rangeSelect = o.at("range").as_string();
  
  string thisTF = o.at("zone").as_string();
  map<string, PointRecordTime::time_format_t> tf = _odbc_zone_strings();
  if (tf.count(thisTF)) {
    pr.setTimeFormat(tf[thisTF]);
  }
};



TimeSeries::_sp DeserializerJson::analyticWithJson(web::json::value jsonValue, vector<TimeSeries::_sp> candidateSeries) {
  
  cout << jsonValue.serialize() << '\n';
  
  web::json::object d = jsonValue.as_object();
  string type = d["type"].as_string();
  if (type == "tank") {
    
    Curve::_sp curve( new Curve );
    
    // tank analytic. check for geometry
    if (d.find("geometry") == d.end() ||
        !d["geometry"].is_object()) {
      throw web::json::json_exception(U("Geometry Invalid or not found"));
    }
    web::json::object geo = d["geometry"].as_object();
    
    // check that important keys exist
    vector<string> keys = {"tankId","inputUnits","outputUnits","data","inletDiameter"};
    for(auto key : keys) {
      if (geo.find(key) == geo.end()) {
        string errStr = "Key not found: " + key;
        throw web::json::json_exception(errStr.c_str());
      }
    }
    
    string tankId = geo["tankId"].as_string();
    
    TimeSeries::_sp tankLevel;
    for (auto ts : candidateSeries) {
      if (ts->name() == tankId) {
        tankLevel = ts;
        break;
      }
    }
    
    if (!tankLevel) {
      cerr << "could not find tank series" << '\n';
      return TimeSeries::_sp(); // not found;
    }
    
    double inletDiam = geo["inletDiameter"].as_double();
    Units inletUnits = Units::unitOfType(geo["inletUnits"].as_string());
    
    auto inUnits = geo["inputUnits"].as_string();
    auto outUnits = geo["outputUnits"].as_string();
    
    curve->inputUnits = Units::unitOfType(inUnits);
    curve->outputUnits = Units::unitOfType(outUnits);
    for(auto pt : geo["data"].as_array()) {
      if (!pt.is_array()) {
        throw web::json::json_exception(_XPLATSTR("Object type not recognized"));
      }
      web::json::array xy = pt.as_array();
      if (xy.size() != 2) {
        throw web::json::json_exception(_XPLATSTR("Object type not recognized"));
      }
      double x = xy[0].as_double();
      double y = xy[1].as_double();
      curve->curveData[x] = y;
    }
    
    // assemble the analytic
    // cv-ma-ma-dvdt = flow
    Clock::_sp m5( new Clock(5*60) );
    Clock::_sp h1( new Clock(60*60) );
    Clock::_sp d1( new Clock(24*60*60) );
    TimeSeries::_sp turnoverTime;
    
    try {
      
      TimeSeries::_sp smoothLevel =
      tankLevel
      ->append(new TimeSeriesFilter)
        ->c(m5)
      ->append(new StatsTimeSeries)
        ->type(StatsTimeSeries::StatsTimeSeriesMean)
        ->window(h1)
        ->mode(BaseStatsTimeSeries::StatsSamplingModeCentered)
      ->append(new StatsTimeSeries)
        ->type(StatsTimeSeries::StatsTimeSeriesMean)
        ->window(h1)
        ->mode(BaseStatsTimeSeries::StatsSamplingModeCentered);
      
      TimeSeries::_sp tankVolume =
      smoothLevel
      ->append(new CurveFunction)
        ->curve(curve)
        ->units(curve->outputUnits)
        ->c(m5);
      
      TimeSeries::_sp avgDailyInflow =
      tankVolume
      ->append(new FirstDerivative)
        ->append(new ValidRangeTimeSeries)
        ->range(0, 100000)
        ->mode(ValidRangeTimeSeries::saturate)
      ->append(new StatsTimeSeries)
        ->type(StatsTimeSeries::StatsTimeSeriesMean)
        ->window(d1)
        ->mode(BaseStatsTimeSeries::StatsSamplingModeLagging)
        ->c(d1);
      
      turnoverTime =
      tankVolume
      ->append(new StatsTimeSeries)
        ->type(StatsTimeSeries::StatsTimeSeriesMean)
        ->window(d1)
        ->mode(BaseStatsTimeSeries::StatsSamplingModeLagging)
        ->c(d1)
      ->append(new MultiplierTimeSeries)
        ->mode(MultiplierTimeSeries::MultiplierModeDivide)
        ->secondary(avgDailyInflow)
        ->units(RTX_DAY)
        ->c(d1)
        ->name(d["name"].as_string());
    } catch (...) {
      cerr << "unknown error" << '\n';
    }
    
    return turnoverTime;
  }
  
  return TimeSeries::_sp();
}

