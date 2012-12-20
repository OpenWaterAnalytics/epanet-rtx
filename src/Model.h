//
//  Model.h
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#ifndef epanet_rtx_Model_h
#define epanet_rtx_Model_h

#include <string.h>
#include <map>
#include <tr1/unordered_map>
#include <time.h>
#include <boost/foreach.hpp>
#include "rtxExceptions.h"
#include "Element.h"
#include "Node.h"
#include "Tank.h"
#include "Reservoir.h"
#include "Link.h"
#include "Pipe.h"
#include "Pump.h"
#include "Valve.h"
#include "Zone.h"
#include "PointRecord.h"
#include "Units.h"
#include "rtxMacros.h"


namespace RTX {
  
  /*!
   \class Model
   \brief A hydraulic / water quality model abstraction.
   
   Provides methods for simulation and storing/retrieving states and parameters, and accessing infrastructure elements
   
   \sa Element, Junction, Pipe
   
   */
  
  class Model {
  public:
    RTX_SHARED_POINTER(Model);
    
    virtual void loadModelFromFile(const std::string& filename) throw(RtxException);
    std::string modelFile();
    virtual void overrideControls() throw(RtxException);
    void runSinglePeriod(time_t time);
    void runExtendedPeriod(time_t start, time_t end);
    void setStorage(PointRecord::sharedPointer record);
    void setParameterSource(PointRecord::sharedPointer record);
    
    // demand zones -- identified by boundary link sets (doesHaveFlowMeasure)
    void initDemandZones();
    
    // element accessors
    void addJunction(Junction::sharedPointer newJunction);
    void addTank(Tank::sharedPointer newTank);
    void addReservoir(Reservoir::sharedPointer newReservoir);
    void addPipe(Pipe::sharedPointer newPipe);
    void addPump(Pump::sharedPointer newPump);
    void addValve(Valve::sharedPointer newValve);
    void addZone(Zone::sharedPointer zone);
    Link::sharedPointer linkWithName(const std::string& name);
    Node::sharedPointer nodeWithName(const std::string& name);
    std::vector<Element::sharedPointer> elements();
    std::vector<Zone::sharedPointer> zones();
    std::vector<Junction::sharedPointer> junctions();
    std::vector<Tank::sharedPointer> tanks();
    std::vector<Reservoir::sharedPointer> reservoirs();
    std::vector<Pipe::sharedPointer> pipes();
    std::vector<Pump::sharedPointer> pumps();
    std::vector<Valve::sharedPointer> valves();
    
    // simulation properties
    virtual void setHydraulicTimeStep(int seconds);
    int hydraulicTimeStep();
    
    virtual void setQualityTimeStep(int seconds);
    int qualityTimeStep();
    
    virtual time_t currentSimulationTime();
    
    virtual std::ostream& toStream(std::ostream &stream);

  protected:
    Model();
    virtual ~Model();
    
    void setSimulationParameters(time_t time);
    void saveHydraulicStates(time_t time);
    
    // units
    Units flowUnits();
    Units headUnits();
    void setFlowUnits(Units units);
    void setHeadUnits(Units units);
    
    
    // model parameter setting
    // recreating or wrapping basic api functionality here.
    virtual double reservoirLevel(const std::string& reservoirName) = 0;
    virtual double tankLevel(const std::string& tankName) = 0;
    virtual double junctionHead(const std::string& junction) = 0;
    virtual double junctionDemand(const std::string& junctionName) = 0;
    virtual double junctionQuality(const std::string& junctionName) = 0;
    // link elements
    virtual double pipeFlow(const std::string& pipe) = 0;
    virtual double pumpEnergy(const std::string& pump) = 0;
    
    virtual void setReservoirHead(const std::string& reservoir, double level) = 0;
    virtual void setTankLevel(const std::string& tank, double level) = 0;
    virtual void setJunctionDemand(const std::string& junction, double demand) = 0;
    
    virtual void setPipeStatus(const std::string& pipe, Pipe::status_t status) = 0;
    virtual void setPumpStatus(const std::string& pump, Pipe::status_t status) = 0;
    virtual void setValveSetting(const std::string& valve, double setting) = 0;
    
    virtual void solveSimulation(time_t time) = 0;
    virtual time_t nextHydraulicStep(time_t time) = 0;
    virtual void stepSimulation(time_t time) = 0;
    virtual int iterations(time_t time) = 0;
    virtual int relativeError(time_t time) = 0;
    
    virtual void setCurrentSimulationTime(time_t time);
    
    
    
  private:
    std::string _modelFile;
    // master list access
    void add(Junction::sharedPointer newJunction);
    void add(Pipe::sharedPointer newPipe);

    // element lists
    // master node/link lists
    std::tr1::unordered_map<std::string, Node::sharedPointer> _nodes;
    std::tr1::unordered_map<std::string, Link::sharedPointer> _links;
    // convenience lists for iterations
    std::vector<Element::sharedPointer> _elements;
    std::vector<Junction::sharedPointer> _junctions;
    std::vector<Tank::sharedPointer> _tanks;
    std::vector<Reservoir::sharedPointer> _reservoirs;
    std::vector<Pipe::sharedPointer> _pipes;
    std::vector<Pump::sharedPointer> _pumps;
    std::vector<Valve::sharedPointer> _valves;
    std::vector<Zone::sharedPointer> _zones;
    
    PointRecord::sharedPointer _record;         // default record for results
    Clock::sharedPointer _regularMasterClock;   // normal hydraulic timestep
    TimeSeries::sharedPointer _relativeError;
    TimeSeries::sharedPointer _iterations;
    Clock::sharedPointer _boundaryResetClock;
    int _qualityTimeStep;
    bool _doesOverrideDemands;
    
    time_t _currentSimulationTime;
    
    Units _flowUnits, _headUnits;

    
  };
  
  std::ostream& operator<< (std::ostream &out, Model &model);

}


#endif
