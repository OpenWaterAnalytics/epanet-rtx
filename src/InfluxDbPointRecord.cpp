#include "InfluxDbPointRecord.h"

#include <boost/asio.hpp>
#include <boost/foreach.hpp>
#include <curl/curl.h>
#include <map>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/replace.hpp>


#include <cpprest/uri.h>
#include <cpprest/json.h>
#include <cpprest/http_client.h>
using namespace web;
using namespace utility;
using namespace http;

using namespace std;
using namespace RTX;
using boost::asio::ip::tcp;

#include <boost/interprocess/sync/scoped_lock.hpp>
using boost::signals2::mutex;
using boost::interprocess::scoped_lock;

#define HTTP_OK 200

const char *kSERIES = "series";
const char *kSHOW_SERIES = "show series";
const char *kERROR = "error";
const char *kRESULTS = "results";

http::uri _InfluxDbPointRecord_uriForQuery(InfluxDbPointRecord& record, const std::string& query, bool withTimePrecision = true);
web::json::value _InfluxDbPointRecord_jsonFromGet(http::uri uri);
std::vector<RTX::Point> _InfluxDbPointRecord_pointsFromJson(json::value json);

/*
 influx will handle units a litte differently since it doesn't have a straightforward k/v store.
 in each metric name, the format is measurement,tag=value,tag=value[,...]
 we use this tag=value to also store units, but we don't want to expose that to the user on this end.
 influx will keep track of it, but we will have to manually intercept that portion of the name bidirectionally.
 */

InfluxDbPointRecord::InfluxDbPointRecord() {
  _connected = false;
  _lastIdRequest = time(NULL);
  host = "HOST";
  user = "USER";
  pass = "PASS";
  port = 8086;
  db = "DB";
  
  useTransactions = true;
  _inBulkOperation = false;
  _mutex.reset(new boost::signals2::mutex);
}

#pragma mark Connecting

void InfluxDbPointRecord::dbConnect() throw(RtxException) {
  
  _connected = false;
  this->errorMessage = "Connecting...";
  
  // see if the database needs to be created
  bool dbExists = false;
  
  http::uri uri = _InfluxDbPointRecord_uriForQuery(*this, "SHOW MEASUREMENTS LIMIT 1", false);
  json::object jsoMeas = _InfluxDbPointRecord_jsonFromGet(uri).as_object();
  auto jsoNOTFOUND = jsoMeas.end();
  if (jsoMeas.empty() || jsoMeas.find(kRESULTS) == jsoNOTFOUND) {
    if (jsoMeas.find("error") != jsoNOTFOUND) {
      this->errorMessage = jsoMeas["error"].as_string();
      return;
    }
    else {
      this->errorMessage = "Connect failed: No Database?";
      return;
    }
  }
  
  json::value resVal = jsoMeas[kRESULTS];
  if (!resVal.is_array() || resVal.as_array().size() == 0) {
    this->errorMessage = "JSON Format Not Recognized";
    return;
  }
  
  auto resFirst = resVal.as_array().begin();
  json::object res = resFirst->as_object();
  if (res.find(kERROR) != res.end()) {
    this->errorMessage = res[kERROR].as_string();
  }
  else {
    dbExists = true;
  }
  
  
  if (!dbExists) {
    // create the database?
    http::uri_builder b;
    b.set_scheme("http")
    .set_host(this->host)
    .set_port(this->port)
    .set_path("query")
    .append_query("u", this->user, false)
    .append_query("p", this->pass, false)
    .append_query("q", "CREATE DATABASE " + this->db, true);
    
    json::object respObj = _InfluxDbPointRecord_jsonFromGet(b.to_uri()).as_object();
    if (respObj.empty() || respObj.find(kRESULTS) == respObj.end()) {
      this->errorMessage = "Can't create database";
      return;
    }
  }
  
  // made it this far? at least we are connected.
  _connected = true;
  this->errorMessage = "OK";
  
  return;
}


string InfluxDbPointRecord::connectionString() {
  stringstream ss;
  ss << "host=" << this->host << "&port=" << this->port << "&db=" << this->db << "&u=" << this->user << "&p=" << this->pass;
  return ss.str();
}

void InfluxDbPointRecord::setConnectionString(const std::string &str) {
  scoped_lock<boost::signals2::mutex> lock(*_mutex);
  // split the tokenized string. we're expecting something like "host=127.0.0.1&port=4242"
  std::map<std::string, std::string> kvPairs;
  {
    boost::regex kvReg("([^=]+)=([^&]+)&?"); // key - value pair
    boost::sregex_iterator it(str.begin(), str.end(), kvReg), end;
    for ( ; it != end; ++it) {
      kvPairs[(*it)[1]] = (*it)[2];
    }
  }
  
  if (kvPairs.count("host")) {
    this->host = kvPairs["host"];
  }
  if (kvPairs.count("port")) {
    int intPort = boost::lexical_cast<int>(kvPairs["port"]);
    this->port = intPort;
  }
  if (kvPairs.count("db")) {
    this->db = kvPairs["db"];
  }
  if (kvPairs.count("u")) {
    this->user = kvPairs["u"];
  }
  if (kvPairs.count("p")) {
    this->pass = kvPairs["p"];
  }
  
  
  return;
}


#pragma mark Listing and creating series


bool InfluxDbPointRecord::insertIdentifierAndUnits(const std::string &id, RTX::Units units) {
  
  MetricInfo m = InfluxDbPointRecord::metricInfoFromName(id);
  m.tags.erase("units"); // get rid of units if they are included.
  string properId = InfluxDbPointRecord::nameFromMetricInfo(m);
  
  {
    scoped_lock<boost::signals2::mutex> lock(*_mutex);
    if (this->readonly()) {
      // already here. ok if units match. otherwise no-no
      return (_identifiersAndUnitsCache.count(properId) && _identifiersAndUnitsCache[properId] == units);
    }
    // otherwise, fine. add the series.
    _identifiersAndUnitsCache[properId] = units;
  }
  
  this->addPoint(id, Point(1,0));
  
  // no futher validation.
  return true;
}



const std::map<std::string,Units> InfluxDbPointRecord::identifiersAndUnits() {
  
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
  {
    scoped_lock<boost::signals2::mutex> lock(*_mutex);
    
    // quick cache hit. 5-second validity window.
    time_t now = time(NULL);
    if (now - _lastIdRequest < 5 && !_identifiersAndUnitsCache.empty()) {
      return DbPointRecord::identifiersAndUnits();
    }
    _lastIdRequest = now;
    
    _identifiersAndUnitsCache.clear();
    
  }
  
  if (!this->isConnected()) {
    this->dbConnect();
  }
  if (!this->isConnected()) {
    return _identifiersAndUnitsCache;
  }
  
  web::uri uri = _InfluxDbPointRecord_uriForQuery(*this, kSHOW_SERIES, false);
  web::json::value jsv = _InfluxDbPointRecord_jsonFromGet(uri);
  
  web::json::object jso = jsv.as_object();
  auto iResult = jso.find("results");
  if (iResult == jso.end()) {
    return _identifiersAndUnitsCache;
  }
  json::value resVal = iResult->second;
  json::array resArr = resVal.as_array();
  auto resIt = resArr.begin();
  json::object resZeroObj = resIt->as_object();
  
  auto seriesArrIt = resZeroObj.find(kSERIES);
  if (seriesArrIt == resZeroObj.end()) {
    return _identifiersAndUnitsCache;
  }
  
  json::array seriesArray = seriesArrIt->second.as_array();
  for (auto seriesIt = seriesArray.begin(); seriesIt != seriesArray.end(); ++seriesIt) {
    
    json::object thisSeries = seriesIt->as_object();
    
    string thisMeasureName = thisSeries["name"].as_string();
    json::array columns = thisSeries["columns"].as_array();
    json::array values = thisSeries["values"].as_array();
    
    for (auto valuesIt = values.begin(); valuesIt != values.end(); ++valuesIt) {
      MetricInfo m;
      m.measurement = thisMeasureName;
      // this is where a time series is defined!
      // parse the timeseries tag key-value pairs, store into metric info
      json::array thisTsVals = valuesIt->as_array();
      int j = 0;
      for (auto thisTsValsIt = thisTsVals.begin(); thisTsValsIt != thisTsVals.end(); ++thisTsValsIt, j++) {
        const string tsKeyStr = columns[j].as_string();
        const string tsValStr = thisTsValsIt->as_string();
        // exclude internal influx _key:
        if (RTX_STRINGS_ARE_EQUAL(tsKeyStr, "_key")) {
          continue;
        }
        // exclude empty valued keys
        if (RTX_STRINGS_ARE_EQUAL(tsValStr, "")) {
          continue;
        }
        m.tags[tsKeyStr] = tsValStr;
      }
      
      // now we have all kv pairs that define a time series.
      // do we have units info? strip it off before showing the user.
      Units units = RTX_NO_UNITS;
      if (m.tags.find("units") != m.tags.end()) {
        units = Units::unitOfType(m.tags["units"]);
        // remove units from string name.
        m.tags.erase("units");
      }
      
      // now assemble the complete name and cache it:
      string properId = InfluxDbPointRecord::nameFromMetricInfo(m);
      _identifiersAndUnitsCache[properId] = units;
      
    } // for each values array (ts definition)
    
  }
  
  return _identifiersAndUnitsCache;
}


InfluxDbPointRecord::MetricInfo InfluxDbPointRecord::metricInfoFromName(const std::string &name) {
  
  MetricInfo m;
  size_t firstComma = name.find(",");
  // measure name is everything up to the first comma, even if that's everything
  m.measurement = name.substr(0,firstComma);
  
  if (firstComma != string::npos) {
    // a comma was found. therefore treat the name as tokenized
    string keysValuesStr = name.substr(firstComma+1);
    boost::regex kvReg("([^=]+)=([^,]+),?"); // key - value pair
    boost::sregex_iterator it(keysValuesStr.begin(), keysValuesStr.end(), kvReg), end;
    for ( ; it != end; ++it) {
      m.tags[(*it)[1]] = (*it)[2];
    }
  }
  return m;
}

const string InfluxDbPointRecord::nameFromMetricInfo(RTX::InfluxDbPointRecord::MetricInfo info) {
  stringstream ss;
  ss << info.measurement;
  typedef pair<string,string> stringPair;
  BOOST_FOREACH( stringPair p, info.tags) {
    ss << "," << p.first << "=" << p.second;
  }
  const string name = ss.str();
  return name;
}

std::string InfluxDbPointRecord::properId(const std::string& id) {
  return InfluxDbPointRecord::nameFromMetricInfo(InfluxDbPointRecord::metricInfoFromName(id));
}


string InfluxDbPointRecord::_influxIdForTsId(const string& id) {
  // put named keys in proper order...
  MetricInfo m = InfluxDbPointRecord::metricInfoFromName(id);
  if (m.tags.count("units")) {
    m.tags.erase("units");
  }
  string tsId = InfluxDbPointRecord::nameFromMetricInfo(m);
  
  if (_identifiersAndUnitsCache.count(tsId) == 0) {
    cerr << "no registered ts with that id: " << tsId << endl;
    return "";
  }
  
  Units u = _identifiersAndUnitsCache[tsId];
  m.tags["units"] = u.unitString();
  string dbId = InfluxDbPointRecord::nameFromMetricInfo(m);
  return dbId;
}

#pragma mark SELECT

std::vector<Point> InfluxDbPointRecord::selectRange(const std::string& id, time_t startTime, time_t endTime) {
  std::vector<Point> points;
  string dbId = _influxIdForTsId(id);
  DbPointRecord::Query q = this->queryPartsFromMetricId(dbId);
  q.where.push_back("time >= " + to_string(startTime) + "s");
  q.where.push_back("time <= " + to_string(endTime) + "s");
  
  http::uri uri = _InfluxDbPointRecord_uriForQuery(*this, q.selectStr());
  json::value jsv = _InfluxDbPointRecord_jsonFromGet(uri);
  return _InfluxDbPointRecord_pointsFromJson(jsv);
}

Point InfluxDbPointRecord::selectNext(const std::string& id, time_t time) {
  std::vector<Point> points;
  string dbId = _influxIdForTsId(id);
  DbPointRecord::Query q = this->queryPartsFromMetricId(dbId);
  q.where.push_back("time > " + to_string(time) + "s");
  q.order = "time asc limit 1";
  
  http::uri uri = _InfluxDbPointRecord_uriForQuery(*this, q.selectStr());
  json::value jsv = _InfluxDbPointRecord_jsonFromGet(uri);
  points = _InfluxDbPointRecord_pointsFromJson(jsv);
  
  if (points.size() == 0) {
    return Point();
  }
  
  return points.front();
}

Point InfluxDbPointRecord::selectPrevious(const std::string& id, time_t time) {
  std::vector<Point> points;
  string dbId = _influxIdForTsId(id);
  
  DbPointRecord::Query q = this->queryPartsFromMetricId(dbId);
  q.where.push_back("time < " + to_string(time) + "s");
  q.order = "time desc limit 1";
  
  http::uri uri = _InfluxDbPointRecord_uriForQuery(*this, q.selectStr());
  json::value jsv = _InfluxDbPointRecord_jsonFromGet(uri);
  points = _InfluxDbPointRecord_pointsFromJson(jsv);
  
  if (points.size() == 0) {
    return Point();
  }
  
  return points.front();
}

#pragma mark INSERT

void InfluxDbPointRecord::insertSingle(const std::string& id, Point point) {
  
  vector<Point> points;
  points.push_back(point);
  
  this->insertRange(id, points);
  
}

void InfluxDbPointRecord::insertRange(const std::string& id, std::vector<Point> points) {
  vector<Point> insertionPoints;
  string dbId = _influxIdForTsId(id);
  
  if (points.size() == 0) {
    return;
  }
  
  // is there anything here?
  string q = "select count(value) from " + dbId;
  http::uri uri = _InfluxDbPointRecord_uriForQuery(*this, q, false);
  json::value jsv = _InfluxDbPointRecord_jsonFromGet(uri);
  
  
  vector<Point> existing;
  existing = this->selectRange(dbId, points.front().time - 1, points.back().time + 1);
  map<time_t,bool> existingMap;
  BOOST_FOREACH(const Point& p, existing) {
    existingMap[p.time] = true;
  }
  
  BOOST_FOREACH(const Point& p, points) {
    if (existingMap.count(p.time) == 0) {
      insertionPoints.push_back(p);
    }
  }
  
  if (insertionPoints.size() == 0) {
    return;
  }
  
  const string content = this->insertionLineFromPoints(dbId, insertionPoints);
  
  if (_inBulkOperation) {
    _transactionLines.push_back(content);
  }
  else {
    this->sendPointsWithString(content);
  }
  
}

#pragma mark TRANSACTION / BULK OPERATIONS

void InfluxDbPointRecord::beginBulkOperation() {
  _inBulkOperation = true;
  _transactionLines.clear();
}

void InfluxDbPointRecord::endBulkOperation(){
  this->commitTransactionLines();
  _inBulkOperation = false;
}

void InfluxDbPointRecord::commitTransactionLines() {
  string lines;
  int i = 0;
  BOOST_FOREACH(const string& line, _transactionLines) {
    if (i != 0) {
      lines.append("\n");
    }
    lines.append(line);
    ++i;
  }
  this->sendPointsWithString(lines);
}


#pragma mark DELETE

void InfluxDbPointRecord::removeRecord(const std::string& id) {
  // to-do fix this. influx bug related to dropping a series:
  //return;
  const string dbId = this->_influxIdForTsId(id);
  DbPointRecord::Query q = this->queryPartsFromMetricId(id);
  
  stringstream sqlss;
  sqlss << "DROP SERIES FROM " << q.nameAndWhereClause();
  
  http::uri uri = _InfluxDbPointRecord_uriForQuery(*this, sqlss.str());
  json::value v = _InfluxDbPointRecord_jsonFromGet(uri);
}

void InfluxDbPointRecord::truncate() {
  
  http::uri_builder b;
  b.set_scheme("http")
  .set_host(this->host)
  .set_port(this->port)
  .set_path("query")
  .append_query("u", this->user, false)
  .append_query("p", this->pass, false)
  .append_query("q", "DROP DATABASE " + this->db, true);
  
  json::value v = _InfluxDbPointRecord_jsonFromGet(b.to_uri());
  
  // reconnecting will re-create the database.
  this->dbConnect();
}


#pragma mark Query Building
DbPointRecord::Query InfluxDbPointRecord::queryPartsFromMetricId(const std::string& name) {
  MetricInfo m = InfluxDbPointRecord::metricInfoFromName(name);
  
  DbPointRecord::Query q;
  
  q.from = "\"" + m.measurement + "\"";
  
  typedef pair<string,string> stringPair;
  if (m.tags.size() > 0) {
    BOOST_FOREACH( stringPair p, m.tags) {
      stringstream ss;
      ss << p.first << "='" << p.second << "'";
      q.where.push_back(ss.str());
    }
  }
  
  return q;
}


http::uri _InfluxDbPointRecord_uriForQuery(InfluxDbPointRecord& record, const std::string& query, bool withTimePrecision) {
  
  http::uri_builder b;
  b.set_scheme("http").set_host(record.host).set_port(record.port).set_path("query")
   .append_query("db", record.db, false)
   .append_query("u", record.user, false)
   .append_query("p", record.pass, false)
   .append_query("q", query, true);
  
  if (withTimePrecision) {
    b.append_query("epoch","s");
  }
  
  return b.to_uri();
}


web::json::value _InfluxDbPointRecord_jsonFromGet(http::uri uri) {
  web::json::value js = web::json::value::object();
  
  try {
    web::http::client::http_client client(uri);
    http_response r = client.request(methods::GET).get(); // waits for response
    if (r.status_code() == 200) {
      js = r.extract_json().get();
    }
  }
  catch(...) {
    
  }
  return js;
}


#pragma mark Parsing

std::vector<RTX::Point> _InfluxDbPointRecord_pointsFromJson(json::value json) {
  
  // multiple time series might be returned eventually, but for now it's just a single-value array.
  vector<Point> points;
  
  json::object jso = json.as_object();
  if (!json.is_object() || jso.empty()) {
    return points;
  }
  if (jso.find(kRESULTS) == jso.end()) {
    return points;
  }
  json::value resultsVal = jso[kRESULTS];
  if (!resultsVal.is_array() || resultsVal.as_array().size() == 0) {
    return points;
  }
  json::value firstRes = resultsVal.as_array()[0];
  if (!firstRes.is_object() || firstRes.as_object().find(kSERIES) == firstRes.as_object().end()) {
    return points;
  }
  json::array seriesArr = firstRes.as_object()[kSERIES].as_array();
  if (seriesArr.size() == 0) {
    return points;
  }
  json::object series = seriesArr[0].as_object();
  string measureName = series["name"].as_string();
  
  // create a little map so we know what order the columns are in
  map<string,int> columnMap;
  json::array cols = series["columns"].as_array();
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
  json::array rows = series["values"].as_array();
  if (rows.size() == 0) {
    return points;
  }
  
  points.reserve((size_t)rows.size());
  for (json::value rowV : rows) {
    json::array row = rowV.as_array();
    time_t t = row[timeIndex].as_integer();
    double v = row[valueIndex].as_double();
    Point::PointQuality q = (Point::PointQuality)(row[qualityIndex].as_integer());
    double c = row[confidenceIndex].as_double();
    Point p(t,v,q,c);
    points.push_back(p);
  }
  
  return points;
}



const string InfluxDbPointRecord::insertionLineFromPoints(const string& tsName, vector<Point> points) {
  /*
   As you can see in the example below, you can post multiple points to multiple series at the same time by separating each point with a new line. Batching points in this manner will result in much higher performance.
   
   curl -i -XPOST 'http://localhost:8086/write?db=mydb' --data-binary '
   cpu_load_short,host=server01,region=us-west value=0.64
   cpu_load_short,host=server02,region=us-west value=0.55 1422568543702900257 
   cpu_load_short,direction=in,host=server01,region=us-west value=23422.0 1422568543702900257'
   */
  
  // escape any spaces in the tsName
  string tsNameEscaped = tsName;
  boost::replace_all(tsNameEscaped, " ", "\\ ");
  
  stringstream ss;
  int i = 0;
  BOOST_FOREACH(const Point& p, points) {
    if (i > 0) {
      ss << '\n';
    }
    string valueStr = to_string(p.value); // influxdb 0.10 supports integers, but only when followed by trailing "i"
    ss << tsNameEscaped << " value=" << valueStr << "," << "quality=" << (int)p.quality << "i," << "confidence=" << p.confidence << " " << p.time;
    ++i;
  }
  
  string data = ss.str();
  return data;
}


void InfluxDbPointRecord::sendPointsWithString(const string& content) {
  
  http::uri_builder b;
  b.set_scheme("http").set_host(this->host).set_port(this->port).set_path("write")
  .append_query("db", this->db, false)
  .append_query("u", this->user, false)
  .append_query("p", this->pass, false)
  .append_query("precision", "s", false);
  
  web::http::client::http_client client(b.to_uri());
  http_response r = client.request(methods::POST, "", content).get(); // waits for response
  if (r.status_code() == 204) {
    // no content. this is fine.
  }
}

