//
//  Zone.h
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#ifndef epanet_rtx_Zone_h
#define epanet_rtx_Zone_h

#include <vector>
#include "rtxMacros.h"
#include "TimeSeries.h"
#include "Junction.h"
#include "Tank.h"
#include "Link.h"
#include "Pipe.h"
#include "Units.h"

namespace RTX {

  /*!
 
   \class Zone
   \brief Zone represents a collection of junctions that can be considered together in a control volume.
   
   Derive from the Zone class to implement custom demand allocation schemes.
 
   
   \fn virtual void Zone::setRecord(PointRecord::sharedPointer record)
   \brief Set the intended PointRecord object for storing data. Defaults to the generic TimeSeries storage mechanism.
   \param record The PointRecord pointer
   \sa PointRecord TimeSeries
   
   
   \fn void Zone::enumerateJunctionsWithRootNode(Junction::sharedPointer junction)
   \brief Add a group of junctions to a Zone.
   
   Uses graph connectivity to enumerate the junctions in a zone starting with the passed Junction pointer. The enumeration uses a depth-first search to follow Pipe elements, and stops at links which return true to the doesHaveFlowMeasure() method.
   
   \param junction A single junction within the intended zone.
   \sa Junction Pipe
   
   
   
   \fn void Zone::setDemand(TimeSeries::sharedPointer demand);
   \brief Set the TimeSeries to use as the basis for demand allocation.
   \param demand A TimeSeries pointer
   \sa TimeSeries
   
   
   \fn virtual void Zone::allocateDemandToJunctions(time_t time)
   \brief Allocate demand from the Zone's demand TimeSeries to the constituent junctions.
   \param time The time frame for which to perform the allocation.
   \sa TimeSeries Junction
   */
  
  
  class Zone : public Element {
  public:
    RTX_SHARED_POINTER(Zone);
    virtual std::ostream& toStream(std::ostream &stream);
    Zone(const std::string& name);
    ~Zone();
    
    virtual void setRecord(PointRecord::sharedPointer record);
    
    // node accessors
    void addJunction(Junction::sharedPointer junction);
    void removeJunction(Junction::sharedPointer junction);
    void enumerateJunctionsWithRootNode(Junction::sharedPointer junction, bool stopAtClosedLinks = false);
    Junction::sharedPointer findJunction(std::string name);
    bool doesHaveJunction(Junction::sharedPointer j);
    std::vector<Junction::sharedPointer> junctions();
    
    // time series accessors
    TimeSeries::sharedPointer demand();
    void setDemand(TimeSeries::sharedPointer demand);
    void setJunctionFlowUnits(Units units);
    
    // business logic
    virtual void allocateDemandToJunctions(time_t time);
        
  private:
    void followJunction(Junction::sharedPointer junction);
    bool isBoundaryFlowJunction(Junction::sharedPointer junction);
    bool isTank(Junction::sharedPointer junction);
    
    std::vector<Junction::sharedPointer> _boundaryFlowJunctions;
    std::vector<Tank::sharedPointer> _tanks;
    std::map< std::string, Junction::sharedPointer> _junctions;
    typedef enum {outDirection, inDirection} direction_t;
    std::map<Pipe::sharedPointer, direction_t> _boundaryPipesDirectional;
    
    TimeSeries::sharedPointer _demand;
    Units _flowUnits;
  };
}


#endif
