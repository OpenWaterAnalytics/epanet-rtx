//
//  ProjectFile.h
//  epanet-rtx
//
//  Created by Sam Hatchett on 9/24/13.
//
//

#ifndef __epanet_rtx__ProjectFile__
#define __epanet_rtx__ProjectFile__

#include <iostream>

#include "rtxMacros.h"
#include "TimeSeries.h"
#include "PointRecord.h"
#include "Junction.h"
#include "Tank.h"
#include "Reservoir.h"
#include "Link.h"
#include "Pipe.h"
#include "Pump.h"
#include "Valve.h"
#include "Model.h"
#include "Units.h"

#include <list>

using std::map;
using std::vector;
using std::string;


#define RTX_LIST std::list

namespace RTX {
  
  class ProjectFile {
  public:
    RTX_SHARED_POINTER(ProjectFile);
    
    virtual void loadProjectFile(const string& path) = 0;
    virtual void saveProjectFile(const string& path) = 0;
    virtual void clear() = 0;
    
    virtual RTX_LIST<TimeSeries::sharedPointer> timeSeries() = 0;
    virtual RTX_LIST<Clock::sharedPointer> clocks() = 0;
    virtual RTX_LIST<PointRecord::sharedPointer> records() = 0;
    virtual Model::sharedPointer model() = 0;
    
    virtual void insertTimeSeries(TimeSeries::sharedPointer ts) = 0;
    virtual void insertClock(Clock::sharedPointer clock) = 0;
    virtual void insertRecord(PointRecord::sharedPointer record) = 0;
    
  };
  
}






#endif /* defined(__epanet_rtx__ProjectFile__) */
