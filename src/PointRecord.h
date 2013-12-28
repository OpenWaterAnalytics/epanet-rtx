//
//  PointRecord.h
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#ifndef epanet_rtx_PointRecord_h
#define epanet_rtx_PointRecord_h

// basics
#include <string>
#include <vector>
#include <deque>
#include <fstream>

#include "Point.h"
#include "rtxMacros.h"
#include "rtxExceptions.h"

using std::string;

namespace RTX {
  
  /*! 
   \class PointRecord
   \brief A Point Record Class for storing and retrieving Points.
   
   The base PointRecord class just keeps short-term records. Derive to add specific persistence implementations
   */
  
  /*! 
   \fn virtual Point PointRecord::point(const std::string &name, time_t time) 
   \brief Get a Point with a specific name at a specific time.
   \param name The name of the data source (tag name).
   \param time The requested time.
   \return The requested Point (as a shared pointer).
   \sa Point
   */
  /*! 
   \fn std::vector<Point> PointRecord::pointsInRange(const std::string &name, time_t startTime, time_t endTime) 
   \brief Get a vector of Points with a specific name within a specific time range.
   \param name The name of the data source (tag name).
   \param startTime The beginning of the requested time range.
   \param endTime The end of the requested time range.
   \return The requested Points (as a vector of shared pointers)
   \sa Point
   */
  
    
  class PointRecord {
    
  public:
    RTX_SHARED_POINTER(PointRecord);
    typedef std::pair<time_t, time_t> time_pair_t;
    
    PointRecord();
    virtual ~PointRecord() {};
    
    std::string name();
    void setName(std::string name);
    
    virtual std::string registerAndGetIdentifier(std::string recordName, Units dataUnits);    // registering record names.
    virtual std::vector<std::string> identifiers();
    
    //virtual bool isPointAvailable(const string& identifier, time_t time);
    virtual Point point(const string& identifier, time_t time);
    virtual Point pointBefore(const string& identifier, time_t time);
    virtual Point pointAfter(const string& identifier, time_t time);
    virtual std::vector<Point> pointsInRange(const string& identifier, time_t startTime, time_t endTime);
    virtual void addPoint(const string& identifier, Point point);
    virtual void addPoints(const string& identifier, std::vector<Point> points);
    virtual void reset();
    virtual void reset(const string& identifier);
    virtual Point firstPoint(const string& id);
    virtual Point lastPoint(const string& id);
    virtual time_pair_t range(const string& id);
    
    virtual std::ostream& toStream(std::ostream &stream);

  protected:
    std::string _cachedPointId;
    Point _cachedPoint;
    
    std::map<std::string,Point> _pointCache;
    
  private:
    std::string _name;
  
  };
  
  std::ostream& operator<< (std::ostream &out, PointRecord &pr);

}

#endif
