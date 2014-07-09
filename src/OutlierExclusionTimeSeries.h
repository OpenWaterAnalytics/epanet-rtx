//
//  OutlierExclusionTimeSeries.h
//  epanet-rtx
//
//  Created by Sam Hatchett on 7/7/14.
//
//

#ifndef __epanet_rtx__OutlierExclusionTimeSeries__
#define __epanet_rtx__OutlierExclusionTimeSeries__

#include <iostream>
#include "ModularTimeSeries.h"

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/variance.hpp>
#include <boost/accumulators/statistics/tail_quantile.hpp>

namespace RTX {
  
  using namespace boost::accumulators;
  
  class OutlierExclusionTimeSeries : public ModularTimeSeries {
    
  public:
    
    typedef enum {
      OutlierExclusionModeInterquartileRange,
      OutlierExclusionModeStdDeviation
    } exclusion_mode_t;
    
    RTX_SHARED_POINTER(OutlierExclusionTimeSeries);
    OutlierExclusionTimeSeries();
    
    void setWindow(Clock::sharedPointer window);
    Clock::sharedPointer window();
    
    void setOutlierMultiplier(double multiplier);
    double outlierMultiplier();
    
    void setExclusionMode(exclusion_mode_t mode);
    exclusion_mode_t exclusionMode();
    
    // overrides
    void setClock(Clock::sharedPointer clock);
    
  protected:
    virtual std::vector<Point> filteredPoints(TimeSeries::sharedPointer sourceTs, time_t fromTime, time_t toTime);
    
  private:
    Clock::sharedPointer _window;
    double _outlierMultiplier;
    exclusion_mode_t _exclusionMode;
    typedef accumulator_set<double, stats<tag::mean, tag::variance(lazy)> > accumulator_t_mean_var;
    typedef accumulator_set<double, stats<tag::tail_quantile<right> > > accumulator_t_right;
    typedef accumulator_set<double, stats<tag::tail_quantile<left> > > accumulator_t_left;
    
    bool shouldIncludePointOnBasis(const Point& p, const std::deque<Point> basis);
    
  };
}

#endif /* defined(__epanet_rtx__OutlierExclusionTimeSeries__) */
