//
//  TempoDbPointRecord.cpp
//  epanet-rtx
//
//  Created by Sam Hatchett on 5/8/14.
//
//

#include "TempoDbPointRecord.h"

using namespace RTX;
using namespace std;


void TempoDbPointRecord::dbConnect() throw(RtxException) {
  
}

string TempoDbPointRecord::registerAndGetIdentifier(std::string recordName) {
  
}

vector<string> TempoDbPointRecord::identifiers() {
  
}

PointRecord::time_pair_t TempoDbPointRecord::range(const string& id) {
  
}


std::vector<Point> TempoDbPointRecord::selectRange(const std::string& id, time_t startTime, time_t endTime) {
  
}

Point TempoDbPointRecord::selectNext(const std::string& id, time_t time) {
  
}

Point TempoDbPointRecord::selectPrevious(const std::string& id, time_t time) {
  
}


void TempoDbPointRecord::insertSingle(const std::string& id, Point point) {
  
}

void TempoDbPointRecord::insertRange(const std::string& id, std::vector<Point> points) {
  
}

void TempoDbPointRecord::removeRecord(const std::string& id) {
  
}


vector<Point> TempoDbPointRecord::pointsFromUrl(const std::string &url) {
  
  vector<Point> points;
  
  JsonDocPtr documentOut;
  stringstream portss;
  portss << this->port;
  
  boost::asio::ip::tcp::iostream sockStream;
  std::string httpVersion, statusMessage;
  unsigned int statusCode;
  
  
  sockStream.connect(this->host, portss.str());
  if (!sockStream) {
    cerr << "cannot connect" << endl;
    return points;
  }
  
  string userPassCombo = this->user + ":" + this->pass;
  string base64str = this->encode64(userPassCombo);
  
  string body;
  
  {
    //    boost::timer::auto_cpu_timer t;
    sockStream << "GET " << this->baseUrl << url << " HTTP/1.0\r\n";
    sockStream << "Host: " << this->host << "\r\n";
    sockStream << "Accept: */*\r\n";
    sockStream << "Authorization: Basic " << base64str << "\r\n";
    sockStream << "Connection: close\r\n\r\n";
    sockStream >> httpVersion;
    sockStream >> statusCode;
    sockStream >> statusMessage;
    
    string headerStr;
    sockStream >> headerStr;
    while (std::getline(sockStream, headerStr) && headerStr != "\r") {
      // nothing
    }
    
    
    std::getline(sockStream,body);
    //  cout << sockStream.rdbuf() << endl;
    sockStream.flush();
    sockStream.close();
    
  }
  
  
  
  documentOut.reset(new rapidjson::Document);
  documentOut.get()->Parse<0>(body.c_str());
  
  
  
  return points;
}


string TempoDbPointRecord::encode64(const std::string &str) {
  
  
  
}
