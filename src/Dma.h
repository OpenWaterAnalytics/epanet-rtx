//
//  Dma.h
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#ifndef epanet_rtx_Dma_h
#define epanet_rtx_Dma_h

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
 
   \class Dma
   \brief Dma represents a collection of junctions that can be considered together in a control volume.
   
   Derive from the Dma class to implement custom demand allocation schemes.
 
   
   \fn virtual void Dma::setRecord(PointRecord::sharedPointer record)
   \brief Set the intended PointRecord object for storing data. Defaults to the generic TimeSeries storage mechanism.
   \param record The PointRecord pointer
   \sa PointRecord TimeSeries
   
   
   \fn void Dma::enumerateJunctionsWithRootNode(Junction::sharedPointer junction)
   \brief Add a group of junctions to a Dma.
   
   Uses graph connectivity to enumerate the junctions in a dma starting with the passed Junction pointer. The enumeration uses a depth-first search to follow Pipe elements, and stops at links which return true to the doesHaveFlowMeasure() method.
   
   \param junction A single junction within the intended dma.
   \sa Junction Pipe
   
   
   
   \fn void Dma::setDemand(TimeSeries::sharedPointer demand);
   \brief Set the TimeSeries to use as the basis for demand allocation.
   \param demand A TimeSeries pointer
   \sa TimeSeries
   
   
   \fn virtual void Dma::allocateDemandToJunctions(time_t time)
   \brief Allocate demand from the DMA's demand TimeSeries to the constituent junctions.
   \param time The time frame for which to perform the allocation.
   \sa TimeSeries Junction
   */
  
  
  class Dma : public Element {
  public:
    RTX_SHARED_POINTER(Dma);
    virtual std::ostream& toStream(std::ostream &stream);
    Dma(const std::string& name);
    ~Dma();
    
    virtual void setRecord(PointRecord::sharedPointer record);
    
    // node accessors
    void addJunction(Junction::sharedPointer junction);
    void removeJunction(Junction::sharedPointer junction);
    void enumerateJunctionsWithRootNode(Junction::sharedPointer junction, bool stopAtClosedLinks = false, std::vector<Pipe::sharedPointer> ignorePipes = std::vector<Pipe::sharedPointer>() );
    Junction::sharedPointer findJunction(std::string name);
    bool doesHaveJunction(Junction::sharedPointer j);
    std::vector<Junction::sharedPointer> junctions();
    std::vector<Tank::sharedPointer> tanks();
    std::map<Pipe::sharedPointer, Pipe::direction_t> measuredBoundaryPipes();
    std::map<Pipe::sharedPointer, Pipe::direction_t> closedBoundaryPipes();
    std::vector<Pipe::sharedPointer> closedInteriorPipes();
    std::vector<Pipe::sharedPointer> measuredInteriorPipes();
    bool isMeasuredPipe(Pipe::sharedPointer pipe);
    bool isClosedPipe(Pipe::sharedPointer pipe);

    // time series accessors
    TimeSeries::sharedPointer demand();
    void setDemand(TimeSeries::sharedPointer demand);
    void setJunctionFlowUnits(Units units);
    
    // business logic
    virtual void allocateDemandToJunctions(time_t time);
        
  private:
    // void followJunction(Junction::sharedPointer junction);
    bool isBoundaryFlowJunction(Junction::sharedPointer junction);
    bool isTank(Junction::sharedPointer junction);
    bool isAlwaysClosed(Pipe::sharedPointer pipe);
    
    std::vector<Junction::sharedPointer> _boundaryFlowJunctions;
    std::vector<Tank::sharedPointer> _tanks;
    std::map< std::string, Junction::sharedPointer> _junctions;
    std::map<Pipe::sharedPointer, Pipe::direction_t> _measuredBoundaryPipesDirectional;
    std::map<Pipe::sharedPointer, Pipe::direction_t> _closedBoundaryPipesDirectional;
    std::vector<Pipe::sharedPointer> _closedInteriorPipes;
    std::vector<Pipe::sharedPointer> _measuredInteriorPipes;

    TimeSeries::sharedPointer _demand;
    Units _flowUnits;
  };
}


#endif
