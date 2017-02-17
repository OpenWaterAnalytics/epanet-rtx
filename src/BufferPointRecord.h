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
    
    RTX_BASE_PROPS(BufferPointRecord);
    BufferPointRecord(int defaultCapacity = RTX_BUFFER_DEFAULT_CACHESIZE);
    virtual ~BufferPointRecord() {};
    
    virtual bool registerAndGetIdentifierForSeriesWithUnits(std::string recordName, Units units);    // registering record names.
    virtual IdentifierUnitsList identifiersAndUnits();
    
    virtual Point point(const string& identifier, time_t time);
    virtual Point pointBefore(const string& identifier, time_t time);
    virtual Point pointAfter(const string& identifier, time_t time);
    virtual std::vector<Point> pointsInRange(const string& identifier, TimeRange range);
    virtual Point firstPoint(const string& id);
    virtual Point lastPoint(const string& id);
    virtual TimeRange range(const string& id);
    
    virtual void addPoint(const string& identifier, Point point);
    virtual void addPoints(const string& identifier, std::vector<Point> points);
    
    virtual void reset();
    virtual void reset(const string& identifier);
    
    virtual std::ostream& toStream(std::ostream &stream);
    
    
  protected:
    
  private:
    typedef boost::circular_buffer<Point> PointBuffer;
    class Buffer {
    public:
      Units units;
      PointBuffer circularBuffer;
    };
    std::map<std::string, Buffer> _keyedBuffers;
    size_t _defaultCapacity;
    boost::signals2::mutex _bigMutex;
  };
  
  std::ostream& operator<< (std::ostream &out, BufferPointRecord &pr);
  
}




#endif /* defined(__epanet_rtx__BufferPointRecord__) */
