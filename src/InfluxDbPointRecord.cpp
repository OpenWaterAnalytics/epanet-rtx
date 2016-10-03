#include <map>
#include <sstream>

#include <curl/curl.h>

#include <boost/asio.hpp>
#include <boost/regex.hpp>
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

using namespace std;
using namespace RTX;
using boost::asio::ip::tcp;

using boost::signals2::mutex;
using boost::interprocess::scoped_lock;

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
const unsigned int __maxTransactionLines(100);

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
  _connected = false;
  _lastIdRequest = 0;
  host = "HOST";
  user = "USER";
  pass = "PASS";
  port = 8086;
  db = "DB";
  
  useTransactions = true;
  _inBulkOperation = false;
  _mutex.reset(new boost::signals2::mutex);
  _sendTask.reset(new PplxTaskWrapper());
}

#pragma mark Connecting

void InfluxDbPointRecord::dbConnect() throw(RtxException) {
  
  _connected = false;
  this->errorMessage = "Connecting...";
  
  // see if the database needs to be created
  bool dbExists = false;
  
  uri uri = _InfluxDbPointRecord_uriForQuery(*this, "SHOW MEASUREMENTS LIMIT 1", false);
  jsObject jsoMeas = _InfluxDbPointRecord_jsonFromRequest(*this,uri).as_object();
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
  
  jsValue resVal = jsoMeas[kRESULTS];
  if (!resVal.is_array() || resVal.as_array().size() == 0) {
    this->errorMessage = "JSON Format Not Recognized";
    return;
  }
  
  auto resFirst = resVal.as_array().begin();
  jsObject res = resFirst->as_object();
  if (res.find(kERROR) != res.end()) {
    this->errorMessage = res[kERROR].as_string();
  }
  else {
    dbExists = true;
  }
  
  
  if (!dbExists) {
    // create the database?
    web::http::uri_builder b;
    b.set_scheme("http")
    .set_host(this->host)
    .set_port(this->port)
    .set_path("query")
    .append_query("u", this->user, false)
    .append_query("p", this->pass, false)
    .append_query("q", "CREATE DATABASE " + this->db, true);
    
    jsObject respObj = _InfluxDbPointRecord_jsonFromRequest(*this, b.to_uri()).as_object();
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
      // if it's here then it's here. if not, too bad.
      return (this->exists(properId, units));
    }
    // otherwise, great. add the series.
    _identifiersAndUnitsCache.set(properId, units);
    _lastIdRequest = time(NULL);
  }
  
  // insert a field key/value for something that we won't ever query again.
  // pay attention to bulk operations here, since we may be inserting new ids en-masse
  string tsNameEscaped = _influxIdForTsId(id);
  boost::replace_all(tsNameEscaped, " ", "\\ ");
  const string content(tsNameEscaped + " exist=true");
  if (_inBulkOperation) {
    scoped_lock<boost::signals2::mutex> lock(*_mutex);
    _transactionLines.push_back(content);
  }
  else {
    this->sendPointsWithString(content);
  }
  // no futher validation.
  return true;
}



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
    return DbPointRecord::identifiersAndUnits();
  }
  scoped_lock<boost::signals2::mutex> lock(*_mutex);
  
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
  
  jsObject jso = jsv.as_object();
  auto iResult = jso.find("results");
  if (iResult == jso.end()) {
    return _identifiersAndUnitsCache;
  }
  jsValue resVal = iResult->second;
  jsArray resArr = resVal.as_array();
  auto resIt = resArr.begin();
  jsObject resZeroObj = resIt->as_object();
  
  auto seriesArrIt = resZeroObj.find(kSERIES);
  if (seriesArrIt == resZeroObj.end()) {
    return _identifiersAndUnitsCache;
  }
  
  jsArray seriesArray = seriesArrIt->second.as_array();
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
      MetricInfo m = this->metricInfoFromName(dbId);
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
      _identifiersAndUnitsCache.set(properId,units);
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
  for (auto p : info.tags) {
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
  
  if (_identifiersAndUnitsCache.get()->count(tsId) == 0) {
    cerr << "no registered ts with that id: " << tsId << endl;
    // yet i'm being asked for it??
    
    
    return "";
  }
  
  Units u = (*_identifiersAndUnitsCache.get())[tsId];
  m.tags["units"] = u.unitString();
  string dbId = InfluxDbPointRecord::nameFromMetricInfo(m);
  return dbId;
}

#pragma mark SELECT

std::vector<Point> InfluxDbPointRecord::selectRange(const std::string& id, time_t startTime, time_t endTime) {
  // bulk operation is inserts, so skip the lookup.
  if (_inBulkOperation) {
    return vector<Point>();
  }
  scoped_lock<boost::signals2::mutex> lock(*_mutex);
  string dbId = _influxIdForTsId(id);
  DbPointRecord::Query q = this->queryPartsFromMetricId(dbId);
  q.where.push_back("time >= " + to_string(startTime) + "s");
  q.where.push_back("time <= " + to_string(endTime) + "s");
  
  uri uri = _InfluxDbPointRecord_uriForQuery(*this, q.selectStr());
  jsValue jsv = _InfluxDbPointRecord_jsonFromRequest(*this, uri);
  return _InfluxDbPointRecord_pointsFromJson(jsv);
}

Point InfluxDbPointRecord::selectNext(const std::string& id, time_t time) {
  std::vector<Point> points;
  string dbId = _influxIdForTsId(id);
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
  string dbId = _influxIdForTsId(id);
  
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

#pragma mark INSERT

void InfluxDbPointRecord::insertSingle(const std::string& id, Point point) {
  this->insertRange(id, {point});
}

void InfluxDbPointRecord::insertRange(const std::string& id, std::vector<Point> points) {
  vector<Point> insertionPoints;
  string dbId = _influxIdForTsId(id);
  
  if (points.size() == 0) {
    return;
  }
  
  // is there anything here?
  /*
  string q = "select count(value) from " + dbId;
  http::uri uri = _InfluxDbPointRecord_uriForQuery(*this, q, false);
  jsValue jsv = _InfluxDbPointRecord_jsonFromGet(uri);
  */
  
  vector<Point> existing;
  //existing = this->selectRange(dbId, points.front().time - 1, points.back().time + 1);
  map<time_t,bool> existingMap;
  for (const auto &p : existing) {
    existingMap[p.time] = true;
  }
  
  for(const auto& p : points) {
    if (existingMap.count(p.time) == 0) {
      insertionPoints.push_back(p);
    }
  }
  
  if (insertionPoints.size() == 0) {
    return;
  }
  
  const string content = this->insertionLineFromPoints(dbId, insertionPoints);
  
  if (_inBulkOperation) {
    size_t nLines = 0;
    {
      scoped_lock<boost::signals2::mutex> lock(*_mutex);
      _transactionLines.push_back(content);
      nLines = _transactionLines.size();
    }
    if (nLines > __maxTransactionLines) {
      this->commitTransactionLines();
    }
  }
  else {
    this->sendPointsWithString(content);
  }
  
}

#pragma mark TRANSACTION / BULK OPERATIONS

void InfluxDbPointRecord::beginBulkOperation() {
  _inBulkOperation = true;
  {
    scoped_lock<boost::signals2::mutex> lock(*_mutex);
    _transactionLines.clear();
  }
}

void InfluxDbPointRecord::endBulkOperation(){
  this->commitTransactionLines();
  _inBulkOperation = false;
}

void InfluxDbPointRecord::commitTransactionLines() {
  string lines;
  int i = 0;
  {
    scoped_lock<boost::signals2::mutex> lock(*_mutex);
    for(const string line : _transactionLines) {
      lines.append(line);
      lines.append("\n");
      ++i;
    }
    _transactionLines.clear();
  }
  this->sendPointsWithString(lines);
}


#pragma mark DELETE

void InfluxDbPointRecord::removeRecord(const std::string& id) {
  
  const string dbId = this->_influxIdForTsId(id);
  DbPointRecord::Query q = this->queryPartsFromMetricId(id);
  
  stringstream sqlss;
  sqlss << "DROP SERIES FROM " << q.nameAndWhereClause();
  
  uri uri = _InfluxDbPointRecord_uriForQuery(*this, sqlss.str());
  jsValue v = _InfluxDbPointRecord_jsonFromRequest(*this, uri, methods::POST);
}

void InfluxDbPointRecord::truncate() {
  this->errorMessage = "Truncating";
  auto ids = _identifiersAndUnitsCache; // copy
  
  
  for (auto ts_units : *ids.get()) {
    this->removeRecord(ts_units.first);
    
    /*
    string id = ts_units.first;
    MetricInfo m = InfluxDbPointRecord::metricInfoFromName(id);
    string measureName = m.measurement;
    stringstream sqlss;
    sqlss << "DROP MEASUREMENT " << measureName;
    http::uri uri = _InfluxDbPointRecord_uriForQuery(*this, sqlss.str(), false);
    jsValue v = _InfluxDbPointRecord_jsonFromRequest(*this, uri, methods::POST);
     */
  }
  
  DbPointRecord::reset();
  
  
  this->beginBulkOperation();
  for (auto ts_units : *ids.get()) {
    this->insertIdentifierAndUnits(ts_units.first, ts_units.second);
  }
  this->endBulkOperation();
  
  this->errorMessage = "OK";
  return;
}


#pragma mark Query Building
DbPointRecord::Query InfluxDbPointRecord::queryPartsFromMetricId(const std::string& name) {
  MetricInfo m = InfluxDbPointRecord::metricInfoFromName(name);
  
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

jsValue _InfluxDbPointRecord_jsonFromRequest(InfluxDbPointRecord& record, uri uri, method withMethod) {
  jsValue js = jsValue::object();
  try {
    web::http::client::http_client_config config;
    config.set_timeout(std::chrono::seconds(60));
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
  
  jsObject jso = json.as_object();
  if (!json.is_object() || jso.empty()) {
    return points;
  }
  if (jso.find(kRESULTS) == jso.end()) {
    return points;
  }
  jsValue resultsVal = jso[kRESULTS];
  if (!resultsVal.is_array() || resultsVal.as_array().size() == 0) {
    return points;
  }
  jsValue firstRes = resultsVal.as_array()[0];
  if (!firstRes.is_object() || firstRes.as_object().find(kSERIES) == firstRes.as_object().end()) {
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
  for(const Point& p: points) {
    if (i > 0) {
      ss << '\n';
    }
    string valueStr = to_string(p.value); // influxdb 0.10+ supports integers, but only when followed by trailing "i"
    ss << tsNameEscaped << " value=" << valueStr << "," << "quality=" << (int)p.quality << "i," << "confidence=" << p.confidence << " " << p.time;
    ++i;
  }
  
  string data = ss.str();
  return data;
}


void InfluxDbPointRecord::sendPointsWithString(const string& content) {
  
  INFLUX_ASYNC_SEND.wait(); // wait on previous send if needed.
  
  const string bodyContent(content);
  INFLUX_ASYNC_SEND = pplx::create_task([&,bodyContent]() {
    scoped_lock<boost::signals2::mutex> lock(*_mutex);
    web::uri_builder b;
    b.set_scheme("http").set_host(this->host).set_port(this->port).set_path("write")
    .append_query("db", this->db, false)
    .append_query("u", this->user, false)
    .append_query("p", this->pass, false)
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
      config.set_timeout(std::chrono::seconds(60));
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

