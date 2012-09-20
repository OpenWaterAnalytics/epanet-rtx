//
//  PointRecord.h
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See Readme.txt and license.txt for more information
//  http://tinyurl.com/epanet-rtx

#ifndef epanet_rtx_PointRecord_h
#define epanet_rtx_PointRecord_h

// basics
#include <string>
#include <vector>
#include <deque>
#include <fstream>

#include "Point.h"
#include "rtxMacros.h"

using std::string;

namespace RTX {
  
  /*! 
   \class PointRecord
   \brief A Point Record Class for storing and retrieving Points.
   
   The base PointRecord class works with the PersistentContainer class to store arbitrary point data for arbitrary identifiers. Derive to add specific persistence implementations
   */
  
  /*! 
   \fn virtual Point::sharedPointer PointRecord::point(const std::string &name, time_t time) 
   \brief Get a Point with a specific name at a specific time.
   \param name The name of the data source (tag name).
   \param time The requested time.
   \return The requested Point (as a shared pointer).
   \sa Point
   */
  /*! 
   \fn std::vector<Point::sharedPointer> PointRecord::pointsInRange(const std::string &name, time_t startTime, time_t endTime) 
   \brief Get a vector of Points with a specific name within a specific time range.
   \param name The name of the data source (tag name).
   \param startTime The beginning of the requested time range.
   \param endTime The end of the requested time range.
   \return The requested Points (as a vector of shared pointers)
   \sa Point
   */
  /*! 
   \fn std::vector<Point::sharedPointer> PointRecord::pointsAdjacentTo(const std::string &name, time_t time) 
   \brief Get a vector of Points with a specific name adjacent to a specified time.
   \param name The name of the data source (tag name).
   \param time The requested time.
   \return The requested Points (as a vector of shared pointers)
   \sa Point
   */
  
  class PointRecord {
    
  public:
    RTX_SHARED_POINTER(PointRecord);
    PointRecord();
    virtual ~PointRecord() {};
    friend std::ostream& operator<< (std::ostream &out, PointRecord &pr);
    
    virtual std::string registerAndGetIdentifier(std::string recordName);    // registering record names.
    virtual std::vector<std::string> identifiers()=0;
    virtual void hintAtRange(const string& identifier, time_t start, time_t end)=0;   // prepare to retrieve range of values
    
    virtual bool isPointAvailable(const string& identifier, time_t time) { return false; };
    virtual Point::sharedPointer point(const string& identifier, time_t time)=0;
    virtual Point::sharedPointer pointBefore(const string& identifier, time_t time)=0;
    virtual Point::sharedPointer pointAfter(const string& identifier, time_t time)=0;
    virtual std::vector<Point::sharedPointer> pointsInRange(const string& identifier, time_t startTime, time_t endTime)=0;
    virtual void addPoint(const string& identifier, Point::sharedPointer point)=0;
    virtual void addPoints(const string& identifier, std::vector<Point::sharedPointer> points)=0;
    virtual void reset()=0;
    
  protected:
    virtual std::ostream& toStream(std::ostream &stream);
    int identifierForName(std::string recordName);
    string nameForIdentifier(int identifier);
    
  private:
    std::map< std::string, int > _keys;
    std::map< int, std::string > _names;
    int _nextKey;
    
  };
}

#endif
