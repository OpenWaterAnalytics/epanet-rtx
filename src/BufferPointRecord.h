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
    // types
    typedef boost::circular_buffer<Point> PointBuffer_t;
    typedef std::pair<PointBuffer_t, boost::shared_ptr<boost::signals2::mutex> > BufferMutexPair_t;
    typedef std::map<std::string, BufferMutexPair_t> KeyedBufferMutexMap_t;
    
    RTX_SHARED_POINTER(BufferPointRecord);
    BufferPointRecord();
    virtual ~BufferPointRecord() {};
    
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
    virtual time_pair_t range(const string& id);
    
    virtual std::ostream& toStream(std::ostream &stream);
    
    
  protected:
    
  private:
    PointBuffer_t::iterator _cacheIterator;
    std::map<std::string, BufferMutexPair_t > _keyedBufferMutex;
    size_t _defaultCapacity;
  };
  
  std::ostream& operator<< (std::ostream &out, BufferPointRecord &pr);
  
}




#endif /* defined(__epanet_rtx__BufferPointRecord__) */
