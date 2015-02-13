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


#define OPC_GOOD 192
#define OPC_BAD  0

namespace RTX {
    
//!   A Point Class to store data tuples (date, value, quality, confidence)
/*!
      The point class keeps track of a piece of measurement data; time, value, and quality.
*/
  class Point {    
  public:
    
    // quality. an amalgamation of OPC standard codes and special "RTX" codes (identified by the 0b10xxxxxx prefix, otherwise unused by OPC)
    //
    
    enum PointQuality: uint8_t {
      
      opc_good         = 0b11000000,  // 192
      opc_bad          = 0b00000000,  // 0
      opc_uncertain    = 0b01000000,  // 64
      opc_rtx_override = 0b10000000,  // 128
      
      rtx_constant     = 0b00000001,
      rtx_interpolated = 0b00000010,
      rtx_averaged     = 0b00000100,
      rtx_aggregated   = 0b00001000,
      rtx_forecasted   = 0b00010000,
      rtx_integrated   = 0b00100000
      
    };
    
    
    //! Empty Constructor, equivalent to Point(0,0,Point::missing,0)
    Point();
    //! Full Constructor, for explicitly setting all internal data within the point object.
    Point(time_t time, double value = 0., PointQuality qual = opc_rtx_override, double confidence = 0.);
    // dtor
    ~Point();
    
    // operators
    Point operator+(const Point& point) const;
    Point& operator+=(const Point& point);
    Point operator+(const double value) const;
    Point& operator+=(const double value);
    Point operator*(const double factor) const;
    Point& operator*=(const double factor);
    Point operator/(const double factor) const;
//    virtual std::ostream& toStream(std::ostream& stream);

    // simple tuple class, so no getters/setters
    time_t time;
    double value;
    PointQuality quality;
    double confidence;
    bool isValid;
    
    // convenience
    const bool hasQual(PointQuality qual) const;
    void addQualFlag(PointQuality qual);
    
    Point inverse();
    
    // static class methods
    static Point convertPoint(const Point& point, const Units& fromUnits, const Units& toUnits);
    static bool comparePointTime(const Point& left, const Point& right);
    static Point linearInterpolate(const Point& p1, const Point& p2, const time_t& t);
    
    
    
    friend std::ostream& operator<<(std::ostream& outputStream, const Point& p)
    {
      return outputStream << p.time << " - " << p.value << " - " << (p.isValid ? "valid" : "invalid") << std::endl;
    }
    
    
  };

  //std::ostream& operator<< (std::ostream &out, Point &point);

}

#endif