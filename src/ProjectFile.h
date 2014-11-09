//
//  ProjectFile.h
//  epanet-rtx
//
//  Open Water Analytics [wateranalytics.org]
//  See README.md and license.txt for more information
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
    
    // public internal class description for project summary
    typedef enum { B_NONE, B_HEAD, B_STATUS, B_DMA_STATUS, B_SETTING, B_DMA_SETTING, B_QUALITY, B_DEMAND } boundary_t;
    typedef enum { M_NONE, M_HEAD, M_FLOW, M_DMA_FLOW, M_QUALITY } measure_t;

    class ElementSummary {
    public:
      ElementSummary() : count(0),minGap(0),maxGap(0),medianGap(0) { };
      Element::sharedPointer element;
      TimeSeries::sharedPointer data;
      boundary_t boundaryType;
      measure_t measureType;
      size_t count;
      double minGap,maxGap,medianGap;
    };
    
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
    
    std::vector<ElementSummary> projectSummary(time_t start, time_t end);
    
  };
  
}






#endif /* defined(__epanet_rtx__ProjectFile__) */
