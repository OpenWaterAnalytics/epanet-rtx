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
  
  class BufferPointRecord : public PointRecord {
    
  public:
//     types and small container for the actual buffers
    typedef boost::circular_buffer<Point> PointBuffer_t;
    class Buffer {
    public:
      Units units;
      PointBuffer_t circularBuffer;
    };
    typedef std::map<std::string, Buffer> KeyedBufferMap_t;
    typedef std::pair<std::string, Buffer> StringBufferPair;
    
    RTX_BASE_PROPS(BufferPointRecord);
    BufferPointRecord(int defaultCapacity = RTX_BUFFER_DEFAULT_CACHESIZE);
    virtual ~BufferPointRecord() {};
    
    virtual bool registerAndGetIdentifierForSeriesWithUnits(std::string recordName, Units units);    // registering record names.
    virtual IdentifierUnitsList identifiersAndUnits();
    
    virtual Point point(const string& identifier, time_t time);
    virtual Point pointBefore(const string& identifier, time_t time);
    virtual Point pointAfter(const string& identifier, time_t time);
    virtual std::vector<Point> pointsInRange(const string& identifier, TimeRange range);
    virtual void addPoint(const string& identifier, Point point);
    virtual void addPoints(const string& identifier, std::vector<Point> points);
    virtual void reset();
    virtual void reset(const string& identifier);
    virtual Point firstPoint(const string& id);
    virtual Point lastPoint(const string& id);
    virtual TimeRange range(const string& id);
    
    virtual std::ostream& toStream(std::ostream &stream);
    
    
  protected:
    
  private:
    KeyedBufferMap_t _keyedBuffers;
    size_t _defaultCapacity;
    boost::signals2::mutex _bigMutex;
  };
  
  std::ostream& operator<< (std::ostream &out, BufferPointRecord &pr);
  
}




#endif /* defined(__epanet_rtx__BufferPointRecord__) */
