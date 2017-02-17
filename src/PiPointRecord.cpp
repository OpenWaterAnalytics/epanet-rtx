#include "PiPointRecord.h"

#include <regex>
#include <boost/lexical_cast.hpp>

#include <cpprest/uri.h>
#include <cpprest/json.h>
#include <cpprest/http_client.h>

#include "PointRecordTime.h"

using namespace RTX;
using namespace std;

using namespace web::http::client;
using web::http::method;
using web::http::methods;
using web::http::http_request;
using web::http::status_codes;
using web::http::http_response;
using web::http::uri;

using jsv = web::json::value;


#define PI_MAX_POINT_COUNT 10000

const string kOSI_REST("OSIsoft.REST");
const string kFULL_VERSION("FullVersion");
const string kWebId("WebId");
const string kItems("Items");
const string kName("Name");
const string kDescriptor("Descriptor");
const string kEngUnits("EngineeringUnits");
const string kTimeStamp("Timestamp");
const string kValue("Value");
const string kGood("Good");
const string kQuest("Questionable");
const string kSubst("Substituted");


const char* t_fmt = "%Y-%m-%dT%H:%M:%SZ";


Point _pointFromJson(const jsv& j);
Point _pointFromJson(const jsv& j) {
  for (const string& key : {kTimeStamp,kValue,kGood,kQuest,kSubst}) {
    if (!j.has_field(key)) {
      cerr << "ERR: JSON object does not contain required field " << key << endl;
      return Point();
    }
  }
  
  // check value type?
  if (!j.at(kValue).is_double()) {
    cerr << "Value field is not double" << endl;
    return Point();
  }
  
  const string tstamp(j.at(kTimeStamp).as_string());
  const double v(j.at(kValue).as_double());
  const bool good(j.at(kGood).as_bool());
  Point::PointQuality q = (good ? Point::PointQuality::opc_good : Point::PointQuality::opc_bad);
  time_t t = PointRecordTime::timeFromIso8601(tstamp);
  
  return Point(t,v,q);
}







PiPointRecord::PiPointRecord() {
  conn.proto = "http";
  conn.host = "devdata.osisoft.com";
  conn.port = 80;
  conn.apiPath = "piwebapi";
  conn.dataServer = "PISRV1";
  conn.user = "user";
  conn.pass = "pass";
  
  _isConnected = false;
  
}

PiPointRecord::~PiPointRecord() {
  
}

bool PiPointRecord::isConnected() {
  return _isConnected;
}

string PiPointRecord::connectionString() {
  
  stringstream ss;
  ss << "proto=" << this->conn.proto << "&host=" << this->conn.host << "&port=" << this->conn.port << "&api=" << this->conn.apiPath << "&dataserver=" << this->conn.dataServer << "&u=" << this->conn.user << "&p=" << this->conn.pass;
  return ss.str();
  
}


void PiPointRecord::setConnectionString(const std::string &str) {
  std::lock_guard<std::mutex> lock(_mtx);
  
  // split the tokenized string. we're expecting something like "host=127.0.0.1&port=4242"
  regex kvReg("([^=]+)=([^&]+)&?"); // key - value pair
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
    {"api", [&](string v){this->conn.apiPath = v;}},
    {"dataserver", [&](string v){this->conn.dataServer = v;}},
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


web::uri_builder PiPointRecord::uriBase() {
  web::http::uri_builder b;
  b.set_scheme(conn.proto).set_host(conn.host).set_port(conn.port)
  .append_path(conn.apiPath);
  return b;
}

void PiPointRecord::dbConnect() throw(RtxException) {
  
  _isConnected = false;
  conn.dsWebId = "";
  
  // try the endpoint
  auto uriQ = uriBase()
  .append_path("system")
  .append_path("versions")
  .to_uri();
  
  jsv j = jsonFromRequest(uriQ, methods::GET);
  if (j.has_field(kOSI_REST)) {
    auto v = j[kOSI_REST];
    if (v.has_field(kFULL_VERSION)) {
      string vers = v[kFULL_VERSION].as_string();
      cout << "PI REST VERSION: " << vers << endl;
    }
  }
  else {
    cerr << "could not retrieve version info from PI endpoint" << endl;
    _isConnected = false;
    errorMessage = "Malformed Response";
    return;
  }
  
  
  // ok, now try to get the webId for the data server
  auto uriDS = uriBase()
  .append_path("dataservers")
  .append_query("name", conn.dataServer)
  .append_query("selectedFields", "WebId;Name;IsConnected;ServerVersion")
  .to_uri();
  
  jsv dj = jsonFromRequest(uriDS, methods::GET);
  if (dj.has_field(kWebId)) {
    string webId = dj[kWebId].as_string();
    cout << "PI DATA SERVER WEB ID: " << webId << endl;
    conn.dsWebId = webId;
    _isConnected = true;
    errorMessage = "Connected";
  }
  else {
    cerr << "could not retrieve dataserver WebId from PI endpoint" << endl;
    _isConnected = false;
    errorMessage = "No Data Server";
    return;
  }
  
  
}




PointRecord::IdentifierUnitsList PiPointRecord::identifiersAndUnits() {
  std::lock_guard<std::mutex> lock(_mtx);
  
  _identifiersAndUnitsCache.clear();
  _webIdLookup.clear();
  
  auto uriPointsB = uriBase()
  .append_path("dataservers")
  .append_path(conn.dsWebId)
  .append_path("points")
  .append_query("selectedFields=Items.WebId;Items.Name;Items.Descriptor;Items.EngineeringUnits");
  
  if (this->tagSearchPath != "") {
    uriPointsB.append_query("nameFilter",this->tagSearchPath);
  }
  jsv j = jsonFromRequest(uriPointsB.to_uri(), methods::GET);
  
  if (j.has_field(kItems)) {
    for(auto i : j[kItems].as_array()) {
      if (i.has_field(kName) && i.has_field(kWebId)) {
        
        string name = i[kName].as_string();
        string webId = i[kWebId].as_string();
        
        _webIdLookup[name] = webId;
        _identifiersAndUnitsCache.set(name, RTX_DIMENSIONLESS);
        
      }
    }
  }
  
  return _identifiersAndUnitsCache;
}




vector<Point> PiPointRecord::selectRange(const string& id, TimeRange range) {
  vector<Point> points;
  
  if (_webIdLookup.count(id) == 0) {
    cerr << "PI RECORD ERROR: id " << id << " not in cache" << endl;
    return points;
  }
  
  auto startStr = PointRecordTime::utcDateStringFromUnix(range.start,t_fmt);
  auto endStr = PointRecordTime::utcDateStringFromUnix(range.end,t_fmt);
  string webId = _webIdLookup[id];
  
  auto uriRange = uriBase()
  .append_path("streams")
  .append_path(webId)
  .append_path("recorded")
  .append_query("startTime",startStr)
  .append_query("endTime",endStr)
  .append_query("maxCount",PI_MAX_POINT_COUNT)
  .to_uri();
  
  jsv j = jsonFromRequest(uriRange, methods::GET);
  
  if (!j.has_field(kItems)) {
    cerr << "PI RECORD COULD NOT FIND ITEMS in response: " << j.serialize() << endl;
    return points;
  }
  
  for (auto pjs : j[kItems].as_array()) {
    Point p = _pointFromJson(pjs);
    if (p.isValid) {
      points.push_back(p);
    }
  }
  
  return points;
}


jsv PiPointRecord::jsonFromRequest(uri uri, method withMethod) {
  jsv js = jsv::object();
  try {
    string auth = conn.user + ":" + conn.pass;
    std::vector<unsigned char> bytes(auth.begin(), auth.end());
    string userpass = utility::conversions::to_base64(bytes);
    
    http_client_config config;
    config.set_timeout(std::chrono::seconds(3));
    //    credentials cred(conn.user,conn.pass);
    //    config.set_credentials(cred);
    http_client client(uri, config);
    http_request req(methods::GET);
    req.headers().add("Authorization", "Basic " + userpass);
    http_response r = client.request(req).get(); // waits for response
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






