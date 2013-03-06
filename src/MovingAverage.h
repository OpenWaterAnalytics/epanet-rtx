//
//  MovingAverage.h
//  epanet_rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#ifndef rtx_movingaverage_h
#define rtx_movingaverage_h

#include "ModularTimeSeries.h"
#include "TimeSeries.h"

using std::vector;


/*
 
 implement a moving average filter, using the "source" timeseries as input. 
 cache the smoothed points using the base-class "TimeSeries::add" methods.
 probably need to detect gaps in the cache (check ::period() ).
 simplest version would be to construct each requested point as an EWA of previous source points.
 
 */

namespace RTX {
  class MovingAverage : public ModularTimeSeries {
  public:
    RTX_SHARED_POINTER(MovingAverage);
    MovingAverage();
    virtual ~MovingAverage();
    //vector<Point> sourceTimeSeries(const std::string& nameSource, PointRecord::sharedPointer source, time_t tStart, long tEnd);
    
    // added functionality
    void setWindowSize(int numberOfPoints);   // set number of points to consider in the moving average calculation
    int windowSize();                         // return the window size (see above)
    
    // overridden methods (from derived classes)
    virtual Point point(time_t time);
    virtual std::vector< Point > points(time_t start, time_t end);
    
  protected:
    virtual bool isCompatibleWith(TimeSeries::sharedPointer withTimeSeries);
    
  private:
    // methods
    Point movingAverageAt(time_t time);
    double calculateAverage(vector< Point > points);
    // attributes
    int N;
    //array to store moving average
    double movAvg[100];
    typedef vector<Point>::const_iterator pointVectorIterator;
    int k, k2, i;
    int _windowSize;
  };
}


#endif
