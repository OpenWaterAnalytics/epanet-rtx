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
#include <set>
#include "rtxMacros.h"
#include "TimeSeries.h"
#include "Junction.h"
#include "Tank.h"
#include "Link.h"
#include "Pipe.h"
#include "Units.h"

//#include "networkGraph.h"

namespace RTX {

  /*!
 
   \class Dma
   \brief Dma represents a collection of junctions that can be considered together in a control volume.
   
   Derive from the Dma class to implement custom demand allocation schemes.
 
   
   \fn virtual void Dma::setRecord(PointRecord::_sp record)
   \brief Set the intended PointRecord object for storing data. Defaults to the generic TimeSeries storage mechanism.
   \param record The PointRecord pointer
   \sa PointRecord TimeSeries
   
   
   \fn void Dma::enumerateJunctionsWithRootNode(Junction::_sp junction)
   \brief Add a group of junctions to a Dma.
   
   Uses graph connectivity to enumerate the junctions in a dma starting with the passed Junction pointer. The enumeration uses a depth-first search to follow Pipe elements, and stops at links which have a flow measure.
   
   \param junction A single junction within the intended dma.
   \sa Junction Pipe
   
   
   
   \fn void Dma::setDemand(TimeSeries::_sp demand);
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
    RTX_BASE_PROPS(Dma);
    typedef std::pair<Pipe::_sp, Pipe::direction_t> pipeDirPair_t;
    
    virtual std::ostream& toStream(std::ostream &stream);
    Dma(const std::string& name);
    ~Dma();
    
    // static class methods
//    static std::list<Dma::_sp> enumerateDmas(std::vector<Node::_sp> nodes);
    
    
    virtual void setRecord(PointRecord::_sp record);
    
    void initDemandTimeseries(const std::set<Pipe::_sp> &boundarySet);
    
    // node accessors
    void addJunction(Junction::_sp junction);
    void removeJunction(Junction::_sp junction);
//    void enumerateJunctionsWithRootNode(Junction::_sp junction, bool stopAtClosedLinks = false, std::vector<Pipe::_sp> ignorePipes = std::vector<Pipe::_sp>() );
    Junction::_sp findJunction(std::string name);
    bool doesHaveJunction(Junction::_sp j);
    std::set<Junction::_sp> junctions();
    std::set<Tank::_sp> tanks();
    std::vector<Dma::pipeDirPair_t> measuredBoundaryPipes();
    std::vector<Dma::pipeDirPair_t> closedBoundaryPipes();
    std::vector<Pipe::_sp> closedInteriorPipes();
    std::vector<Pipe::_sp> measuredInteriorPipes();
    bool isMeasuredBoundaryPipe(Pipe::_sp pipe);
    bool isMeasuredInteriorPipe(Pipe::_sp pipe);
    bool isMeasuredPipe(Pipe::_sp pipe);
    bool isClosedBoundaryPipe(Pipe::_sp pipe);
    bool isClosedInteriorPipe(Pipe::_sp pipe);
    bool isClosedPipe(Pipe::_sp pipe);
    bool isBoundaryPipe(Pipe::_sp pipe);

    // time series accessors
    TimeSeries::_sp demand();
    void setDemand(TimeSeries::_sp demand);
    void setJunctionFlowUnits(Units units);
    
    // business logic
    virtual int allocateDemandToJunctions(time_t time);
    
    std::string hashedName;
    
  private:
    // void followJunction(Junction::_sp junction);
    bool isBoundaryFlowJunction(Junction::_sp junction);
    bool isTank(Junction::_sp junction);
    bool isAlwaysClosed(Pipe::_sp pipe);
    
    std::vector<Junction::_sp> _boundaryFlowJunctions;
    std::set<Tank::_sp> _tanks;
    std::set<Junction::_sp> _junctions;
    std::vector<Dma::pipeDirPair_t> _measuredBoundaryPipesDirectional;
    std::vector<Dma::pipeDirPair_t> _closedBoundaryPipesDirectional;
    std::vector<Pipe::_sp> _closedInteriorPipes;
    std::vector<Pipe::_sp> _measuredInteriorPipes;

    TimeSeries::_sp _demand;
    Units _flowUnits;
  };
}


#endif
