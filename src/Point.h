//
//  Point.h
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See Readme.txt and license.txt for more information
//  https://sourceforge.net/projects/epanet-rtx/

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
    friend std::ostream& operator<< (std::ostream &out, RTX::Point &point);
    
  public:
    RTX_SHARED_POINTER(Point);

    //! quality flag
    enum Qual_t { good, missing, estimated, forecasted, interpolated };
    
    //! Empty Constructor, equivalent to Point(0,0,Point::missing,0)
    Point();
    //! Full Constructor, for explicitly setting all internal data within the point object.
    Point(time_t time, double value, Qual_t qual = good, double confidence = 0.);
    // dtor
    ~Point();
    
    Point operator+(const Point& point) const;
    Point operator+=(const Point& point);
    Point operator*(const double factor) const;
    Point operator/(const double factor) const;
    static Point::sharedPointer convertPoint(const Point& point, const Units& fromUnits, const Units& toUnits);
    
    time_t time() const;
    double value() const;
    Qual_t quality() const;
    double confidence() const;
    
  private:
    time_t _time;
    double _value;
    Qual_t _qual;
    double _confidence;
    
  };

  


}

#endif