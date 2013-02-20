//
//  PointRecord.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#include <iostream>

#include "PointRecord.h"
#include <boost/foreach.hpp>
#include <boost/range/adaptors.hpp>

using namespace RTX;
using namespace std;


PointRecord::PointRecord() {
  _connectionString = "";
}

std::ostream& RTX::operator<< (std::ostream &out, PointRecord &pr) {
  return pr.toStream(out);
}

std::ostream& PointRecord::toStream(std::ostream &stream) {
  stream << "Point Record - connection: " << this->connectionString() << std::endl;
  return stream;
}

void PointRecord::setConnectionString(const std::string& connection) {
  _connectionString = connection;
}
const std::string& PointRecord::connectionString() {
  return _connectionString;
}


std::string PointRecord::registerAndGetIdentifier(std::string recordName) {
  
  return recordName;
}

std::vector<std::string> PointRecord::identifiers() {
  
  vector<string> names;
  
  return names;
}


void PointRecord::preFetchRange(const string& identifier, time_t start, time_t end) {
  
}



bool PointRecord::isPointAvailable(const string& identifier, time_t time) {
  
  return false;
}


Point PointRecord::point(const string& identifier, time_t time) {
  return Point();
}


Point PointRecord::pointBefore(const string& identifier, time_t time) {
  return Point();
}


Point PointRecord::pointAfter(const string& identifier, time_t time) {
  return Point();
}


std::vector<Point> PointRecord::pointsInRange(const string& identifier, time_t startTime, time_t endTime) {
  std::vector<Point> pointVector;
  return pointVector;
}

Point PointRecord::firstPoint(const string &id) {
  return Point();
}

Point PointRecord::lastPoint(const string &id) {
  return Point();
}


void PointRecord::addPoint(const string& identifier, Point point) {
  
}


void PointRecord::addPoints(const string& identifier, std::vector<Point> points) {
  
}


void PointRecord::reset() {
  
}

void PointRecord::reset(const string& identifier) {
  
}


