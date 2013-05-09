//
//  GainTimeSeries.h
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//

#include "SinglePointFilterModularTimeSeries.h"

#ifndef __epanet_rtx__GainTimeSeries__
#define __epanet_rtx__GainTimeSeries__

namespace RTX {
  class GainTimeSeries : public SinglePointFilterModularTimeSeries {
    
  public:
    RTX_SHARED_POINTER(GainTimeSeries);
    GainTimeSeries();
    
    void setGain(double gain);
    double gain();
    void setGainUnits(Units u);
    Units gainUnits();
    
    virtual void setUnits(Units u);
    virtual bool isCompatibleWith(TimeSeries::sharedPointer ts);
    
  protected:
    Point filteredSingle(Point p, Units sourceU);
    
  private:
    double _gain;
    Units _gainUnits;
    
  };
}

#endif /* defined(__epanet_rtx__GainTimeSeries__) */
