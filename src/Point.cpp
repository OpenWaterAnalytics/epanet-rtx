//
//  Point.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#include "Point.h"
#include <cmath>
#include <iostream>

using std::cout;
using std::endl;
using namespace RTX;

Point::Point() : time(0),value(0),quality(PointQuality::opc_bad),confidence(0),isValid(false) {
  
}

Point::Point(time_t t, double v, PointQuality q, double c) : time(t),value(v),quality(q),confidence(c),isValid((std::isnan(v)) ? false : true) {
  if (std::isnan(v)) {
    cout << "nan" << endl;
  }
  if (time == 0) {
    isValid = false;
    quality = (PointQuality::opc_bad);
  }
}

Point::~Point() {
  
}

const bool Point::hasQual(PointQuality qual) const {
  return (this->quality & qual) || (this->quality == qual);
}

void Point::addQualFlag(PointQuality qual) {
  // make sure it's an RTX override quality. if we're using this method, then it's not process data.
  // funny bitwise math here since the OPC standard is not a straightforward bitmask
  PointQuality override = this->quality;
  override = (PointQuality)(override & (~opc_good)); // remove opc bits
  override = (PointQuality)(override | opc_rtx_override); // add override mask
  override = (PointQuality)(override | qual);
  this->quality = override;
  if (this->hasQual(opc_bad)) {
    this->isValid = false;
  }
};


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

Point Point::operator+(const double value) const {
  Point result = *this;
  result += value;
  return result;
}

Point& Point::operator+=(const double value) {
  this->value += value;
  return *this;
}


Point Point::operator*(const double factor) const {
  double value;
  double confidence;
  time_t time;
  
  value = (this->value * factor);
  confidence = (this->confidence * factor);  // TODO -- figure out confidence intervals.
  time = this->time;
  
  return Point(time, value, opc_rtx_override, confidence);
}

Point& Point::operator*=(const double factor) {
  this->value *= factor;
  this->confidence *= factor;
  return *this;
}

Point Point::operator*(const Point p) const {
  if (this->time != p.time) {
    return Point();
  }
    
  double value = (this->value * p.value);
  double confidence = (this->confidence * p.confidence);  // TODO -- figure out confidence intervals.
  time_t time = this->time;
  PointQuality q = (PointQuality)(this->quality | p.quality);
  
  return Point(time, value, q, confidence);
}

Point Point::operator/(const double factor) const {
  double value = (this->value / factor);
  double confidence = factor == 0 ? 0 : (this->confidence / factor);  // TODO -- figure out confidence intervals.
  time_t time = this->time;
  
  return Point(time, value, opc_rtx_override, confidence);
}

Point Point::operator/(const Point p) const {
  if (this->time != p.time) {
    return Point();
  }
  
  double value = (this->value / p.value);
  double confidence = p.confidence == 0 ? 0 : (this->confidence / p.confidence);  // TODO -- figure out confidence intervals.
  time_t time = this->time;
  PointQuality q = (PointQuality)(this->quality | p.quality);
  
  return Point(time, value, q, confidence);
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
    p = Point(t, newValue, (PointQuality)(p1.quality | p2.quality), newConfidence);
    p.addQualFlag(rtx_interpolated);
  }
  return p;
}


Point Point::inverse() const {
  double value;
  double confidence;
  time_t time;
  PointQuality q = this->quality;
  
  
  if (this->value != 0.) {
    value = (1 / this->value);
  }
  else {
    // nan
    return Point();
  }
  
  if (this->confidence != 0.) {
    confidence = (1 / this->confidence);  // TODO -- figure out confidence intervals.
  }
  else {
    confidence = 0;
  }
  
  time = this->time;
  
  return Point(time, value, q, confidence);
}


Point Point::converted(const Units& fromUnits, const Units& toUnits) const {
  double value = Units::convertValue(this->value, fromUnits, toUnits);
  double confidence = Units::convertValue(this->confidence, fromUnits, toUnits);
  return Point(this->time, value, this->quality, confidence);
}

#pragma mark - Class Methods

Point Point::convertPoint(const Point& point, const Units& fromUnits, const Units& toUnits) {
  return point.converted(fromUnits, toUnits);
}



