//
//  Model.h
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#ifndef epanet_rtx_Model_h
#define epanet_rtx_Model_h

#include <string>
#include <map>
#include <time.h>

#include <future>

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
#include <PointRecord.h>
#include <Units.h>
#include <Curve.h>
#include "rtxMacros.h"

using TSF::TimeSeries;
using TSF::Units;
using TSF::Clock;
using TSF::PointRecord;

namespace RTX {
  
  /*!
   \class Model
   \brief A hydraulic / water quality model abstraction.
   
   Provides methods for simulation and storing/retrieving states and parameters, and accessing infrastructure elements
   
   \sa Element, Junction, Pipe
   
   */
  
  using std::vector;
  using std::string;
  using std::set;
  
  class Model : public RTX_object {
  public:
    TSF_BASE_PROPS(Model);
    typedef std::function<void(const std::string&)> RTX_Logging_Callback_Block;
    
    Model();
    Model(const std::string& filename);
    virtual ~Model();
    
    virtual void initEngine() { };
    virtual void closeEngine() { };
    std::string name();
    void setName(std::string name);
        
//    virtual void loadModelFromFile(const string& filename) throw(std::exception);
    virtual void useModelFromPath(const std::string& path);
    virtual string modelFile();
    virtual string modelHash();
    virtual void overrideControls();
    virtual std::string getProjectionString();
    virtual void setProjectionString(std::string projectionString);
    
    /// simulation methods
    void runSinglePeriod(time_t time);
    void runExtendedPeriod(time_t start, time_t end);
    void runForecast(time_t start, time_t end);
    
    bool solveAndSaveOutputAtTime(time_t simulationTime);
    
    bool solveInitial(time_t time);
    bool updateSimulationToTime(time_t time);
    virtual void cleanupModelAfterSimulation() {};
    
    void cancelSimulation();
    
    void refreshRecordsForModeledStates();
    
    // these were considered but never used / implemented
//    void setStorage(PointRecord::_sp record);
//    void setParameterSource(PointRecord::_sp record);
    
    virtual void disableControls() {};
    virtual void enableControls() {};
    
    bool shouldRunWaterQuality();
    void setShouldRunWaterQuality(bool run);
    
    enum QualityType {
      None = 0,
      Age = 1,
      Trace = 2,
      UNKNOWN = 3
    };
    
    virtual void setQualityOptions(QualityType qt, const std::string& traceNode = "") = 0;
    virtual QualityType qualityType() = 0;
    virtual std::string qualityTraceNode() = 0;
    
    // DMAs -- identified by boundary link sets (doesHaveFlowMeasure)
    void initDMAs();
    void setDmaShouldDetectClosedLinks(bool detect);
    bool dmaShouldDetectClosedLinks();
    void setDmaPipesToIgnore(vector<Pipe::_sp> ignorePipes);
    vector<Pipe::_sp> dmaPipesToIgnore();
    
    // element accessors
    void addJunction(Junction::_sp newJunction);
    void addTank(Tank::_sp newTank);
    void addReservoir(Reservoir::_sp newReservoir);
    void addPipe(Pipe::_sp newPipe);
    void addPump(Pump::_sp newPump);
    void addValve(Valve::_sp newValve);
    void addCurve(Curve::_sp newCurve);
    void addDma(Dma::_sp dma);
    
    void removeNode(Node::_sp n);
    void removeLink(Link::_sp l);
    
    Link::_sp linkWithName(const string& name);
    Node::_sp nodeWithName(const string& name);
    vector<Element::_sp> elements();
    vector<Dma::_sp> dmas();
    vector<Node::_sp> nodes();
    vector<Link::_sp> links();
    vector<Junction::_sp> junctions();
    vector<Tank::_sp> tanks();
    vector<Reservoir::_sp> reservoirs();
    vector<Pipe::_sp> pipes();
    vector<Pump::_sp> pumps();
    vector<Valve::_sp> valves();
    vector<Curve::_sp> curves();
    
    virtual void updateEngineWithElementProperties(Element::_sp e);
    
    // simulation properties
    virtual void setHydraulicTimeStep(int seconds);
    int hydraulicTimeStep();
    
    virtual void setReportTimeStep(int seconds);
    int reportTimeStep();
    
    virtual void setQualityTimeStep(int seconds);
    int qualityTimeStep();
    
    void setInitialJunctionUniformQuality(double qual);
    double initialUniformQuality();
    void setInitialJunctionQualityFromMeasurements(time_t time);
    virtual void applyInitialQuality() { };
    virtual void applyInitialTankLevels() { };
    vector<Node::_sp> nearestNodes(Node::_sp junc, double maxDistance);

    virtual time_t currentSimulationTime();
    TimeSeries::_sp iterations() {return _iterations;}
    TimeSeries::_sp relativeError() {return _relativeError;}
    TimeSeries::_sp convergence() {return _convergence; }
    
    void setTankResetClock(Clock::_sp resetClock);
    
    void setTanksNeedReset(bool reset);
    bool tanksNeedReset();
        
    virtual std::ostream& toStream(std::ostream &stream);

    void setRecordForDmaDemands(PointRecord::_sp record);
    void setRecordForSimulationStats(PointRecord::_sp record);
    TimeSeries::_sp heartbeat();
    
    
    // specify records for certain states or inputs
    typedef enum {
      ElementOptionNone               = 0,
      ElementOptionMeasuredAll        = 1 << 0, // setting for pre-fetch record
      ElementOptionMeasuredTanks      = 1 << 1,
      ElementOptionMeasuredFlows      = 1 << 2,
      ElementOptionMeasuredPressures  = 1 << 3,
      ElementOptionMeasuredQuality    = 1 << 4,
      ElementOptionMeasuredSettings   = 1 << 5,
      ElementOptionMeasuredStatuses   = 1 << 6,
      ElementOptionAllTanks           = 1 << 7,
      ElementOptionAllFlows           = 1 << 8,
      ElementOptionAllPressures       = 1 << 9,
      ElementOptionAllHeads           = 1 << 10,
      ElementOptionAllQuality         = 1 << 11
    } elementOption_t;
    
//    void setRecordForElementInputs(PointRecord::_sp record);
//    void setRecordForElementOutput(PointRecord::_sp record, elementOption_t options);
    
//    vector<TimeSeries::_sp> networkStatesWithOptions(elementOption_t options);
//    vector<TimeSeries::_sp> networkInputSeries(elementOption_t options);
//    set<TimeSeries::_sp> networkInputRootSeries(elementOption_t options);

    // fetch points for a group of series
//    void fetchElementInputs(TimeRange range);
    
    // units
    Units flowUnits();
    Units headUnits();
    Units pressureUnits();
    Units qualityUnits();
    Units volumeUnits();
    
    void setFlowUnits(Units units);
    void setHeadUnits(Units units);
    void setPressureUnits(Units units);
    void setQualityUnits(Units units);
    void setVolumeUnits(Units units);
    
    
    void setSimulationLoggingCallback(RTX_Logging_Callback_Block);
    RTX_Logging_Callback_Block simulationLoggingCallback();
    
    void setWillSimulateCallback(std::function<void(time_t)> cb);
    void setDidSimulateCallback(std::function<void(time_t)> cb);
    
    std::map<std::string,std::string> dmaNameHashes;
        
    void setSimulationParameters(time_t time);
    void fetchSimulationStates();
    void saveNetworkStates(time_t time, std::set<PointRecord::_sp> bulkOperationRecords);
    
    
    
    // model parameter setting
    // recreating or wrapping basic api functionality here.
    virtual double reservoirLevel(const string& reservoirName) { return 0; };
    virtual double tankLevel(const string& tankName) { return 0; };
    virtual double tankVolume(const std::string& tank) { return 0; };
    virtual double tankFlow(const std::string& tank) { return 0; };
    virtual double junctionHead(const string& junction) { return 0; };
    virtual double junctionPressure(const string& junction) { return 0; };
    virtual double junctionDemand(const string& junctionName) { return 0; };
    virtual double junctionQuality(const string& junctionName) { return 0; };
    virtual double tankInletQuality(const string& tankName) { return 0; };
    
    // link elements
    virtual double pipeFlow(const string& pipe) { return 0; };
    virtual double pipeSetting(const string& pipe) {return 0;};
    virtual double pipeStatus(const string& pipe) {return 0;};
    virtual double pipeEnergy(const string& pipe) { return 0; };
    
    virtual void setReservoirHead(const string& reservoir, double level) { };
    virtual void setReservoirQuality(const string& reservoir, double quality) { };
    virtual void setTankLevel(const string& tank, double level) { };
    virtual void setJunctionDemand(const string& junction, double demand) { };
    virtual void setJunctionQuality(const string& junction, double quality) { };
    
    enum enableControl_t : bool {enable=true,disable=false};
    virtual void setPipeStatus(const string& pipe, Pipe::status_t status) { };
    virtual void setPipeStatusControl(const string& pipe, Pipe::status_t status, enableControl_t) { };
    virtual void setPumpStatus(const string& pump, Pipe::status_t status) { };
    virtual void setPumpStatusControl(const string& pump, Pipe::status_t status, enableControl_t) { };
    virtual void setPumpSetting(const std::string& pump, double setting) { };
    virtual void setPumpSettingControl(const std::string& pump, double setting, enableControl_t) { };
    virtual void setValveSetting(const string& valve, double setting) { };
    virtual void setValveSettingControl(const string& valve, double setting, enableControl_t) { };

  protected:
    
    virtual bool solveSimulation(time_t time) { return false; };
    virtual time_t nextHydraulicStep(time_t time) { return 0; };
    virtual void stepSimulation(time_t time) { };
    virtual int iterations(time_t time) { return 0; };
    virtual double relativeError(time_t time) { return 0; };
    
    virtual void setCurrentSimulationTime(time_t time);
    
    double nodeDirectDistance(Node::_sp n1, Node::_sp n2);
    double toRadians(double degrees);
    
    void logLine(const std::string& line);
    
    string _modelFile;
    
  private:
    void initObj();
    string _name;
    bool _shouldRunWaterQuality;
    bool _tanksNeedReset;
    void _checkTanksForReset(time_t time);
    // master list access
    void add(Junction::_sp newJunction);
    void add(Pipe::_sp newPipe);
    std::set<PointRecord::_sp> _recordsForModeledStates;
    
    
    // element lists
    // master node/link lists
//    std::vector<Node::_sp> _nodes;
//    std::vector<Link::_sp> _links;
    std::map<string, Node::_sp> _nodes;
    std::map<string, Link::_sp> _links;
    // convenience lists for iterations
    vector<Element::_sp> _elements;
    vector<Junction::_sp> _junctions;
    vector<Tank::_sp> _tanks;
    vector<Reservoir::_sp> _reservoirs;
    vector<Pipe::_sp> _pipes;
    vector<Pump::_sp> _pumps;
    vector<Valve::_sp> _valves;
    vector<Curve::_sp> _curves;
    vector<Dma::_sp> _dmas;
    vector<Pipe::_sp> _dmaPipesToIgnore;
    bool _dmaShouldDetectClosedLinks;
    
    Clock::_sp _regularMasterClock, _simReportClock;
    TimeSeries::_sp _relativeError, _iterations, _convergence, _heartbeat, _simWallTime, _saveWallTime, _filterWallTime;
    Clock::_sp _tankResetClock;
    int _qualityTimeStep;
    bool _doesOverrideDemands;
    bool _shouldCancelSimulation;
    time_t _currentSimulationTime;
    Units _flowUnits, _headUnits, _pressureUnits, _qualityUnits, _volumeUnits;
    std::mutex _simulationInProcessMutex;
    double _initialQuality;
    RTX_Logging_Callback_Block _simLogCallback;
    std::function<void(time_t)> _didSimulateCallback, _willSimulateCallback;
    std::future<void> _saveStateFuture;
    std::string _projectionString;
    
  };
  
  std::ostream& operator<< (std::ostream &out, Model &model);

}


#endif
