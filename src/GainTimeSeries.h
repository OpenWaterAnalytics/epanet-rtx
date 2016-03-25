//
//  GainTimeSeries.h
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//

#include "TimeSeriesFilterSinglePoint.h"

#ifndef __epanet_rtx__GainTimeSeries__
#define __epanet_rtx__GainTimeSeries__

namespace RTX {
  class GainTimeSeries : public TimeSeriesFilterSinglePoint {
    
  public:
    RTX_BASE_PROPS(GainTimeSeries);
    GainTimeSeries();
    
    double gain();
    void setGain(double gain);
    
    Units gainUnits();
    void setGainUnits(Units u);
    
  protected:
    Point filteredWithSourcePoint(Point sourcePoint);
    virtual bool canSetSource(TimeSeries::_sp ts);
    virtual void didSetSource(TimeSeries::_sp ts);
    virtual bool canChangeToUnits(Units units);
    
  private:
    double _gain;
    Units _gainUnits;
    
  };
}

#endif /* defined(__epanet_rtx__GainTimeSeries__) */
