#include "InfluxDbPointRecord.h"

#include <boost/asio.hpp>
#include <boost/foreach.hpp>
#include <curl/curl.h>
#include <map>


#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <boost/timer/timer.hpp>

using namespace std;
using namespace RTX;
using boost::asio::ip::tcp;

#define HTTP_OK 200



InfluxDbPointRecord::InfluxDbPointRecord() {
  _connected = false;
  _range = make_pair(0,0);
}

void InfluxDbPointRecord::dbConnect() throw(RtxException) {
  
  stringstream q;
  q << "/db?u=" << this->user << "&p=" << this->pass;
  
  JsonDocPtr doc = this->documentFromUrl(q.str());
  if (!doc) {
    _connected = false;
    cerr << "could not connect" << endl;
  }
  else {
    _connected = true;
  }
  
}


string InfluxDbPointRecord::registerAndGetIdentifier(std::string recordName, Units dataUnits) {
  return recordName;
}


PointRecord::time_pair_t InfluxDbPointRecord::range(const string& id) {
  
  if (_range.second > 0) {
    return _range;
  }
  
  stringstream sqlss;
  string url;
  JsonDocPtr doc;
  vector<Point> firstPoints;
  vector<Point> lastPoints;
  
  {
    boost::timer::auto_cpu_timer t;
    cout << "first:" << endl;
    
    sqlss << "select time,value,quality from " << id << " order asc limit 1";
    url = this->urlForQuery(sqlss.str());
    doc = this->documentFromUrl(url);
    firstPoints = this->pointsFromDoc(doc);
    sqlss.str("");
  }
  
  
  {
    boost::timer::auto_cpu_timer t;
    cout << "last:" << endl;
    
    sqlss << "select time,value,quality from " << id << " where time < now() + 36500d order desc limit 1";
    url = this->urlForQuery(sqlss.str());
    doc = this->documentFromUrl(url);
    lastPoints = this->pointsFromDoc(doc);
    
  }
  
  if (firstPoints.size() == 0 || lastPoints.size() == 0) {
    cerr << "not enough points" << endl;
    return std::make_pair(0, 0);
  }
  
  Point first = firstPoints.front();
  Point last = lastPoints.back();
  
  PointRecord::time_pair_t range = std::make_pair(first.time, last.time);
  
  _range = range;
  return _range;
}





std::vector<Point> InfluxDbPointRecord::selectRange(const std::string& id, time_t startTime, time_t endTime) {
  std::vector<Point> points;
  
  stringstream sqlss;
  sqlss << "select time,value,quality from " << id;
  sqlss << " where time > " << startTime << "s";
  sqlss << " and time < "   << endTime << "s";
  sqlss << " order asc";
  string url = this->urlForQuery(sqlss.str());

  JsonDocPtr doc = this->documentFromUrl(url);
  return this->pointsFromDoc(doc);
}


Point InfluxDbPointRecord::selectNext(const std::string& id, time_t time) {
  
  std::vector<Point> points;
  
  stringstream sqlss;
  sqlss << "select time,value,quality from " << id;
  sqlss << " where time > " << time + 1 << "s";
  sqlss << " order asc limit 1";
  string url = this->urlForQuery(sqlss.str());
  
  JsonDocPtr doc = this->documentFromUrl(url);
  points = this->pointsFromDoc(doc);
  
  if (points.size() == 0) {
    return Point();
  }
  
  return points.front();
}


Point InfluxDbPointRecord::selectPrevious(const std::string& id, time_t time) {
  std::vector<Point> points;
  
  stringstream sqlss;
  sqlss << "select time,value,quality from " << id;
  sqlss << " where time < " << time - 1 << "s";
  sqlss << " order desc limit 1";
  string url = this->urlForQuery(sqlss.str());
  
  JsonDocPtr doc = this->documentFromUrl(url);
  points = this->pointsFromDoc(doc);
  
  if (points.size() == 0) {
    return Point();
  }
  
  return points.front();
}



void InfluxDbPointRecord::insertSingle(const std::string& id, Point point) {
  
  vector<Point> points;
  points.push_back(point);
  
  this->insertRange(id, points);
  
}

void InfluxDbPointRecord::insertRange(const std::string& id, std::vector<Point> points) {
  
  vector<Point> existing;
  existing = this->selectRange(id, points.front().time - 1, points.back().time + 1);
  map<time_t,bool> existingMap;
  BOOST_FOREACH(const Point& p, existing) {
    existingMap[p.time] = true;
  }
  
  vector<Point> insertionPoints;
  
  BOOST_FOREACH(const Point& p, points) {
    if (existingMap.find(p.time) == existingMap.end()) {
      insertionPoints.push_back(p);
    }
  }
  
  if (insertionPoints.size() == 0) {
    return;
  }
  
  JsonDocPtr doc = this->insertionDocumentFromPoints(id, insertionPoints);
  string body = this->serializedDocument(doc);
  
  this->postPointsWithBody(body);
  
  BOOST_FOREACH(const Point& p, insertionPoints) {
    if (p.time > _range.second) {
      _range.second = p.time;
    }
    if (p.time < _range.first || _range.first == 0) {
      _range.first = p.time;
    }
  }
  
}


void InfluxDbPointRecord::removeRecord(const std::string& id) {
  
  
  stringstream sqlss;
  sqlss << "drop series " << id;
  string url = this->urlForQuery(sqlss.str(),false);
  
  JsonDocPtr doc = this->documentFromUrl(url);
  
  
}




vector<string> InfluxDbPointRecord::identifiers() {
  vector<string> ids;
  string query = "/db/" + this->db + "/series?u=" + this->user + "&p=" + this->pass + "&q=list%20series";
  JsonDocPtr jsonDoc = this->documentFromUrl(query);
  
  if (jsonDoc) {
    for (rapidjson::SizeType i = 0; i < jsonDoc->Size(); ++i) {
      string id = (*jsonDoc)[i]["name"].GetString();
      ids.push_back(id);
    }
  }
  
  return ids;
}





vector<Point> InfluxDbPointRecord::pointsFromDoc(JsonDocPtr doc) {
  vector<Point> points;
  
  // multiple time series might be returned eventually, but for now it's just a single-value array.
  
  if (!doc->IsArray()) {
    cerr << "not an array: bad output" << endl;
    return points;
  }
  
  if (doc->Size() == 0) {
    return points; // no data
  }
  
  const rapidjson::SizeType zero = 0;
  const rapidjson::Value& tsData = (*doc)[zero];
  string tsName = tsData["name"].GetString();
  
  // create a little map so we know what order the columns are in
  map<string,int> columnMap;
  const rapidjson::Value& columns = tsData["columns"];
  for (rapidjson::SizeType i = 0; i < columns.Size(); ++i) {
    string colName = columns[i].GetString();
    columnMap[colName] = (int)i;
  }
  
  int timeIndex = columnMap["time"];
  int valueIndex = columnMap["value"];
  int qualityIndex = columnMap["quality"];
  
  // now go through each returned row and create a point.
  // use the column name map to set point properties.
  const rapidjson::Value& pointRows = tsData["points"];
  points.reserve((size_t)pointRows.Size());
  for (rapidjson::SizeType i = 0; i < pointRows.Size(); ++i) {
    const rapidjson::Value& row = pointRows[i];
    time_t pointTime = (time_t)row[timeIndex].GetInt();
    double pointValue = row[valueIndex].GetDouble();
    Point::Qual_t pointQuality = (row[qualityIndex].IsNull()) ? Point::good : (Point::Qual_t)row[qualityIndex].GetInt();
    
    Point p(pointTime, pointValue, pointQuality);
    points.push_back(p);
  }
  
  
  
  return points;
}





JsonDocPtr InfluxDbPointRecord::documentFromUrl(const std::string &url) {
  JsonDocPtr documentOut;
  stringstream portss;
  portss << this->port;
  
  InfluxConnectInfo_t connectionInfo;
  
  connectionInfo.sockStream.connect(this->host, portss.str());
  if (!connectionInfo.sockStream) {
    cerr << "cannot connect" << endl;
    return documentOut;
  }
  
  string body;
  
  {
//    boost::timer::auto_cpu_timer t;
    connectionInfo.sockStream << "GET " << url << " HTTP/1.0\r\n";
    connectionInfo.sockStream << "Host: " << this->host << "\r\n";
    connectionInfo.sockStream << "Accept: */*\r\n";
    connectionInfo.sockStream << "Connection: close\r\n\r\n";
    connectionInfo.sockStream >> connectionInfo.httpVersion;
    connectionInfo.sockStream >> connectionInfo.statusCode;
    connectionInfo.sockStream >> connectionInfo.statusMessage;
    
    string headerStr;
    connectionInfo.sockStream >> headerStr;
    while (std::getline(connectionInfo.sockStream, headerStr) && headerStr != "\r") {
      // nothing
    }
    
    
    std::getline(connectionInfo.sockStream,body);
    //  cout << connectionInfo.sockStream.rdbuf() << endl;
    connectionInfo.sockStream.flush();
    connectionInfo.sockStream.close();
    
  }
  
  
  
  documentOut.reset(new rapidjson::Document);
  documentOut.get()->Parse<0>(body.c_str());
  
  return documentOut;
}


const std::string InfluxDbPointRecord::urlEncode(std::string s) {
  
  std::string encStr("");
  
  CURL *curl;
  curl = curl_easy_init();
  if (curl) {
    char *enc;
    enc = curl_easy_escape(curl, s.c_str(), 0);
    encStr = string(enc);
    curl_easy_cleanup(curl);
  }
  
//  cout << s << endl;
  return encStr;
}



const string InfluxDbPointRecord::serializedDocument(JsonDocPtr doc) {
  
  rapidjson::StringBuffer strbuf;
  rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
  doc->Accept(writer);
  
  string serialized(strbuf.GetString());
  return serialized;
}

const string InfluxDbPointRecord::urlForQuery(const std::string& query, bool appendTimePrecision) {
  stringstream queryss;
  queryss << "/db/" << this->db;
  queryss << "/series?";
  queryss << "u=" << this->user;
  queryss << "&p=" << this->pass;
  queryss << "&q=" << this->urlEncode(query);
  if (appendTimePrecision) {
    queryss << "&time_precision=s";
  }
  
  return queryss.str();
}

JsonDocPtr InfluxDbPointRecord::insertionDocumentFromPoints(const std::string& tsName, std::vector<Point> points) {
  
  JsonDocPtr doc( new rapidjson::Document );
  rapidjson::Document::AllocatorType& allocator = doc->GetAllocator();
  
  doc->SetArray();
  
  rapidjson::Value jsName(tsName.c_str());
  rapidjson::Value jsCols(rapidjson::kArrayType);
  jsCols.PushBack("time", allocator);
  jsCols.PushBack("value", allocator);
  jsCols.PushBack("quality", allocator);
  
  rapidjson::Value jsPoints(rapidjson::kArrayType);
  BOOST_FOREACH(const Point& p, points) {
    rapidjson::Value jsPoint(rapidjson::kArrayType);
    jsPoint.PushBack((int)p.time,     allocator);
    jsPoint.PushBack((double)p.value, allocator);
    jsPoint.PushBack((int)p.quality,  allocator);
    // add it to the points collection
    jsPoints.PushBack(jsPoint, allocator);
  }
  
  // put it all together.
  rapidjson::Value tsData(rapidjson::kObjectType);
  tsData.AddMember("name", tsName.c_str(), allocator);
  tsData.AddMember("columns", jsCols, allocator);
  tsData.AddMember("points", jsPoints, allocator);
  
  
  doc->PushBack(tsData, allocator);
  
  return doc;
  
  /*
   
   [
     {
       "name": "response_times",
       "columns": ["time", "value"],
       "points": [
         [1382819388, 234.3],
         [1382819389, 120.1],
         [1382819380, 340.9]
       ]
     }
   ]
   
   
   */
  
}




void InfluxDbPointRecord::postPointsWithBody(const std::string& body) {
  
  stringstream queryss;
  queryss << "/db/" << this->db;
  queryss << "/series?";
  queryss << "u=" << this->user;
  queryss << "&p=" << this->pass;
  queryss << "&time_precision=s";
  string url(queryss.str());
  
  
  stringstream portss;
  portss << this->port;
  
  InfluxConnectInfo_t connectionInfo;
  
  connectionInfo.sockStream.connect(this->host, portss.str());
  if (!connectionInfo.sockStream) {
    cerr << "cannot connect" << endl;
    return;
  }
  
  connectionInfo.sockStream << "POST " << url << " HTTP/1.0\r\n";
  connectionInfo.sockStream << "Host: " << this->host << "\r\n";
  connectionInfo.sockStream << "Accept: */*\r\n";
  connectionInfo.sockStream << "Content-Type: application/json; charset=utf-8\r\n";
  connectionInfo.sockStream << "Content-Length: " << body.size() << "\r\n";
  connectionInfo.sockStream << "Connection: close\r\n\r\n";
  
  connectionInfo.sockStream << body;
  connectionInfo.sockStream.flush();
//  cout << connectionInfo.sockStream.rdbuf();
  connectionInfo.sockStream.close();
  
}







