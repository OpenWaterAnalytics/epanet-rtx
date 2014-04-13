//
//  Resampler.h
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#ifndef epanet_rtx_Resampler_h
#define epanet_rtx_Resampler_h

#include <boost/circular_buffer.hpp>
#include "ModularTimeSeries.h"

namespace RTX {
  
  class Resampler : public ModularTimeSeries {
    
  public:
    typedef boost::circular_buffer<Point> pointBuffer_t;
    typedef std::vector<Point>::const_iterator pVec_cIt;
    typedef enum {linear,step} interpolateMode_t;
    RTX_SHARED_POINTER(Resampler);
    Resampler();
    virtual ~Resampler();
    
    virtual Point point(time_t time);
    
    interpolateMode_t mode();
    void setMode(interpolateMode_t mode);
    
  protected:
    virtual int margin(); // override this for specific subclass implementation -- default is 1
    virtual bool isCompatibleWith(TimeSeries::sharedPointer withTimeSeries);
    virtual std::vector<Point> filteredPoints(TimeSeries::sharedPointer sourceTs, time_t fromTime, time_t toTime);
    //virtual Point filteredSingle(const pointBuffer_t& window, time_t t, Units fromUnits);
    virtual Point filteredSingle(pVec_cIt& vecStart, pVec_cIt& vecEnd, pVec_cIt& vecPos, time_t t, Units fromUnits);
    bool alignVectorIterators(pVec_cIt& start, pVec_cIt& end, pVec_cIt& pos, time_t t, pVec_cIt& back, pVec_cIt& fwd);
    
  private:
    std::pair<time_t,time_t> expandedRange(TimeSeries::sharedPointer sourceTs, time_t start, time_t end);
    interpolateMode_t _mode;

  };
  
}

#endif
