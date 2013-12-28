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
#include "Dma.h"
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
  
  using std::vector;
  using std::string;
  
  class Model {
  public:
    RTX_SHARED_POINTER(Model);
    Model();
    virtual ~Model();
    std::string name();
    void setName(std::string name);
    virtual void loadModelFromFile(const string& filename) throw(std::exception);
    string modelFile();
    virtual void overrideControls() throw(RtxException);
    void runSinglePeriod(time_t time);
    void runExtendedPeriod(time_t start, time_t end);
    void setStorage(PointRecord::sharedPointer record);
    void setParameterSource(PointRecord::sharedPointer record);
    
    bool shouldRunWaterQuality();
    void setShouldRunWaterQuality(bool run);
    
    // DMAs -- identified by boundary link sets (doesHaveFlowMeasure)
    void initDMAs();
    void setDmaShouldDetectClosedLinks(bool detect);
    bool dmaShouldDetectClosedLinks();
    void setDmaPipesToIgnore(vector<Pipe::sharedPointer> ignorePipes);
    vector<Pipe::sharedPointer> dmaPipesToIgnore();
    
    // element accessors
    void addJunction(Junction::sharedPointer newJunction);
    void addTank(Tank::sharedPointer newTank);
    void addReservoir(Reservoir::sharedPointer newReservoir);
    void addPipe(Pipe::sharedPointer newPipe);
    void addPump(Pump::sharedPointer newPump);
    void addValve(Valve::sharedPointer newValve);
    void addDma(Dma::sharedPointer dma);
    Link::sharedPointer linkWithName(const string& name);
    Node::sharedPointer nodeWithName(const string& name);
    vector<Element::sharedPointer> elements();
    vector<Dma::sharedPointer> dmas();
    vector<Junction::sharedPointer> junctions();
    vector<Tank::sharedPointer> tanks();
    vector<Reservoir::sharedPointer> reservoirs();
    vector<Pipe::sharedPointer> pipes();
    vector<Pump::sharedPointer> pumps();
    vector<Valve::sharedPointer> valves();
    
    // simulation properties
    virtual void setHydraulicTimeStep(int seconds);
    int hydraulicTimeStep();
    
    virtual void setQualityTimeStep(int seconds);
    int qualityTimeStep();
    virtual void setInitialQuality(double qual);
    
    virtual time_t currentSimulationTime();
    TimeSeries::sharedPointer iterations() {return _iterations;}
    TimeSeries::sharedPointer relativeError() {return _relativeError;}
    
    bool tanksNeedReset();
    void setTanksNeedReset(bool needReset);
    
    virtual std::ostream& toStream(std::ostream &stream);

    
    // units
    Units flowUnits();
    Units headUnits();
    Units qualityUnits();
    Units volumeUnits();
    
    void setFlowUnits(Units units);
    void setHeadUnits(Units units);
    void setQualityUnits(Units units);
    void setVolumeUnits(Units units);
    
  protected:
    
    void setSimulationParameters(time_t time);
    void saveNetworkStates(time_t time);
    
    
    
    // model parameter setting
    // recreating or wrapping basic api functionality here.
    virtual double reservoirLevel(const string& reservoirName) { return 0; };
    virtual double tankLevel(const string& tankName) { return 0; };
    virtual double junctionHead(const string& junction) { return 0; };
    virtual double junctionDemand(const string& junctionName) { return 0; };
    virtual double junctionQuality(const string& junctionName) { return 0; };
    // link elements
    virtual double pipeFlow(const string& pipe) { return 0; };
    virtual double pumpEnergy(const string& pump) { return 0; };
    
    virtual void setReservoirHead(const string& reservoir, double level) { };
    virtual void setTankLevel(const string& tank, double level) { };
    virtual void setJunctionDemand(const string& junction, double demand) { };
    virtual void setJunctionQuality(const string& junction, double quality) { };
    
    virtual void setPipeStatus(const string& pipe, Pipe::status_t status) { };
    virtual void setPumpStatus(const string& pump, Pipe::status_t status) { };
    virtual void setPumpSetting(const std::string& pump, double setting) { };
    virtual void setValveSetting(const string& valve, double setting) { };
    
    virtual void solveSimulation(time_t time) { };
    virtual time_t nextHydraulicStep(time_t time) { return 0; };
    virtual void stepSimulation(time_t time) { };
    virtual int iterations(time_t time) { return 0; };
    virtual int relativeError(time_t time) { return 0; };
    
    virtual void setCurrentSimulationTime(time_t time);
    
    
    
  private:
    string _name;
    string _modelFile;
    bool _shouldRunWaterQuality;
    bool _tanksNeedReset;
    // master list access
    void add(Junction::sharedPointer newJunction);
    void add(Pipe::sharedPointer newPipe);

    // element lists
    // master node/link lists
    std::map<string, Node::sharedPointer> _nodes;
    std::map<string, Link::sharedPointer> _links;
    // convenience lists for iterations
    vector<Element::sharedPointer> _elements;
    vector<Junction::sharedPointer> _junctions;
    vector<Tank::sharedPointer> _tanks;
    vector<Reservoir::sharedPointer> _reservoirs;
    vector<Pipe::sharedPointer> _pipes;
    vector<Pump::sharedPointer> _pumps;
    vector<Valve::sharedPointer> _valves;
    vector<Dma::sharedPointer> _dmas;
    vector<Pipe::sharedPointer> _dmaPipesToIgnore;
    bool _dmaShouldDetectClosedLinks;
    
    PointRecord::sharedPointer _record;         // default record for results
    Clock::sharedPointer _regularMasterClock;   // normal hydraulic timestep
    TimeSeries::sharedPointer _relativeError;
    TimeSeries::sharedPointer _iterations;
    Clock::sharedPointer _boundaryResetClock;
    int _qualityTimeStep;
    bool _doesOverrideDemands;
    
    time_t _currentSimulationTime;
    
    Units _flowUnits, _headUnits, _qualityUnits, _volumeUnits;

    
  };
  
  std::ostream& operator<< (std::ostream &out, Model &model);

}


#endif
