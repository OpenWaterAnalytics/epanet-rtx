#include "LinkJsonSerialization.hpp"
#include <cpprest/json.h>
using JSV = web::json::value;
using namespace RTX;
using namespace std;

static string _c("_class");

map<string, PointRecordTime::time_format_t> _odbc_zone_strings();
map<string, PointRecordTime::time_format_t> _odbc_zone_strings() {
  return {{"local",PointRecordTime::LOCAL},{"utc",PointRecordTime::UTC}};
}

template<typename T> RTX_object::_sp __createShared() {
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
  _v["unitString"] = JSV(u.unitString());
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
      { "sqlite",     &__createShared<SqlitePointRecord>},
      { "influx",     &__createShared<InfluxDbPointRecord>},
      { "odbc",       &__createShared<OdbcDirectPointRecord>},
      { "timeseries", &__createShared<TimeSeries>},
      { "filter",     &__createShared<TimeSeriesFilter>},
      { "units",      &__createShared<Units>},
      { "clock",      &__createShared<Clock>},
      { "units",      &__createShared<Units>}
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
        throw bad_cast();
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
  JSV jsU = _v.as_object()["units"];
  Units::_sp u = static_pointer_cast<Units>(from_json(jsU));
  ts.setUnits(*u.get());
  ts.setName(_v.as_object()["name"].as_string());
};
void DeserializerJson::visit(TimeSeriesFilter& ts) {
  this->visit((TimeSeries&)ts);
  JSV jsC = _v.as_object()["clock"];
  Clock::_sp c = static_pointer_cast<Clock>(from_json(jsC));
  ts.setClock(c);
};
/******* UNITS *******/
void DeserializerJson::visit(Units &u) {
  string uStr = _v.as_object()["unitString"].as_string();
  u = Units::unitOfType(uStr);
};
/******* CLOCK *******/
void DeserializerJson::visit(Clock &c) {
  int p = _v.as_object()["period"].as_integer();
  int o = _v.as_object()["offset"].as_integer();
  c.setPeriod(p);
  c.setStart(o);
};

/******* POINTRECORD *******/
void DeserializerJson::visit(PointRecord& pr) {
  pr.setName(_v.as_object()["name"].as_string());
};
void DeserializerJson::visit(DbPointRecord &pr) {
  this->visit((PointRecord&)pr);
  web::json::object o = _v.as_object();
  if (o.find("readonly") != o.end()) {
    pr.setReadonly(o["readonly"].as_bool());
  }
  else {
    pr.setReadonly(true);
  }
  pr.setConnectionString(o["connectionString"].as_string());
};
void DeserializerJson::visit(SqlitePointRecord &pr) {
  this->visit((DbPointRecord&)pr);
};
void DeserializerJson::visit(InfluxDbPointRecord &pr) {
  this->visit((DbPointRecord&)pr);
};
void DeserializerJson::visit(OdbcDirectPointRecord &pr) {
  this->visit((DbPointRecord&)pr);
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
























