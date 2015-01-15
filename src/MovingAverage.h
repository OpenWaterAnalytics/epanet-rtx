//
//  MovingAverage.h
//  epanet_rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#ifndef rtx_movingaverage_h
#define rtx_movingaverage_h

#include "Resampler.h"

using std::vector;


/*
 
 implement a moving average filter, using the "source" timeseries as input. 
 cache the smoothed points using the base-class "TimeSeries::add" methods.
 probably need to detect gaps in the cache (check ::period() ).
 simplest version would be to construct each requested point as an EWA of previous source points.
 
 */

namespace RTX {
  class MovingAverage : public Resampler {
  public:
    RTX_SHARED_POINTER(MovingAverage);
    MovingAverage();
    virtual ~MovingAverage();
    //vector<Point> sourceTimeSeries(const std::string& nameSource, PointRecord::_sp source, time_t tStart, long tEnd);
    
    // added functionality
    void setWindowSize(int numberOfPoints);   // set number of points to consider in the moving average calculation
    int windowSize();                         // return the window size (see above)
    
  protected:
    virtual bool isCompatibleWith(TimeSeries::_sp withTimeSeries);
    virtual int margin();
    //virtual Point filteredSingle(const pointBuffer_t& window, time_t t, Units fromUnits);
    virtual Point filteredSingle(pVec_cIt& vecStart, pVec_cIt& vecEnd, pVec_cIt& vecPos, time_t t, Units fromUnits);
    
  private:
    int _windowSize;
  };
}


#endif
