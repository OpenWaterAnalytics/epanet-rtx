//
//  BufferPointRecord.h
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//

#ifndef __epanet_rtx__BufferPointRecord__
#define __epanet_rtx__BufferPointRecord__


// basics
#include <string>
#include <vector>
#include <deque>
#include <fstream>

#include "Point.h"
#include "rtxMacros.h"
#include "rtxExceptions.h"
#include "PointRecord.h"

#include <boost/circular_buffer.hpp>
#include <boost/signals2/mutex.hpp>

using std::string;

namespace RTX {
  
  class BufferPointRecord : public PointRecord{
    
  public:
    RTX_SHARED_POINTER(BufferPointRecord);
    BufferPointRecord();
    virtual ~BufferPointRecord() {};
    
    virtual std::string registerAndGetIdentifier(std::string recordName);    // registering record names.
    virtual std::vector<std::string> identifiers();
    virtual void preFetchRange(const string& identifier, time_t start, time_t end);   // prepare to retrieve range of values
    
    virtual bool isPointAvailable(const string& identifier, time_t time);
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
    
    // types
    typedef std::pair<double,double> PointPair_t;
    typedef std::pair<time_t, PointPair_t > TimePointPair_t;
    typedef boost::circular_buffer<TimePointPair_t> PointBuffer_t;
    typedef std::pair<PointBuffer_t, boost::shared_ptr<boost::signals2::mutex> > BufferMutexPair_t;
    typedef std::map<std::string, BufferMutexPair_t> KeyedBufferMutexMap_t;
    
    size_t _defaultCapacity;
    
  protected:
    std::string _cachedPointId;
    Point _cachedPoint;
    
  private:
    std::map<std::string, BufferMutexPair_t > _keyedBufferMutex;
    Point makePoint(PointBuffer_t::const_iterator iterator);
  };
  
  std::ostream& operator<< (std::ostream &out, BufferPointRecord &pr);
  
}




#endif /* defined(__epanet_rtx__BufferPointRecord__) */
