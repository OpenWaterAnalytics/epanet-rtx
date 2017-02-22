#include <regex>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/replace.hpp>

#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>

#include <cpprest/uri.h>
#include <cpprest/json.h>
#include <cpprest/http_client.h>

#include <boost/asio.hpp>

#include "InfluxAdapter.h"

#include "MetricInfo.h"

#define RTX_INFLUX_CLIENT_TIMEOUT 15

using namespace std;
using namespace RTX;

using web::http::uri;
using jsValue = web::json::value;

using web::http::method;
using web::http::methods;
using web::http::status_codes;
using web::http::http_response;
using web::http::uri;
using jsObject = web::json::object;
using jsArray = web::json::array;



/***************************************************************************************/
InfluxAdapter::connectionInfo::connectionInfo() {
  proto = "HTTP";
  host = "HOST";
  user = "USER";
  pass = "PASS";
  db = "DB";
  port = 8086;
}
/***************************************************************************************/

InfluxAdapter::InfluxAdapter( errCallback_t cb ) : DbAdapter(cb) {
  
}
InfluxAdapter::~InfluxAdapter() {
  
}


void InfluxAdapter::setConnectionString(const std::string& str) {
  _RTX_DB_SCOPED_LOCK;
  
  regex kvReg("([^=]+)=([^&]+)&?"); // key - value pair
  // split the tokenized string. we're expecting something like "host=127.0.0.1&port=4242"
  std::map<std::string, std::string> kvPairs;
  {
    auto kv_begin = sregex_iterator(str.begin(), str.end(), kvReg);
    auto kv_end = sregex_iterator();
    for (auto it = kv_begin ; it != kv_end; ++it) {
      kvPairs[(*it)[1]] = (*it)[2];
    }
  }
  
  const map<string, function<void(string)> > 
  kvSetters({
    {"proto", [&](string v){this->conn.proto = v;}},
    {"host", [&](string v){this->conn.host = v;}},
    {"port", [&](string v){this->conn.port = boost::lexical_cast<int>(v);}},
    {"db", [&](string v){this->conn.db = v;}},
    {"u", [&](string v){this->conn.user = v;}},
    {"p", [&](string v){this->conn.pass = v;}}
  }); 
  
  for (auto kv : kvPairs) {
    if (kvSetters.count(kv.first) > 0) {
      kvSetters.at(kv.first)(kv.second);
    }
    else {
      cerr << "key not recognized: " << kv.first << " - skipping." << '\n' << flush;
    }
  }
  
  return;
}



void InfluxAdapter::beginTransaction() {
  if (_inTransaction) {
    return;
  }
  _inTransaction = true;
  {
    _RTX_DB_SCOPED_LOCK;
    _transactionLines.clear();
  }
}
void InfluxAdapter::endTransaction() {
  if (!_inTransaction) {
    return;
  }
  this->commitTransactionLines();
  _inTransaction = false;
}

void InfluxAdapter::commitTransactionLines() {
  {
    _RTX_DB_SCOPED_LOCK;
    if (_transactionLines.size() == 0) {
      return;
    }
  }
  {
    string concatLines;
    _RTX_DB_SCOPED_LOCK;
    auto curs = _transactionLines.begin();
    auto end = _transactionLines.end();
    size_t iLine = 0;
    while (curs != end) {
      const string str = *curs;
      ++iLine;
      concatLines.append(str);
      concatLines.append("\n");
      // check for max lines (must respect max lines per send event)
      if (iLine >= this->maxTransactionLines()) {
        // push these lines and clear the queue
        this->sendPointsWithString(concatLines);
        concatLines.clear();
        iLine = 0;
      }
      ++curs;
    }
    if (iLine > 0) {
      // push all/remaining points out
      this->sendPointsWithString(concatLines);
    }
    _transactionLines.clear();
  }
}


bool InfluxAdapter::insertIdentifierAndUnits(const std::string &id, RTX::Units units) {
  
  MetricInfo m(id);
  m.tags.erase("units"); // get rid of units if they are included.
  string properId = m.name();
  
  _idCache.set(properId, units);
  
  // insert a field key/value for something that we won't ever query again.
  // pay attention to bulk operations here, since we may be inserting new ids en-masse
  string tsNameEscaped = influxIdForTsId(id);
  boost::replace_all(tsNameEscaped, " ", "\\ ");
  const string content(tsNameEscaped + " exist=true");
  if (_inTransaction) {
    _RTX_DB_SCOPED_LOCK;
    _transactionLines.push_back(content);
  }
  else {
    this->sendPointsWithString(content);
  }
  // no futher validation.
  return true;
}


void InfluxAdapter::insertSingle(const std::string& id, Point point) {
  this->insertRange(id, {point});
}

void InfluxAdapter::insertRange(const std::string& id, std::vector<Point> points) {
  if (points.size() == 0) {
    return;
  }
  string dbId = influxIdForTsId(id);
  auto content = this->insertionLinesFromPoints(dbId, points);
  
  if (_inTransaction) {
    size_t nLines = 0;
    { // mutex
      _RTX_DB_SCOPED_LOCK;
      for (auto s : content) {
        _transactionLines.push_back(s);
      }
      nLines = _transactionLines.size();
    } // end mutex
    if (nLines > maxTransactionLines()) {
      this->commitTransactionLines();
    }
  }
  else {
    _transactionLines = content;
    this->commitTransactionLines();
  }
  
}


void InfluxAdapter::sendInfluxString(time_t time, const string& seriesId, const string& values) {
  
  string tsNameEscaped = seriesId;
  boost::replace_all(tsNameEscaped, " ", "\\ ");
  
  stringstream ss;
  string timeStr = this->formatTimestamp(time);
  
  ss << tsNameEscaped << " " << values << " " << timeStr;
  string data = ss.str();
  
  
  if (_inTransaction) {
    size_t nLines = 0;
    {
      _RTX_DB_SCOPED_LOCK;
      _transactionLines.push_back(data);
      nLines = _transactionLines.size();
    }
    if (nLines > maxTransactionLines()) {
      this->commitTransactionLines();
    }
  }
  else {
    this->sendPointsWithString(data);
  }
}

string InfluxAdapter::influxIdForTsId(const string& id) {
  // sort named keys into proper order...
  MetricInfo m(id);
  if (m.tags.count("units")) {
    m.tags.erase("units");
  }
  string tsId = m.name();
  if (_idCache.get()->count(tsId) == 0) {
    cerr << "no registered ts with that id: " << tsId << endl;
    // yet i'm being asked for it??
    return "";
  }
  Units u = (*_idCache.get())[tsId];
  m.tags["units"] = u.to_string();
  return m.name();
}


vector<string> InfluxAdapter::insertionLinesFromPoints(const string& tsName, vector<Point> points) {
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
  vector<string> outData;
  
  for(const Point& p: points) {
    stringstream ss;
    string valueStr = to_string(p.value); // influxdb 0.10+ supports integers, but only when followed by trailing "i"
    string timeStr = this->formatTimestamp(p.time);
    ss << tsNameEscaped << " value=" << valueStr << "," << "quality=" << (int)p.quality << "i," << "confidence=" << p.confidence << " " << timeStr;
    outData.push_back(ss.str());
  }
  
  return outData;
}

bool InfluxAdapter::assignUnitsToRecord(const std::string& name, const Units& units) {
  return false;
}



/****************************************************************************************************/
/****************************************************************************************************/
/****************************************************************************************************/
const char *kSERIES = "series";
const char *kSHOW_SERIES = "show series";
const char *kERROR = "error";
const char *kRESULTS = "results";
// INFLUX TCP
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


std::string InfluxTcpAdapter::Query::selectStr() {
  stringstream ss;
  ss << "SELECT ";
  if (this->select.size() == 0) {
    ss << "*";
  }
  else {
    ss << boost::algorithm::join(this->select,", ");
  }
  ss << " FROM " << this->nameAndWhereClause();
  if (this->order.length() > 0) {
    ss << " ORDER BY " << this->order;
  }
  return ss.str();
}
std::string InfluxTcpAdapter::Query::nameAndWhereClause() {
  stringstream ss;
  ss << this->from;
  if (this->where.size() > 0) {
    ss << " WHERE " << boost::algorithm::join(this->where," AND ");
  }
  return ss.str();
}


jsValue __jsonFromRequest(uri uri, method withMethod = methods::GET);
std::vector<RTX::Point> __pointsFromJson(jsValue json);




InfluxTcpAdapter::InfluxTcpAdapter( errCallback_t cb) : InfluxAdapter(cb) {
  _sendTask.reset(new PplxTaskWrapper());
}

InfluxTcpAdapter::~InfluxTcpAdapter() {
  
}

const DbAdapter::adapterOptions InfluxTcpAdapter::options() const {
  DbAdapter::adapterOptions o;
  
  o.supportsUnitsColumn = true;
  o.supportsSinglyBoundQuery = true;
  o.searchIteratively = false;
  o.canAssignUnits = false;
  o.implementationReadonly = false;
  
  return o;
}

std::string InfluxTcpAdapter::connectionString() {
  stringstream ss;
  ss << "proto=" << this->conn.proto << "&host=" << this->conn.host << "&port=" << this->conn.port << "&db=" << this->conn.db << "&u=" << this->conn.user << "&p=" << this->conn.pass;
  return ss.str();
}

void InfluxTcpAdapter::doConnect() { 
  _connected = false;
  _errCallback("Connecting...");
  
  // see if the database needs to be created
  bool dbExists = false;
  
  uri uri = uriForQuery("SHOW MEASUREMENTS LIMIT 1", false);
  jsValue jsoMeas = __jsonFromRequest(uri);
  if (!jsoMeas.has_field(kRESULTS)) {
    if (jsoMeas.has_field("error")) {
      _errCallback(jsoMeas["error"].as_string());
      return;
    }
    else {
      _errCallback("Connect failed: No Database?");
      return;
    }
  }
  
  jsValue resVal = jsoMeas[kRESULTS];
  if (!resVal.is_array() || resVal.as_array().size() == 0) {
    _errCallback("JSON Format Not Recognized");
    return;
  }
  
  // for sure it's an array.
  if (resVal.size() > 0 && resVal[0].has_field(kERROR)) {
    _errCallback(resVal[0][kERROR].as_string());
  }
  else {
    dbExists = true;
  }
  
  
  if (!dbExists) {
    // create the database?
    web::http::uri_builder b;
    b.set_scheme(this->conn.proto)
    .set_host(this->conn.host)
    .set_port(this->conn.port)
    .set_path("query")
    .append_query("u", this->conn.user, false)
    .append_query("p", this->conn.pass, false)
    .append_query("q", "CREATE DATABASE " + this->conn.db, true);
    
    jsValue response = __jsonFromRequest(b.to_uri());
    if (response.size() == 0 || !response.has_field(kRESULTS) ) {
      _errCallback("Can't create database");
      return;
    }
  }
  
  // made it this far? at least we are connected.
  _connected = true;
  _errCallback("OK");
  
  return;

}

IdentifierUnitsList InfluxTcpAdapter::idUnitsList() {
  
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
  
  IdentifierUnitsList ids;
  
  // if i'm busy, then don't bother. unless this could be the first query.
  if (_inTransaction) {
    return _idCache;
  }
  _RTX_DB_SCOPED_LOCK;
  
  
  
  uri uri = uriForQuery(kSHOW_SERIES, false);
  jsValue jsv = __jsonFromRequest(uri);
  
  if (jsv.has_field(kRESULTS) &&
      jsv[kRESULTS].is_array() &&
      jsv[kRESULTS].size() > 0 &&
      jsv[kRESULTS][0].has_field(kSERIES) &&
      jsv[kRESULTS][0][kSERIES].is_array() ) 
  {
    jsArray seriesArray = jsv["results"][0][kSERIES].as_array();
    for (auto seriesIt = seriesArray.begin(); seriesIt != seriesArray.end(); ++seriesIt) {
      
      string str = seriesIt->serialize();
      jsObject thisSeries = seriesIt->as_object();
      
      jsArray columns = thisSeries["columns"].as_array(); // the only column is "key"
      jsArray values = thisSeries["values"].as_array(); // array of single-string arrays
      
      for (auto valuesIt = values.begin(); valuesIt != values.end(); ++valuesIt) {
        jsArray singleStrArr = valuesIt->as_array();
        string dbId = singleStrArr[0].as_string();
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
        ids.set(properId,units);
      } // for each values array (ts definition)
    }
  }
  // else nothing
  _idCache = ids;
  return ids;
}

// READ
std::vector<Point> InfluxTcpAdapter::selectRange(const std::string& id, TimeRange range) {
  _RTX_DB_SCOPED_LOCK;
  string dbId = influxIdForTsId(id);
  InfluxTcpAdapter::Query q = this->queryPartsFromMetricId(dbId);
  q.where.push_back("time >= " + to_string(range.start) + "s");
  q.where.push_back("time <= " + to_string(range.end) + "s");
  
  uri uri = uriForQuery(q.selectStr());
  jsValue jsv = __jsonFromRequest(uri);
  return __pointsFromJson(jsv);
}

Point InfluxTcpAdapter::selectNext(const std::string& id, time_t time) {
  std::vector<Point> points;
  string dbId = influxIdForTsId(id);
  Query q = this->queryPartsFromMetricId(dbId);
  q.where.push_back("time > " + to_string(time) + "s");
  q.order = "time asc limit 1";
  
  uri uri = uriForQuery(q.selectStr());
  jsValue jsv = __jsonFromRequest(uri);
  points = __pointsFromJson(jsv);
  
  if (points.size() == 0) {
    return Point();
  }
  
  return points.front();
}

Point InfluxTcpAdapter::selectPrevious(const std::string& id, time_t time) {
  std::vector<Point> points;
  string dbId = influxIdForTsId(id);
  
  Query q = this->queryPartsFromMetricId(dbId);
  q.where.push_back("time < " + to_string(time) + "s");
  q.order = "time desc limit 1";
  
  uri uri = uriForQuery(q.selectStr());
  jsValue jsv = __jsonFromRequest(uri);
  points = __pointsFromJson(jsv);
  
  if (points.size() == 0) {
    return Point();
  }
  
  return points.front();
}

// DELETE
void InfluxTcpAdapter::removeRecord(const std::string& id) {
  const string dbId = this->influxIdForTsId(id);
  Query q = this->queryPartsFromMetricId(id);
  
  stringstream sqlss;
  sqlss << "DROP SERIES FROM " << q.nameAndWhereClause();
  
  uri uri = uriForQuery(sqlss.str());
  jsValue v = __jsonFromRequest(uri, methods::POST);
}

void InfluxTcpAdapter::removeAllRecords() {
  _errCallback("Truncating");
  
  auto ids = this->idUnitsList();
  
  stringstream sqlss;
  sqlss << "DROP DATABASE " << this->conn.db << "; CREATE DATABASE " << this->conn.db;
  
  uri uri = uriForQuery(sqlss.str());
  jsValue v = __jsonFromRequest(uri, methods::POST);
  
  this->beginTransaction();
  for (auto ts_units : *ids.get()) {
    this->insertIdentifierAndUnits(ts_units.first, ts_units.second);
  }
  this->endTransaction();
  
  _errCallback("OK");
  return;
}

size_t InfluxTcpAdapter::maxTransactionLines() {
  return 1000;
}

void InfluxTcpAdapter::sendPointsWithString(const std::string& content) {
  INFLUX_ASYNC_SEND.wait(); // wait on previous send if needed.
  
  const string bodyContent(content);
  INFLUX_ASYNC_SEND = pplx::create_task([&,bodyContent]() {
    web::uri_builder b;
    b.set_scheme(this->conn.proto).set_host(this->conn.host).set_port(this->conn.port).set_path("write")
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
      auto status = r.status_code();
      if (status == status_codes::NoContent) {
        // no content. this is fine.
      }
      else if (status != status_codes::OK) {
        DebugLog << "send points to influx: POST returned " << r.status_code() << " - " << r.reason_phrase() << EOL;
      }
    } catch (std::exception &e) {
      cerr << "exception POST: " << e.what() << endl;
    }
  });
  
  if (!_inTransaction) {
    INFLUX_ASYNC_SEND.wait();
    // block returning unless we are still in a bulk operation.
    // otherwise, carry on. no need to wait - let other processes continue.
  }

}


uri InfluxTcpAdapter::uriForQuery(const std::string& query, bool withTimePrecision) {
  
  web::http::uri_builder b;
  b.set_scheme(this->conn.proto).set_host(this->conn.host).set_port(this->conn.port).set_path("query")
  .append_query("db", this->conn.db, false)
  .append_query("u", this->conn.user, false)
  .append_query("p", this->conn.pass, false)
  .append_query("q", query, true);
  
  if (withTimePrecision) {
    b.append_query("epoch","s");
  }
  
  return b.to_uri();
}

jsValue __jsonFromRequest(uri uri, method withMethod) {
  jsValue js = jsValue::object();
  try {
    web::http::client::http_client_config config;
    config.set_timeout(std::chrono::seconds(RTX_INFLUX_CLIENT_TIMEOUT));
    web::http::client::http_client client(uri, config);
    http_response r = client.request(withMethod).get(); // waits for response
    if (r.status_code() == status_codes::OK) {
      js = r.extract_json().get();
    }
    else {
      cerr << "CONNECTION ERROR: " << r.reason_phrase() << EOL;
    }
  }
  catch (std::exception &e) {
    cerr << "exception in GET: " << e.what() << endl;
  }
  return js;
}

std::vector<RTX::Point> __pointsFromJson(jsValue json) {
  
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


InfluxTcpAdapter::Query InfluxTcpAdapter::queryPartsFromMetricId(const std::string &name) {
  MetricInfo m(name);
  
  Query q;
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

std::string InfluxTcpAdapter::formatTimestamp(time_t t) {
  return to_string(t);
}


/****************************************************************************************************/
/****************************************************************************************************/
/****************************************************************************************************/
/****************************************************************************************************/


InfluxUdpAdapter::InfluxUdpAdapter( errCallback_t cb ) : InfluxAdapter(cb) {
  _sendFuture = async(launch::async, [&](){return;});
}

InfluxUdpAdapter::~InfluxUdpAdapter() {
  
}

const DbAdapter::adapterOptions InfluxUdpAdapter::options() const {
  DbAdapter::adapterOptions o;
  
  o.supportsUnitsColumn = true;
  o.supportsSinglyBoundQuery = true;
  o.searchIteratively = false;
  o.canAssignUnits = false;
  o.implementationReadonly = false;
  
  return o;
}

std::string InfluxUdpAdapter::connectionString() {
  stringstream ss;
  ss << "host=" << this->conn.host << "&port=" << this->conn.port;
  return ss.str();
}

void InfluxUdpAdapter::doConnect() {
  _connected = false;
  boost::asio::io_service io_service;
  try {
    using boost::asio::ip::udp;
    boost::asio::io_service io_service;
    udp::resolver resolver(io_service);
    udp::resolver::query query(udp::v4(), this->conn.host, to_string(this->conn.port));
    udp::endpoint receiver_endpoint = *resolver.resolve(query);
    udp::socket socket(io_service);
    socket.open(udp::v4());
    socket.close();
  } catch (const std::exception &err) {
    DebugLog << "could not connect to UDP endpoint" << EOL << flush;
    _errCallback("Invalid UDP Endpoint");
    return;
  }
  _errCallback("Connected");
  _connected = true;
}

IdentifierUnitsList InfluxUdpAdapter::idUnitsList() {
  return IdentifierUnitsList();
}

// READ
std::vector<Point> InfluxUdpAdapter::selectRange(const std::string& id, TimeRange range) {
  return {Point()};
}

Point InfluxUdpAdapter::selectNext(const std::string& id, time_t time) {
  return Point();
}

Point InfluxUdpAdapter::selectPrevious(const std::string& id, time_t time) {
  return Point();
}


// DELETE
void InfluxUdpAdapter::removeRecord(const std::string& id) {
  return;
}

void InfluxUdpAdapter::removeAllRecords() {
  return;
}

size_t InfluxUdpAdapter::maxTransactionLines() {
  return 10;
}

void InfluxUdpAdapter::sendPointsWithString(const std::string& content) {
  if (_sendFuture.valid()) {
    _sendFuture.wait();
  }
  string body(content);
  _sendFuture = async(launch::async, [&,body]() {
    using boost::asio::ip::udp;
    boost::asio::io_service io_service;
    udp::resolver resolver(io_service);
    udp::resolver::query query(udp::v4(), this->conn.host, to_string(this->conn.port));
    udp::endpoint receiver_endpoint = *resolver.resolve(query);
    udp::socket socket(io_service);
    socket.open(udp::v4());
    boost::system::error_code err;
    socket.send_to(boost::asio::buffer(body, body.size()), receiver_endpoint, 0, err);
    if (err) {
      DebugLog << "UDP SEND ERROR: " << err.message() << EOL << flush;
    }
    socket.close();
  });

}

std::string InfluxUdpAdapter::formatTimestamp(time_t t) {
  // Line protocol requires unix-nano unles qualified by HTTP-GET fields,
  // which of course we don't have over UDP
  return to_string(t) + "000000000";
}






































