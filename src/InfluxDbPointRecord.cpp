#include <map>
#include <sstream>

#include <curl/curl.h>

//#include <boost/asio.hpp>
//#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

#include <cpprest/uri.h>
#include <cpprest/json.h>
#include <cpprest/http_client.h>

#include "InfluxDbPointRecord.h"
#include "MetricInfo.h"

using namespace std;
using namespace RTX;

const char *kSERIES = "series";
const char *kSHOW_SERIES = "show series";
const char *kERROR = "error";
const char *kRESULTS = "results";

using web::http::method;
using web::http::methods;
using web::http::status_codes;
using web::http::http_response;
using web::http::uri;
using jsValue = web::json::value;
using jsObject = web::json::object;
using jsArray = web::json::array;

#define RTX_INFLUX_CLIENT_TIMEOUT 10

// task wrapper impl
namespace RTX {
  class PplxTaskWrapper : public ITaskWrapper {
  public:
    PplxTaskWrapper();
    pplx::task<void> task;
  };
}
// why the fully private implementation? it's really to guard client applications from having
// to #include the pplx concurrency libs. this way everything is self-contained.
PplxTaskWrapper::PplxTaskWrapper() {
  this->task = pplx::task<void>([]() {
    return; // simple no-op task as filler.
  });
}
#define INFLUX_ASYNC_SEND static_pointer_cast<PplxTaskWrapper>(_sendTask)->task

uri _InfluxDbPointRecord_uriForQuery(InfluxDbPointRecord& record, const std::string& query, bool withTimePrecision = true);
jsValue _InfluxDbPointRecord_jsonFromRequest(InfluxDbPointRecord& record, uri uri, method withMethod = methods::GET);
std::vector<RTX::Point> _InfluxDbPointRecord_pointsFromJson(jsValue json);

/*
 influx will handle units a litte differently since it doesn't have a straightforward k/v store.
 in each metric name, the format is measurement,tag=value,tag=value[,...]
 we use this tag=value to also store units, but we don't want to expose that to the user on this end.
 influx will keep track of it, but we will have to manually intercept that portion of the name bidirectionally.
 */

InfluxDbPointRecord::InfluxDbPointRecord() {
  _lastIdRequest = 0;
  
  useTransactions = true;
  _inBulkOperation = false;
  _sendTask.reset(new PplxTaskWrapper());
}

#pragma mark Connecting

void InfluxDbPointRecord::dbConnect() throw(RtxException) {
  
  _connected = false;
  this->errorMessage = "Connecting...";
  
  // see if the database needs to be created
  bool dbExists = false;
  
  uri uri = _InfluxDbPointRecord_uriForQuery(*this, "SHOW MEASUREMENTS LIMIT 1", false);
  jsValue jsoMeas = _InfluxDbPointRecord_jsonFromRequest(*this,uri);
  if (!jsoMeas.has_field(kRESULTS)) {
    if (jsoMeas.has_field("error")) {
      this->errorMessage = jsoMeas["error"].as_string();
      return;
    }
    else {
      this->errorMessage = "Connect failed: No Database?";
      return;
    }
  }
  
  jsValue resVal = jsoMeas[kRESULTS];
  if (!resVal.is_array() || resVal.as_array().size() == 0) {
    this->errorMessage = "JSON Format Not Recognized";
    return;
  }
  
  // for sure it's an array.
  if (resVal.size() > 0 && resVal[0].has_field(kERROR)) {
    this->errorMessage = resVal[0][kERROR].as_string();
  }
  else {
    dbExists = true;
  }
  
  
  if (!dbExists) {
    // create the database?
    web::http::uri_builder b;
    b.set_scheme("http")
    .set_host(this->conn.host)
    .set_port(this->conn.port)
    .set_path("query")
    .append_query("u", this->conn.user, false)
    .append_query("p", this->conn.pass, false)
    .append_query("q", "CREATE DATABASE " + this->conn.db, true);
    
    jsValue response = _InfluxDbPointRecord_jsonFromRequest(*this, b.to_uri());
    if (response.size() == 0 || !response.has_field(kRESULTS) ) {
      this->errorMessage = "Can't create database";
      return;
    }
  }
  
  // made it this far? at least we are connected.
  _connected = true;
  this->errorMessage = "OK";
  
  return;
}



#pragma mark Listing and creating series


PointRecord::IdentifierUnitsList InfluxDbPointRecord::identifiersAndUnits() {
  
  /*
   
   perform a query to get all the series.
   response will be nested in terms of "measurement", and then each array in the "values" array will denote an individual time series:
   
   series: [
   {   name: flow
   columns:  [asset_id, asset_type, dma, ... ]
   values: [ [33410,    pump,       brecon, ...],
   [33453,    pipe,       mt.\ washington, ...],
   [...]
   ]
   },
   {   name: pressure
   columns:   [asset_id, asset_type, dma, ...]
   values: [  [44305,    junction,   brecon, ...],
   [43205,    junction,   mt.\ washington, ...],
   [...]
   ]
   }
   
   */
  
  // if i'm busy, then don't bother. unless this could be the first query.
  if (_inBulkOperation && !_identifiersAndUnitsCache.empty()) {
    return I_InfluxDbPointRecord::identifiersAndUnits();
  }
  std::lock_guard<std::mutex> lock(_influxMutex);
  
  // quick cache hit. 5-second validity window.
  time_t now = time(NULL);
  if (now - _lastIdRequest < 5 && !_identifiersAndUnitsCache.empty()) {
    return DbPointRecord::identifiersAndUnits();
  }
  _lastIdRequest = now;
  _identifiersAndUnitsCache.clear();
  
  
  if (!this->isConnected()) {
    this->dbConnect();
  }
  if (!this->isConnected()) {
    return _identifiersAndUnitsCache;
  }
  
  uri uri = _InfluxDbPointRecord_uriForQuery(*this, kSHOW_SERIES, false);
  jsValue jsv = _InfluxDbPointRecord_jsonFromRequest(*this, uri);
  
  if (!jsv.has_field("results")) {
    return _identifiersAndUnitsCache;
  }
  
  jsArray resArr = jsv["results"].as_array();
  if (resArr.size() == 0) {
    return _identifiersAndUnitsCache;
  }
  
  jsValue zeroObj = resArr[0];
  if (!zeroObj.has_field(kSERIES)) {
    return _identifiersAndUnitsCache;
  }
  
  jsArray seriesArray = zeroObj["series"].as_array();
  for (auto seriesIt = seriesArray.begin(); seriesIt != seriesArray.end(); ++seriesIt) {
    
    string str = seriesIt->serialize();
    jsObject thisSeries = seriesIt->as_object();
    
    jsArray columns = thisSeries["columns"].as_array(); // the only column is "key"
    jsArray values = thisSeries["values"].as_array(); // array of single-string arrays
    
    for (auto valuesIt = values.begin(); valuesIt != values.end(); ++valuesIt) {
      jsArray singleStrArr = valuesIt->as_array();
      jsValue strVal = singleStrArr[0];
      string dbId = strVal.as_string();
      boost::replace_all(dbId, "\\ ", " ");
      MetricInfo m(dbId);
      // now we have all kv pairs that define a time series.
      // do we have units info? strip it off before showing the user.
      Units units = RTX_NO_UNITS;
      if (m.tags.count("units") > 0) {
        units = Units::unitOfType(m.tags["units"]);
        // remove units from string name.
        m.tags.erase("units");
      }
      // now assemble the complete name and cache it:
      string properId = m.name();
      _identifiersAndUnitsCache.set(properId,units);
    } // for each values array (ts definition)
  }
  
  return _identifiersAndUnitsCache;
}


#pragma mark SELECT

std::vector<Point> InfluxDbPointRecord::selectRange(const std::string& id, TimeRange req_range) {
  // bulk operation is inserts, so skip the lookup.
  //if (_inBulkOperation) {
    //return vector<Point>();
  //}
  std::lock_guard<std::mutex> lock(_influxMutex);
  string dbId = influxIdForTsId(id);
  DbPointRecord::Query q = this->queryPartsFromMetricId(dbId);
  q.where.push_back("time >= " + to_string(req_range.start) + "s");
  q.where.push_back("time <= " + to_string(req_range.end) + "s");
  
  uri uri = _InfluxDbPointRecord_uriForQuery(*this, q.selectStr());
  jsValue jsv = _InfluxDbPointRecord_jsonFromRequest(*this, uri);
  return _InfluxDbPointRecord_pointsFromJson(jsv);
}

Point InfluxDbPointRecord::selectNext(const std::string& id, time_t time) {
  std::vector<Point> points;
  string dbId = influxIdForTsId(id);
  DbPointRecord::Query q = this->queryPartsFromMetricId(dbId);
  q.where.push_back("time > " + to_string(time) + "s");
  q.order = "time asc limit 1";
  
  uri uri = _InfluxDbPointRecord_uriForQuery(*this, q.selectStr());
  jsValue jsv = _InfluxDbPointRecord_jsonFromRequest(*this, uri);
  points = _InfluxDbPointRecord_pointsFromJson(jsv);
  
  if (points.size() == 0) {
    return Point();
  }
  
  return points.front();
}

Point InfluxDbPointRecord::selectPrevious(const std::string& id, time_t time) {
  std::vector<Point> points;
  string dbId = influxIdForTsId(id);
  
  DbPointRecord::Query q = this->queryPartsFromMetricId(dbId);
  q.where.push_back("time < " + to_string(time) + "s");
  q.order = "time desc limit 1";
  
  uri uri = _InfluxDbPointRecord_uriForQuery(*this, q.selectStr());
  jsValue jsv = _InfluxDbPointRecord_jsonFromRequest(*this, uri);
  points = _InfluxDbPointRecord_pointsFromJson(jsv);
  
  if (points.size() == 0) {
    return Point();
  }
  
  return points.front();
}



#pragma mark DELETE

void InfluxDbPointRecord::truncate() {
  this->errorMessage = "Truncating";
  
  stringstream sqlss;
  sqlss << "DROP DATABASE " << this->conn.db << "; CREATE DATABASE " << this->conn.db;
  
  uri uri = _InfluxDbPointRecord_uriForQuery(*this, sqlss.str());
  jsValue v = _InfluxDbPointRecord_jsonFromRequest(*this, uri, methods::POST);
  
  auto ids = _identifiersAndUnitsCache; // copy
  
  DbPointRecord::reset();
  
  this->beginBulkOperation();
  for (auto ts_units : *ids.get()) {
    this->insertIdentifierAndUnits(ts_units.first, ts_units.second);
  }
  this->endBulkOperation();
  
  this->errorMessage = "OK";
  return;

}


void InfluxDbPointRecord::removeRecord(const std::string& id) {
  
  const string dbId = this->influxIdForTsId(id);
  DbPointRecord::Query q = this->queryPartsFromMetricId(id);
  
  stringstream sqlss;
  sqlss << "DROP SERIES FROM " << q.nameAndWhereClause();
  
  uri uri = _InfluxDbPointRecord_uriForQuery(*this, sqlss.str());
  jsValue v = _InfluxDbPointRecord_jsonFromRequest(*this, uri, methods::POST);
}



#pragma mark Query Building
DbPointRecord::Query InfluxDbPointRecord::queryPartsFromMetricId(const std::string& name) {
  MetricInfo m(name);
  
  DbPointRecord::Query q;
  q.select = {"time", "value", "quality", "confidence"};
  q.from = "\"" + m.measurement + "\"";
  
  if (m.tags.size() > 0) {
    for( auto p : m.tags) {
      stringstream ss;
      ss << p.first << "='" << p.second << "'";
      q.where.push_back(ss.str());
    }
  }
  
  return q;
}


uri _InfluxDbPointRecord_uriForQuery(InfluxDbPointRecord& record, const std::string& query, bool withTimePrecision) {
  
  web::http::uri_builder b;
  b.set_scheme("http").set_host(record.conn.host).set_port(record.conn.port).set_path("query")
   .append_query("db", record.conn.db, false)
   .append_query("u", record.conn.user, false)
   .append_query("p", record.conn.pass, false)
   .append_query("q", query, true);
  
  if (withTimePrecision) {
    b.append_query("epoch","s");
  }
  
  return b.to_uri();
}

jsValue _InfluxDbPointRecord_jsonFromRequest(InfluxDbPointRecord& record, uri uri, method withMethod) {
  jsValue js = jsValue::object();
  try {
    web::http::client::http_client_config config;
    config.set_timeout(std::chrono::seconds(RTX_INFLUX_CLIENT_TIMEOUT));
    web::http::client::http_client client(uri, config);
    http_response r = client.request(withMethod).get(); // waits for response
    if (r.status_code() != 204) {
      js = r.extract_json().get();
    }
  }
  catch (std::exception &e) {
    cerr << "exception in GET: " << e.what() << endl;
  }
  return js;
}


#pragma mark Parsing

std::vector<RTX::Point> _InfluxDbPointRecord_pointsFromJson(jsValue json) {
  
  // multiple time series might be returned eventually, but for now it's just a single-value array.
  vector<Point> points;
  
  if (!json.is_object() || json.size() == 0) {
    return points;
  }
  if ( !json.has_field(kRESULTS) ) {
    return points;
  }
  jsValue resultsVal = json[kRESULTS];
  if (!resultsVal.is_array() || resultsVal.size() == 0) {
    return points;
  }
  jsValue firstRes = resultsVal.as_array()[0];
  if ( !firstRes.is_object() || !firstRes.has_field(kSERIES) ) {
    return points;
  }
  jsArray seriesArr = firstRes.as_object()[kSERIES].as_array();
  if (seriesArr.size() == 0) {
    return points;
  }
  jsObject series = seriesArr[0].as_object();
  string measureName = series["name"].as_string();
  
  // create a little map so we know what order the columns are in
  map<string,int> columnMap;
  jsArray cols = series["columns"].as_array();
  for (int i = 0; i < cols.size(); ++i) {
    string colName = cols[i].as_string();
    columnMap[colName] = (int)i;
  }
  
  // check columns are all there
  for (const string &key : {"time","value","quality","confidence"}) {
    if (columnMap.count(key) == 0) {
      cerr << "column map does not contain key: " << key << endl;
      return points;
    }
  }
  
  int timeIndex = columnMap["time"];
  int valueIndex = columnMap["value"];
  int qualityIndex = columnMap["quality"];
  int confidenceIndex = columnMap["confidence"];
  
  // now go through each returned row and create a point.
  // use the column name map to set point properties.
  jsArray rows = series["values"].as_array();
  if (rows.size() == 0) {
    return points;
  }
  
  points.reserve((size_t)rows.size());
  for (jsValue rowV : rows) {
    jsArray row = rowV.as_array();
    time_t t = row[timeIndex].as_integer();
    double v = row[valueIndex].as_double();
    Point::PointQuality q = (Point::PointQuality)(row[qualityIndex].as_integer());
    double c = row[confidenceIndex].as_double();
    Point p(t,v,q,c);
    points.push_back(p);
  }
  
  return points;
}




void InfluxDbPointRecord::sendPointsWithString(const string& content) {
  
  INFLUX_ASYNC_SEND.wait(); // wait on previous send if needed.
  
  const string bodyContent(content);
  INFLUX_ASYNC_SEND = pplx::create_task([&,bodyContent]() {
    std::lock_guard<std::mutex> lock(_influxMutex);
    web::uri_builder b;
    b.set_scheme("http").set_host(this->conn.host).set_port(this->conn.port).set_path("write")
    .append_query("db", this->conn.db, false)
    .append_query("u", this->conn.user, false)
    .append_query("p", this->conn.pass, false)
    .append_query("precision", "s", false);
    
    namespace bio = boost::iostreams;
    std::stringstream compressed;
    std::stringstream origin(bodyContent);
    bio::filtering_streambuf<bio::input> out;
    out.push(bio::gzip_compressor(bio::gzip_params(bio::gzip::best_compression)));
    out.push(origin);
    bio::copy(out, compressed);
    const string zippedContent(compressed.str());
    
    try {
      web::http::client::http_client_config config;
      config.set_timeout(std::chrono::seconds(RTX_INFLUX_CLIENT_TIMEOUT));
      //    credentials not supported in cpprestsdk
      //    config.set_credentials(http::client::credentials(this->user, this->pass));
      web::http::client::http_client client(b.to_uri(), config);
      web::http::http_request req(methods::POST);
      req.set_body(zippedContent);
      req.headers().add("Content-Encoding", "gzip");
      http_response r = client.request(req).get();
      if (r.status_code() == 204) {
        // no content. this is fine.
      }
    } catch (std::exception &e) {
      cerr << "exception POST: " << e.what() << endl;
    }
  });
  
  if (!_inBulkOperation) {
    INFLUX_ASYNC_SEND.wait();
    // block returning unless we are still in a bulk operation.
    // otherwise, carry on. no need to wait - let other processes continue.
  }
  
}

