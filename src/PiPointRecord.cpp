#include "PiPointRecord.h"

#include <regex>
#include <boost/lexical_cast.hpp>

#include <cpprest/uri.h>
#include <cpprest/json.h>
#include <cpprest/http_client.h>


using namespace RTX;
using namespace std;

using namespace web::http::client;
using web::http::method;
using web::http::methods;
using web::http::status_codes;
using web::http::http_response;
using web::http::uri;

using jsv = web::json::value;


static string kOSI_REST("OSIsoft.REST");
static string kFULL_VERSION("FullVersion");
static string kWebId("WebId");




jsv PiPointRecord::jsonFromRequest(uri uri, method withMethod) {
  jsv js = jsv::object();
  try {
    
    credentials cred(conn.user,conn.pass);
    http_client_config config;
    config.set_timeout(std::chrono::seconds(3));
    config.set_credentials(cred);
    http_client client(uri, config);
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

string PiPointRecord::connectionString() {
  
  stringstream ss;
  ss << "proto=" << this->conn.proto << "&host=" << this->conn.host << "&port=" << this->conn.port << "&api=" << this->conn.apiPath << "&dataServer=" << this->conn.dataServer << "&u=" << this->conn.user << "&p=" << this->conn.pass;
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
    {"dataServer", [&](string v){this->conn.dataServer = v;}},
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


uri_builder PiPointRecord::uriBase() {
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
  
  jsv j = jsonFromRequest(uriQ, GET);
  if (j.has_field(kOSI_REST)) {
    auto v = j[kOSI_REST];
    if (v.has_field(kFULL_VERSION)) {
      string vers = v[kFULL_VERSION].as_string();
      cout << "PI REST VERSION: " << vers << endl;
    }
  }
  
  
  // ok, now try to get the webId for the data server
  auto uriDS = uriBase()
  .append_path("dataservers")
  .append_query("name", conn.dataServer)
  .append_query("selectedFields", "WebId;Name;IsConnected;ServerVersion")
  .to_uri();
  
  jsv dj = jsonFromRequest(uriDS, GET);
  if (dj.has_field(kWebId)) {
    string webId = dj[kWebId].as_string();
    cout << "PI DATA SERVER WEB ID: " << webId << endl;
    conn.dsWebId = webId;
    _isConnected = true;
  }
  else {
    _isConnected = false;
    return;
  }
  
  
  
  
  
  
  
  
  
  
}


















