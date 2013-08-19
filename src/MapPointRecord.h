//
//  MapPointRecord.h
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#ifndef epanet_rtx_MapPointRecord_h
#define epanet_rtx_MapPointRecord_h

// basics
#include <string>
#include <vector>
#include <deque>
#include <fstream>

#include "Point.h"
#include "rtxMacros.h"
#include "rtxExceptions.h"
#include "PointRecord.h"

using std::string;

namespace RTX {
  
  class MapPointRecord : public PointRecord{
    
  public:
    RTX_SHARED_POINTER(MapPointRecord);
    MapPointRecord();
    virtual ~MapPointRecord() {};
    
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
    
    virtual std::ostream& toStream(std::ostream &stream);

  protected:
    typedef std::map<time_t, Point> pointMap_t;
    typedef std::map< int, pointMap_t> keyedPointMap_t;
    int identifierForName(std::string recordName);
    string nameForIdentifier(int identifier);

    
  private:
    keyedPointMap_t _points;
    std::map< std::string, int > _keys;
    std::map< int, std::string > _names;
    int _nextKey;
    std::string _connectionString;
    
  };
  
  std::ostream& operator<< (std::ostream &out, MapPointRecord &pr);

}

#endif
