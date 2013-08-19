//
//  PointRecord.h
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#ifndef epanet_rtx_DequePointRecord_h
#define epanet_rtx_DequePointRecord_h

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
  
  class DequePointRecord : public PointRecord {
    
  public:
    RTX_SHARED_POINTER(DequePointRecord);
    DequePointRecord();
    virtual ~DequePointRecord() {};
    
    virtual std::string registerAndGetIdentifier(std::string recordName, Units dataUnits);    // registering record names.
    virtual std::vector<std::string> identifiers();
    
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
    typedef std::deque<Point> pointQ_t;
    typedef std::map< std::string, pointQ_t> keyedPointVector_t;
    
  private:
    keyedPointVector_t _points;
    // private methods
    pointQ_t& pointQueueWithKeyName(const std::string& name);
    
    
  };
  
  std::ostream& operator<< (std::ostream &out, DequePointRecord &pr);

}

#endif
