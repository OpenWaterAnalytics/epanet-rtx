//
//  Point.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#ifdef _WIN32
  #define isnan(x) (x!=x)
#endif

#include "Point.h"
#include <math.h>
#include <iostream>

using namespace std;
using namespace RTX;

Point::Point() : time(0),value(0),quality(Point::missing),isValid(false),confidence(0) {
  
}


Point::Point(time_t t, double v, Qual_t q, double c) : time(t),value(v),quality(q),isValid((q == missing)||(isnan(v)) ? false : true),confidence(c) {
  if (isnan(v)) {
    cout << "nan" << endl;
  }
}

Point::~Point() {
  
}


#pragma mark - Operators

Point Point::operator+(const Point& point) const {
  Point result = *this;
  result += point;
  return result;
}

Point& Point::operator+=(const Point& point) {
  if (point.time != this->time) {
    std::cerr << "point times do not match: " << (point.time - this->time) << "s gap." << std::endl;
  }
  double value = this->value + point.value;
  double confidence = (this->confidence + point.confidence) / 2.;
  
  this->value = value;
  this->confidence = confidence;
  
  return *this;
}

Point Point::operator*(const double factor) const {
  double value;
  double confidence;
  time_t time;
  
  value = (this->value * factor);
  confidence = (this->confidence * factor);  // TODO -- figure out confidence intervals.
  time = this->time;
  
  return Point(time, value, Point::good, confidence);
}

Point& Point::operator*=(const double factor) {
  this->value *= factor;
  this->confidence *= factor;
  return *this;
}


Point Point::operator/(const double factor) const {
  double value;
  double confidence;
  time_t time;
  
  value = (this->value / factor);
  confidence = (this->confidence / factor);  // TODO -- figure out confidence intervals.
  time = this->time;
  
  return Point(time, value, Point::good, confidence);
}


bool Point::comparePointTime(const Point& left, const Point& right) {
  return left.time < right.time;
}

Point Point::linearInterpolate(const Point& p1, const Point& p2, const time_t& t) {
  Point p;
  if (p1.time == t) {
    p = p1;
  }
  else if (p2.time == t) {
    p = p2;
  }
  else {
    time_t dt = p2.time - p1.time;
    double dv = p2.value - p1.value;
    time_t dt2 = t - p1.time;
    double dv2 = dv * dt2 / dt;
    double newValue = p1.value + dv2;
    double newConfidence = (p1.confidence + p2.confidence) / 2; // TODO -- more elegant confidence estimation
    p = Point(t, newValue, Point::good, newConfidence);
  }
  return p;
}


/*
std::ostream& RTX::operator<< (std::ostream &out, Point &point) {
  return point.toStream(out);
}
*/

std::ostream& Point::toStream(std::ostream &stream) {
  stream << "(" << time << "," << value << "," << quality << "," << confidence << ")";
  return stream;
}



#pragma mark - Class Methods

Point Point::convertPoint(const Point& point, const Units& fromUnits, const Units& toUnits) {
  double value = Units::convertValue(point.value, fromUnits, toUnits);
  double confidence = Units::convertValue(point.confidence, fromUnits, toUnits);
  return Point(point.time, value, point.quality, confidence);
}



