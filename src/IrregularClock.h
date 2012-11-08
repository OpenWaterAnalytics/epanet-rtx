//
//  IrregularClock.h
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#ifndef epanet_rtx_IrregularClock_h
#define epanet_rtx_IrregularClock_h

#include "Clock.h"
#include "PointContainer.h"

namespace RTX {
  
  class IrregularClock : public Clock {
  public:
    RTX_SHARED_POINTER(IrregularClock);
    IrregularClock(PointContainer::sharedPointer pointContainer);
    virtual ~IrregularClock();
    
    virtual bool isCompatibleWith(Clock::sharedPointer clock);
    virtual bool isValid(time_t time);
    virtual time_t timeAfter(time_t time);
    virtual time_t timeBefore(time_t time);
    virtual std::ostream& toStream(std::ostream &stream);
    
  private:
    PointContainer::sharedPointer _pointContainer;
  };
  
}

#endif
