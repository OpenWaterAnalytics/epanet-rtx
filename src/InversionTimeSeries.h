//
//  InversionTimeSeries.h
//  epanet-rtx
//
//  Open Water Analytics [wateranalytics.org]
//  See README.md and license.txt for more information
//

#ifndef __epanet_rtx__InversionTimeSeries__
#define __epanet_rtx__InversionTimeSeries__

#include <iostream>
#include "SinglePointFilterModularTimeSeries.h"

namespace RTX {
  class InversionTimeSeries : public SinglePointFilterModularTimeSeries {
    
  public:
    RTX_SHARED_POINTER(InversionTimeSeries);
    void setSource(TimeSeries::sharedPointer ts);
    
  protected:
    void checkUnits();
    
  private:
    Point filteredSingle(Point p, Units sourceU);
    
  };
}



#endif /* defined(__epanet_rtx__InversionTimeSeries__) */
