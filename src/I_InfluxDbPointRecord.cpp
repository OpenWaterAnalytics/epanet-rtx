#include "I_InfluxDbPointRecord.h"
#include "MetricInfo.h"

#include <regex>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/replace.hpp>

#include <cpprest/uri.h>
#include <cpprest/json.h>
#include <cpprest/http_client.h>


using namespace std;
using namespace RTX;

using web::http::uri;
using jsValue = web::json::value;


I_InfluxDbPointRecord::connectionInfo::connectionInfo() {
  proto = "HTTP";
  host = "HOST";
  user = "USER";
  pass = "PASS";
  db = "DB";
  port = 8086;
}


std::string I_InfluxDbPointRecord::Query::selectStr() {
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

std::string I_InfluxDbPointRecord::Query::nameAndWhereClause() {
  stringstream ss;
  ss << this->from;
  
  if (this->where.size() > 0) {
    ss << " WHERE " << boost::algorithm::join(this->where," AND ");
  }
  return ss.str();
}






I_InfluxDbPointRecord::I_InfluxDbPointRecord() {
  _connected = false;
  _dbOptions = DbOptions(true, false, false, true);
}


string I_InfluxDbPointRecord::serializeConnectionString() {
  stringstream ss;
  ss << "proto=" << this->conn.proto << "&host=" << this->conn.host << "&port=" << this->conn.port << "&db=" << this->conn.db << "&u=" << this->conn.user << "&p=" << this->conn.pass;
  return ss.str();
}

void I_InfluxDbPointRecord::parseConnectionString(const std::string &str) {
  std::lock_guard<std::mutex> lock(_influxMutex);
  
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



bool I_InfluxDbPointRecord::insertIdentifierAndUnits(const std::string &id, RTX::Units units) {
  
  MetricInfo m(id);
  m.tags.erase("units"); // get rid of units if they are included.
  string properId = m.name();
  
  {
    std::lock_guard<std::mutex> lock(_influxMutex);
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
  string tsNameEscaped = influxIdForTsId(id);
  boost::replace_all(tsNameEscaped, " ", "\\ ");
  const string content(tsNameEscaped + " exist=true");
  if (_inBulkOperation) {
    std::lock_guard<std::mutex> lock(_influxMutex);
    _transactionLines.push_back(content);
  }
  else {
    this->sendPointsWithString(content);
  }
  // no futher validation.
  return true;
}





#pragma mark INSERT

void I_InfluxDbPointRecord::insertSingle(const std::string& id, Point point) {
  this->insertRange(id, {point});
}

void I_InfluxDbPointRecord::insertRange(const std::string& id, std::vector<Point> points) {
  if (points.size() == 0) {
    return;
  }
  
  vector<Point> insertionPoints;
  string dbId = influxIdForTsId(id);
  
  // is there anything here?
  /*
   string q = "select count(value) from " + dbId;
   http::uri uri = _InfluxDbPointRecord_uriForQuery(*this, q, false);
   jsValue jsv = _InfluxDbPointRecord_jsonFromGet(uri);
   */
  
  vector<Point> existing;
  //existing = this->selectRange(dbId, points.front().time - 1, points.back().time + 1);
  // NOPE - faster just to send
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
  
  auto content = this->insertionLinesFromPoints(dbId, insertionPoints);
  
  if (_inBulkOperation) {
    size_t nLines = 0;
    { // mutex
      std::lock_guard<std::mutex> lock(_influxMutex);
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
    //this->sendPointsWithString(content);
  }
  
}

void I_InfluxDbPointRecord::sendInfluxString(time_t time, const string& seriesId, const string& values) {
  
  string tsNameEscaped = seriesId;
  boost::replace_all(tsNameEscaped, " ", "\\ ");
  
  stringstream ss;
  string timeStr = this->formatTimestamp(time);
  
  ss << tsNameEscaped << " " << values << " " << timeStr;
  string data = ss.str();
  
  
  if (_inBulkOperation) {
    size_t nLines = 0;
    {
      std::lock_guard<std::mutex> lock(_influxMutex);
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


string I_InfluxDbPointRecord::influxIdForTsId(const string& id) {
  // sort named keys into proper order...
  MetricInfo m(id);
  if (m.tags.count("units")) {
    m.tags.erase("units");
  }
  string tsId = m.name();
  if (_identifiersAndUnitsCache.get()->count(tsId) == 0) {
    cerr << "no registered ts with that id: " << tsId << endl;
    // yet i'm being asked for it??
    return "";
  }
  Units u = (*_identifiersAndUnitsCache.get())[tsId];
  m.tags["units"] = u.to_string();
  return m.name();
}



vector<string> I_InfluxDbPointRecord::insertionLinesFromPoints(const string& tsName, vector<Point> points) {
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


#pragma mark TRANSACTION / BULK OPERATIONS

void I_InfluxDbPointRecord::beginBulkOperation() {
  if (_inBulkOperation) {
    return;
  }
  
  _inBulkOperation = true;
  {
    std::lock_guard<std::mutex> lock(_influxMutex);
    _transactionLines.clear();
  }
}

void I_InfluxDbPointRecord::endBulkOperation(){
  if (!_inBulkOperation) {
    return;
  }
  this->commitTransactionLines();
  _inBulkOperation = false;
}

void I_InfluxDbPointRecord::commitTransactionLines() {
  {
    std::lock_guard<std::mutex> lock(_influxMutex);
    if (_transactionLines.size() == 0) {
      return;
    }
  }
  {
    string concatLines;
    std::lock_guard<std::mutex> lock(_influxMutex);
    auto curs = _transactionLines.begin();
    auto end = _transactionLines.end();
    size_t iLine = 0;
    while (curs != end) {
      const string str = *curs;
      ++iLine;
      concatLines.append(str);
      concatLines.append("\n");
      // check for max lines (must respect max lines per send event)
      if (iLine >= maxTransactionLines()) {
        // push these lines and clear the queue
        this->sendPointsWithString(concatLines);
        concatLines.clear();
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




















