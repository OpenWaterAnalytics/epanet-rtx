//
//  FirstDerivative.h
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See Readme.txt and license.txt for more information
//  https://sourceforge.net/projects/epanet-rtx/

#ifndef epanet_rtx_FirstDerivative_h
#define epanet_rtx_FirstDerivative_h

#include "ModularTimeSeries.h"

namespace RTX {
  
  class FirstDerivative : public ModularTimeSeries {
  public:
    RTX_SHARED_POINTER(FirstDerivative);
    FirstDerivative();
    virtual ~FirstDerivative();
    
    virtual Point::sharedPointer point(time_t time);

  protected:
    virtual std::ostream& toStream(std::ostream &stream);
    
  };
  
}// namespace

#endif
