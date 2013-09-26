//
//  Point.h
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#ifndef epanet_rtx_point_h
#define epanet_rtx_point_h

#include <time.h>
#include <map>
#include "rtxMacros.h"
#include "Units.h"

namespace RTX {
    
//!   A Point Class to store data tuples (date, value, quality, confidence)
/*!
      The point class keeps track of a piece of measurement data; time, value, and quality.
*/
  class Point {    
  public:
    //! quality flag
    enum Qual_t { good, questionable, missing, estimated, forecasted, bad };
    
    //! Empty Constructor, equivalent to Point(0,0,Point::missing,0)
    Point();
    //! Full Constructor, for explicitly setting all internal data within the point object.
    Point(time_t time, double value, Qual_t qual = good, double confidence = 0.);
    // dtor
    ~Point();
    
    // operators
    Point operator+(const Point& point) const;
    Point& operator+=(const Point& point);
    Point operator*(const double factor) const;
    Point& operator*=(const double factor);
    Point operator/(const double factor) const;
    virtual std::ostream& toStream(std::ostream& stream);

    // simple tuple class, so no getters/setters
    time_t time;
    double value;
    Qual_t quality;
    double confidence;
    bool isValid;
    
    // static class methods
    static Point convertPoint(const Point& point, const Units& fromUnits, const Units& toUnits);
    static bool comparePointTime(const Point& left, const Point& right);
    static Point linearInterpolate(const Point& p1, const Point& p2, const time_t& t);
    
    friend std::ostream& operator<<(std::ostream& outputStream, const Point& p)
    {
      return outputStream << p.value;
    }
    
    
  };

  //std::ostream& operator<< (std::ostream &out, Point &point);

}

#endif