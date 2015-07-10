//
//  Model.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  


#include <iostream>
#include <set>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include "Model.h"
#include "Units.h"

#include "DbPointRecord.h"


#include <boost/config.hpp>
#include <vector>
#include <algorithm>
#include <utility>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/connected_components.hpp>
#include <boost/range/adaptors.hpp>

#include <boost/interprocess/sync/scoped_lock.hpp>
using boost::signals2::mutex;
using boost::interprocess::scoped_lock;

using namespace RTX;
using namespace std;

bool _rtxmodel_isDbRecord(PointRecord::_sp record);


Model::Model() : _flowUnits(1), _headUnits(1), _pressureUnits(1) {
  // default reset clock is 24 hours, default reporting clock is 1 hour.
  // TODO -- configure boundary reset clock in a smarter way?
  _regularMasterClock.reset( new Clock(3600) );
  _tankResetClock = Clock::_sp();
  _simReportClock = Clock::_sp();
  _relativeError.reset( new TimeSeries() );
  _iterations.reset( new TimeSeries() );
  _convergence.reset( new TimeSeries() );
  
  _relativeError->setName("Relative Error");
  _iterations->setName("Iterations");
  _convergence->setName("Convergence Status");
  _doesOverrideDemands = false;
  _shouldRunWaterQuality = false;
  
  _dmaShouldDetectClosedLinks = false;
  _dmaPipesToIgnore = vector<Pipe::_sp>();
  
  // defaults
  setFlowUnits(RTX_LITER_PER_SECOND);
  setPressureUnits(RTX_PASCAL);
  setHeadUnits(RTX_METER);
  _name = "Model";
  _shouldCancelSimulation = false;
  _tanksNeedReset = false;
}
Model::~Model() {
  
}
std::ostream& RTX::operator<< (std::ostream &out, Model &model) {
  return model.toStream(out);
}




string Model::name() {
  return _name;
}

void Model::setName(string name) {
  _name = name;
}

void Model::loadModelFromFile(const std::string& filename) throw(std::exception) {
  _modelFile = filename;
}

std::string Model::modelFile() {
  return _modelFile;
}

bool Model::shouldRunWaterQuality() {
  return _shouldRunWaterQuality;
}

void Model::setShouldRunWaterQuality(bool run) {
  _shouldRunWaterQuality = run;
}


#pragma mark - Units
Units Model::flowUnits()    {
  return _flowUnits;
}
Units Model::pressureUnits()    {
  return _pressureUnits;
}
Units Model::headUnits()    {
  return _headUnits;
}
Units Model::qualityUnits() {
  return _qualityUnits;
}
Units Model::volumeUnits() {
  return _volumeUnits;
}

void Model::setFlowUnits(Units units)    {
  if (!units.isSameDimensionAs(RTX_LITER_PER_SECOND)) {
    cerr << "units not dimensionally consistent with flow" << endl;
    return;
  }
  _flowUnits = units;
  BOOST_FOREACH(Junction::_sp j, this->junctions()) {
    j->demand()->setUnits(units);
  }
  BOOST_FOREACH(Tank::_sp t, this->tanks()) {
    t->flowMeasure()->setUnits(units);
    t->flow()->setUnits(units);
  }
  BOOST_FOREACH(Pipe::_sp p, this->pipes()) {
    p->flow()->setUnits(units);
  }
  BOOST_FOREACH(Pump::_sp p, this->pumps()) {
    p->flow()->setUnits(units);
  }
  BOOST_FOREACH(Valve::_sp v, this->valves()) {
    v->flow()->setUnits(units);
  }
}
void Model::setHeadUnits(Units units)    {
  _headUnits = units;
  
  BOOST_FOREACH(Junction::_sp j, this->junctions()) {
    j->head()->setUnits(units);
  }
  BOOST_FOREACH(Tank::_sp t, this->tanks()) {
    t->head()->setUnits(units);
    t->level()->setUnits(units);
  }
  BOOST_FOREACH(Reservoir::_sp r, this->reservoirs()) {
    r->head()->setUnits(units);
  }
  
}
void Model::setPressureUnits(Units units) {
  _pressureUnits = units;
  BOOST_FOREACH(Junction::_sp j, this->junctions()) {
    j->pressure()->setUnits(units);
  }
}
void Model::setQualityUnits(Units units) {
  _qualityUnits = units;
  BOOST_FOREACH(Junction::_sp j, this->junctions()) {
    j->quality()->setUnits(units);
  }
  BOOST_FOREACH(Tank::_sp t, this->tanks()) {
    t->quality()->setUnits(units);
  }
  BOOST_FOREACH(Reservoir::_sp r, this->reservoirs()) {
    r->quality()->setUnits(units);
  }
  
}

void Model::setVolumeUnits(RTX::Units units) {
  _volumeUnits = units;
  BOOST_FOREACH(Tank::_sp t, this->tanks()) {
    t->volume()->setUnits(units);
  }
}

#pragma mark - Storage
#pragma mark - Storage


void Model::setRecordForDmaDemands(PointRecord::_sp record) {
  
  BOOST_FOREACH(Dma::_sp dma, dmas()) {
    dma->setRecord(record);
  }
  
}

void Model::setRecordForSimulationStats(PointRecord::_sp record) {
  
  _relativeError->setRecord(record);
  _iterations->setRecord(record);
  _convergence->setRecord(record);
  
}


set<PointRecord::_sp> Model::recordsForModeledStates() {
  set<PointRecord::_sp> stateRecordsUsed;
  BOOST_FOREACH(Junction::_sp e, this->junctions()) {
    vector<PointRecord::_sp> elementRecords;
    elementRecords.push_back(e->pressure()->record());
    elementRecords.push_back(e->head()->record());
    elementRecords.push_back(e->quality()->record());
    elementRecords.push_back(e->demand()->record());
    
    BOOST_FOREACH(PointRecord::_sp r, elementRecords) {
      if (_rtxmodel_isDbRecord(r)) {
        stateRecordsUsed.insert(r);
      }
    }
  }
  BOOST_FOREACH(Tank::_sp t, this->tanks()) {
    vector<PointRecord::_sp> recVec;
    recVec.push_back(t->level()->record());
    recVec.push_back(t->flow()->record());
    BOOST_FOREACH(PointRecord::_sp r, recVec) {
      if (_rtxmodel_isDbRecord(r)) {
        stateRecordsUsed.insert(r);
      }
    }
  }
  BOOST_FOREACH(Pipe::_sp p, this->pipes()) {
    vector<PointRecord::_sp> recVec;
    recVec.push_back(p->flow()->record());
    BOOST_FOREACH(PointRecord::_sp r, recVec) {
      if (_rtxmodel_isDbRecord(r)) {
        stateRecordsUsed.insert(r);
      }
    }
  }
  BOOST_FOREACH(Valve::_sp p, this->valves()) {
    vector<PointRecord::_sp> recVec;
    recVec.push_back(p->flow()->record());
    BOOST_FOREACH(PointRecord::_sp r, recVec) {
      if (_rtxmodel_isDbRecord(r)) {
        stateRecordsUsed.insert(r);
      }
    }
  }
  BOOST_FOREACH(Pump::_sp p, this->pumps()) {
    vector<PointRecord::_sp> recVec;
    recVec.push_back(p->flow()->record());
    BOOST_FOREACH(PointRecord::_sp r, recVec) {
      if (_rtxmodel_isDbRecord(r)) {
        stateRecordsUsed.insert(r);
      }
    }
  }
  
  return stateRecordsUsed;

}


#pragma mark - Demand dmas


void Model::initDMAs() {
  
  _dmas.clear();
  
  set<Pipe::_sp> boundaryPipes;
  
  using namespace boost;
  std::map<Node::_sp, int> nodeIndexMap;
  boost::adjacency_list <boost::vecS, boost::vecS, boost::undirectedS> G;
  
  typedef struct {
    Link::_sp link;
    int fromIdx, toIdx;
  } LinkDescriptor;
  
  // load up the link descriptor lookup table
  vector<LinkDescriptor> linkDescriptors;
  {
    int iNode = 0;
    BOOST_FOREACH(Node::_sp node, _nodes | boost::adaptors::map_values) {
      nodeIndexMap[node] = iNode;
      ++iNode;
    }
    BOOST_FOREACH(Link::_sp link, _links | boost::adaptors::map_values) {
      int from = nodeIndexMap[link->from()];
      int to = nodeIndexMap[link->to()];
      linkDescriptors.push_back((LinkDescriptor){link, from, to});
    }
  }
  
  //
  // build a boost graph of the network, ignoring links that are dma boundaries or explicitly ignored
  // is this faster than adapting the model to be BGL compliant? certainly faster dev time, but this should be tested in code.
  //
  vector<Pipe::_sp> ignorePipes = this->dmaPipesToIgnore();
  
  BOOST_FOREACH(Link::_sp link, _links | boost::adaptors::map_values) {
    Pipe::_sp pipe = boost::static_pointer_cast<Pipe>(link);
    // flow measure?
    if (pipe->flowMeasure()) {
      boundaryPipes.insert(pipe);
      continue;
    }
    // pipe closed? (and not a pump)
    if (pipe->fixedStatus() == Pipe::CLOSED && pipe->type() != Element::PUMP) {
      boundaryPipes.insert(pipe);
      continue;
    }
    // pipe ignored?
    if (find(ignorePipes.begin(), ignorePipes.end(), pipe) != ignorePipes.end()) {
      continue;
    }
    
    int from = nodeIndexMap[link->from()];
    int to = nodeIndexMap[link->to()];
    add_edge(from, to, G); // BGL add edge to graph
  }
  
  // use the BGL to find the number of connected components and get the membership list
  vector<int> componentMap(_nodes.size());
  int nDmas = connected_components(G, &componentMap[0]);
  
  // for each connected component, create a new dma.
  vector<Dma::_sp> newDmas;
  for (int iDma = 0; iDma < nDmas; ++iDma) {
    stringstream dmaNameStream("");
    dmaNameStream << "dma " << iDma;
    Dma::_sp dma( new Dma(dmaNameStream.str()) );
    newDmas.push_back(dma);
  }
  
  // now that we have the dma list, go through the node membership and add each node object to the appropriate dma.
  vector<Node::_sp> indexedNodes;
  indexedNodes.reserve(_nodes.size());
  BOOST_FOREACH(Node::_sp j, _nodes | boost::adaptors::map_values) {
    indexedNodes.push_back(j);
  }
  
  for (int nodeIdx = 0; nodeIdx < _nodes.size(); ++nodeIdx) {
    int dmaIdx = componentMap[nodeIdx];
    Dma::_sp dma = newDmas[dmaIdx];
    dma->addJunction(boost::static_pointer_cast<Junction>(indexedNodes[nodeIdx]));
  }
  
  // finally, let the dma assemble its aggregators
  BOOST_FOREACH(const Dma::_sp dma, newDmas) {
    dma->initDemandTimeseries(boundaryPipes);
    dma->demand()->setUnits(this->flowUnits());
    //cout << "adding dma: " << *dma << endl;
    this->addDma(dma);
  }
  
  
}


void Model::setDmaShouldDetectClosedLinks(bool detect) {
  _dmaShouldDetectClosedLinks = detect;
}

bool Model::dmaShouldDetectClosedLinks() {
  return _dmaShouldDetectClosedLinks;
}

void Model::setDmaPipesToIgnore(vector<Pipe::_sp> ignorePipes) {
  _dmaPipesToIgnore = ignorePipes;
}

vector<Pipe::_sp> Model::dmaPipesToIgnore() {
  return _dmaPipesToIgnore;
}

#pragma mark - Controls

void Model::overrideControls() throw(RtxException) {
  _doesOverrideDemands = true;
}

#pragma mark - Element Accessors

void Model::addJunction(Junction::_sp newJunction) {
  if (!newJunction) {
    std::cerr << "junction not specified" << std::endl;
    return;
  }
  _junctions.push_back(newJunction);
  add(newJunction);
}
void Model::addTank(Tank::_sp newTank) {
  if (!newTank) {
    std::cerr << "tank not specified" << std::endl;
    return;
  }
  _tanks.push_back(newTank);
  add(newTank);
}
void Model::addReservoir(Reservoir::_sp newReservoir) {
  if (!newReservoir) {
    std::cerr << "reservoir not specified" << std::endl;
    return;
  }
  _reservoirs.push_back(newReservoir);
  add(newReservoir);
}
void Model::addPipe(Pipe::_sp newPipe) {
  if (!newPipe) {
    std::cerr << "pipe not specified" << std::endl;
    return;
  }
  _pipes.push_back(newPipe);
  add(newPipe);
}
void Model::addPump(Pump::_sp newPump) {
  if (!newPump) {
    std::cerr << "pump not specified" << std::endl;
    return;
  }
  _pumps.push_back(newPump);
  add(newPump);
}
void Model::addValve(Valve::_sp newValve) {
  if (!newValve) {
    std::cerr << "valve not specified" << std::endl;
    return;
  }
  _valves.push_back(newValve);
  add(newValve);
}

void Model::addDma(Dma::_sp dma) {
  _dmas.push_back(dma);
  dma->setJunctionFlowUnits(this->flowUnits());
  Clock::_sp hydClock(new Clock(hydraulicTimeStep()));
  //dma->demand()->setClock(hydClock);
  
}

// add to master lists
void Model::add(Junction::_sp newJunction) {
  _nodes[newJunction->name()] = newJunction;
  _elements.push_back(newJunction);
}
void Model::add(Pipe::_sp newPipe) {
  // manually add the pipe to the nodes' lists.
  newPipe->from()->addLink(newPipe);
  newPipe->to()->addLink(newPipe);
  // add to master link and element lists.
  _links[newPipe->name()] = newPipe;
  _elements.push_back(newPipe);
}

Link::_sp Model::linkWithName(const string& name) {
  if ( _links.find(name) == _links.end() ) {
    Link::_sp emptyLink;
    return emptyLink;
  }
  else {
    return _links[name];
  }
}
Node::_sp Model::nodeWithName(const string& name) {
  if ( _nodes.find(name) == _nodes.end() ) {
    Node::_sp emptyNode;
    return emptyNode;
  }
  else {
    return _nodes[name];
  }
}

std::vector<Element::_sp> Model::elements() {
  return _elements;
}

std::vector<Node::_sp> Model::nodes() {
  std::vector<Node::_sp> nodes;
  BOOST_FOREACH(Node::_sp node, _nodes | boost::adaptors::map_values) {
    nodes.push_back(node);
  }
  return nodes;
}

std::vector<Link::_sp> Model::links() {
  std::vector<Link::_sp> links;
  BOOST_FOREACH(Link::_sp link, _links | boost::adaptors::map_values) {
    links.push_back(link);
  }
  return links;
}

std::vector<Dma::_sp> Model::dmas() {
  return _dmas;
}
std::vector<Junction::_sp> Model::junctions() {
  return _junctions;
}
std::vector<Tank::_sp> Model::tanks() {
  return _tanks;
}
std::vector<Reservoir::_sp> Model::reservoirs() {
  return _reservoirs;
}
std::vector<Pipe::_sp> Model::pipes() {
  return _pipes;
}
std::vector<Pump::_sp> Model::pumps() {
  return _pumps;
}
std::vector<Valve::_sp> Model::valves() {
  return _valves;
}



#pragma mark - Publicly Accessible Simulation Methods

void Model::runSinglePeriod(time_t time) {
  cerr << "whoops, not implemented" << endl;
  
  time_t lastSimulationPeriod, boundaryReset, start;
  // to run a single period, we need boundary conditions.
  // so back up to either the most recent set of results, or the most recent boundary-reset event
  // (whichever is nearer) and simulate through the requested time.
  //lastSimulationPeriod = _masterClock->validTime(time);
  boundaryReset = _tankResetClock->validTime(time);
  start = RTX_MAX(lastSimulationPeriod, boundaryReset);
  
  // run the simulation to the requested time.
  runExtendedPeriod(start, time);
}

void Model::runExtendedPeriod(time_t start, time_t end) {
  TimeRange range(start,end);
  time_t simulationTime = range.start;
  time_t nextClockTime = range.start;
  time_t nextReportTime = range.start;
  time_t nextSimulationTime = range.start;
  time_t nextResetTime = range.start;
  time_t stepToTime = range.start;
  struct tm * timeinfo;
  bool success;
  
  {
    // of course we don't want to cancel - we just started!
    scoped_lock<boost::signals2::mutex> l(_simulationInProcessMutex);
    _shouldCancelSimulation = false;
  }
  
  // get the record(s) being used
  set<PointRecord::_sp> stateRecordsUsed = this->recordsForModeledStates();
  
//  // pre-fetch simulation input data for entire period -- testing only
//  string scratchPath = "/var/tmp/RTX_scratch.sqlite";
//  SqlitePointRecord::_sp scratchdb( new SqlitePointRecord());
//  scratchdb->setPath(scratchPath);
//  scratchdb->dbConnect();
//  scratchdb->truncate();
//  this->setRecordForElementInputs(scratchdb);
//  this->fetchElementInputs(range);

  // Extended period simulation
  while (simulationTime < range.end) {
    
    {
      scoped_lock<boost::signals2::mutex> l(_simulationInProcessMutex);
      if (_shouldCancelSimulation) {
        break;
      }
    }
    
    // get parameters from the RTX elements, and pull them into the simulation
    setSimulationParameters(simulationTime);
    
    // simulate this period, find the next timestep boundary.
    success = solveSimulation(simulationTime);
    
    // save simulation stats here so we can track convergence issues
    Point error(simulationTime, relativeError(simulationTime));
    _relativeError->insert(error);
    Point iterationCount(simulationTime, iterations(simulationTime));
    _iterations->insert(iterationCount);
    Point convergenceStatus(simulationTime, success);
    _convergence->insert(convergenceStatus);
    
    if (success) {
      // tell each element to update its derived states (simulation-computed values)
      if (!_simReportClock || _simReportClock->isValid(simulationTime)) {
        BOOST_FOREACH(PointRecord::_sp r, stateRecordsUsed) {
          r->beginBulkOperation();
        }
        saveNetworkStates(simulationTime);
        BOOST_FOREACH(PointRecord::_sp r, stateRecordsUsed) {
          r->endBulkOperation();
        }
      }
      // get time to next simulation period
      nextSimulationTime = nextHydraulicStep(simulationTime);
      nextClockTime = _regularMasterClock->timeAfter(simulationTime);
      nextReportTime = (_simReportClock) ? _simReportClock->timeAfter(simulationTime) : nextSimulationTime;
      if ( _tankResetClock ) {
        nextResetTime = _tankResetClock->timeAfter(simulationTime);
      }
      else {
        nextResetTime = nextSimulationTime;
      }
      stepToTime = min( min( min( nextClockTime, nextSimulationTime ), nextReportTime ), nextResetTime);
      
      // and step the simulation to that time.
      stepSimulation(stepToTime);
      simulationTime = currentSimulationTime();
      
      timeinfo = localtime (&simulationTime);
      cout << "Simulation time :: " << asctime(timeinfo) << endl;
    }
    else {
      // simulation failed -- advance the time and reset tank levels
      nextClockTime = _regularMasterClock->timeAfter(simulationTime);
      simulationTime = nextClockTime;
      setCurrentSimulationTime(simulationTime);
      cout << "will reset tanks" << endl;
      this->setTanksNeedReset(true);
    }
    
  } // simulation while-loop
  
  {
    scoped_lock<boost::signals2::mutex> l(_simulationInProcessMutex);
    _shouldCancelSimulation = false;
  }
}

/**
 @brief Run a Forecasted simulation.
 @param start The start time
 @param end The end time
 
 This method will re-enable any Rules or Controls that would otherwise be disabled during realtime (or retrospective) analysis. The simulation PointRecord destination will be changed for participating elements to the forecasting record.
 
 If it is desired for the tank levels to be reset for the start of the forecast, then be sure to call tanksNeedReset() on the Model object before calling this method. Otherwise the previous tank levels (as simulated) will be used to begin the forecast simulation.
 
 */
void Model::runForecast(time_t start, time_t end) {
  
  time_t simulationTime = start;
  time_t nextClockTime = start;
  time_t nextReportTime = start;
  time_t nextSimulationTime = start;
  time_t nextResetTime = start;
  time_t stepToTime = start;
  struct tm * timeinfo;
  bool success;
  
  
  this->enableControls();
  
  // get the record(s) being used
  set<PointRecord::_sp> stateRecordsUsed = this->recordsForModeledStates();
  
  while (simulationTime < end) {
    
    // get parameters from the RTX elements, and pull them into the simulation
    setSimulationParameters(simulationTime);
    
    // simulate this period, find the next timestep boundary.
    success = solveSimulation(simulationTime);
    
    // save simulation stats here so we can track convergence issues
    Point error(simulationTime, relativeError(simulationTime));
    _relativeError->insert(error);
    Point iterationCount(simulationTime, iterations(simulationTime));
    _iterations->insert(iterationCount);
    Point convergenceStatus(simulationTime, success);
    _convergence->insert(convergenceStatus);
    
    if (success) {
      // tell each element to update its derived states (simulation-computed values)
      if (!_simReportClock || _simReportClock->isValid(simulationTime)) {
        BOOST_FOREACH(PointRecord::_sp r, stateRecordsUsed) {
          r->beginBulkOperation();
        }
        saveNetworkStates(simulationTime);
        BOOST_FOREACH(PointRecord::_sp r, stateRecordsUsed) {
          r->endBulkOperation();
        }
      }
      // get time to next simulation period
      nextSimulationTime = nextHydraulicStep(simulationTime);
      nextClockTime = _regularMasterClock->timeAfter(simulationTime);
      nextReportTime = (_simReportClock) ? _simReportClock->timeAfter(simulationTime) : nextSimulationTime;
      if ( _tankResetClock ) {
        nextResetTime = _tankResetClock->timeAfter(simulationTime);
      }
      else {
        nextResetTime = nextSimulationTime;
      }
      stepToTime = min( min( min( nextClockTime, nextSimulationTime ), nextReportTime ), nextResetTime);
      
      // and step the simulation to that time.
      stepSimulation(stepToTime);
      simulationTime = currentSimulationTime();
      
      timeinfo = localtime (&simulationTime);
      cout << "Simulation time :: " << asctime(timeinfo) << endl;
    }
    else {
      // simulation failed -- advance the time and reset tank levels
      nextClockTime = _regularMasterClock->timeAfter(simulationTime);
      simulationTime = nextClockTime;
      setCurrentSimulationTime(simulationTime);
      cout << "will reset tanks" << endl;
      this->setTanksNeedReset(true);
    }
    
  } // simulation loop
  
  
}


void Model::cancelSimulation() {
  scoped_lock<boost::signals2::mutex> l(_simulationInProcessMutex);
  _shouldCancelSimulation = true;
}

void Model::setReportTimeStep(int seconds) {
  _simReportClock.reset( new Clock(seconds) );
}

int Model::reportTimeStep() {
  return _simReportClock->period();
}


void Model::setHydraulicTimeStep(int seconds) {
  _regularMasterClock.reset( new Clock(seconds) );
}

int Model::hydraulicTimeStep() {
  return _regularMasterClock->period();
}

void Model::setQualityTimeStep(int seconds) {
  _qualityTimeStep = seconds;
}

int Model::qualityTimeStep() {
  return _qualityTimeStep;
}

void Model::setInitialJunctionUniformQuality(double qual) {
  // Constant initial quality of Junctions and Tanks (Reservoirs are boundary conditions)
  BOOST_FOREACH(Junction::_sp junc, this->junctions()) {
    junc->setInitialQuality(qual);
  }
  BOOST_FOREACH(Tank::_sp tank, this->tanks()) {
    tank->setInitialQuality(qual);
  }
}

void Model::setTankResetClock(Clock::_sp resetClock) {
  _tankResetClock = resetClock;
}

bool Model::tanksNeedReset() {
  return _tanksNeedReset;
}

void Model::setTanksNeedReset(bool reset) {
  _tanksNeedReset = reset;
  
  BOOST_FOREACH(Tank::_sp tank, this->tanks()) {
    tank->setNeedsReset(reset);
  }
  
}

void Model::_checkTanksForReset(time_t time) {
  
  if (!tanksNeedReset()) {
    return;
  }
  
  BOOST_FOREACH(Tank::_sp tank, this->tanks()) {
    if (tank->levelMeasure()) {
      Point p = tank->levelMeasure()->pointAtOrBefore(time);
      if (p.isValid) {
        double levelValue = Units::convertValue(p.value, tank->levelMeasure()->units(), headUnits());
        // adjust for model limits (epanet rejects otherwise, for example)
        levelValue = (levelValue <= tank->maxLevel()) ? levelValue : tank->maxLevel();
        levelValue = (levelValue >= tank->minLevel()) ? levelValue : tank->minLevel();
        setTankLevel(tank->name(), levelValue);
      }
      else {
        cerr << "ERR: Invalid head point for Tank " << tank->name() << " at time " << time << endl;
      }
    }
  }
  
  this->setTanksNeedReset(false);
  
}

void Model::setInitialJunctionQualityFromMeasurements(time_t time) {
  // Measured initial quality of Junctions and Tanks (Reservoirs are boundary conditions)
  // using nearest neighbor interpolation of quality measurements
  
  // junction measurements
  std::vector< std::pair<Junction::_sp, double> > measuredJunctions;
  BOOST_FOREACH(Junction::_sp junc, this->junctions()) {
    if (junc->qualityMeasure()) {
      TimeSeries::_sp qualityTS = junc->qualityMeasure();
      Point aPoint = qualityTS->pointAtOrBefore(time);
      if (aPoint.isValid) {
        measuredJunctions.push_back(make_pair(junc, aPoint.value));
      }
    }
  }
  // tank measurements
  BOOST_FOREACH(Tank::_sp tank, this->tanks()) {
    if (tank->qualityMeasure()) {
      TimeSeries::_sp qualityTS = tank->qualityMeasure();
      Point aPoint = qualityTS->pointAtOrBefore(time);
      if (aPoint.isValid) {
        measuredJunctions.push_back(make_pair(tank, aPoint.value));
      }
    }
  }
  
  // Nearest neighbor interpolation by enumeration
  // junctions
  BOOST_FOREACH(Junction::_sp junc, this->junctions()) {
    std::pair<Junction::_sp, double> mjunc;
    double minDistance = DBL_MAX;
    double initQuality = 0;
    BOOST_FOREACH(mjunc, measuredJunctions) {
      double d = nodeDirectDistance(junc, mjunc.first);
      if (d < minDistance) {
        minDistance = d;
        initQuality = mjunc.second;
      }
    }
    // initialize the junction quality
    junc->setInitialQuality(initQuality);
//    cout << "Junction " << junc->name() << " Quality: " << initQuality << endl;
  }
  // tanks
  BOOST_FOREACH(Tank::_sp tank, this->tanks()) {
    std::pair<Junction::_sp, double> mjunc;
    double minDistance = DBL_MAX;
    double initQuality = 0;
    BOOST_FOREACH(mjunc, measuredJunctions) {
      double d = nodeDirectDistance(tank, mjunc.first);
      if (d < minDistance) {
        minDistance = d;
        initQuality = mjunc.second;
      }
    }
    // initialize the tank quality
    tank->setInitialQuality(initQuality);
//    cout << "Tank " << tank->name() << " Quality: " << initQuality << endl;
  }
  
}

std::vector<Node::_sp> Model::nearestNodes(Node::_sp node, double maxDistance) {
  // simple enumeration -- max distance in meters
  vector<Node::_sp> nodeList;
  BOOST_FOREACH(Node::_sp n, _nodes | boost::adaptors::map_values) {
    if (nodeDirectDistance(n, node) <= maxDistance) {
      nodeList.push_back(n);
    }
  }

  return nodeList;
}


#pragma mark - Protected Methods

std::ostream& Model::toStream(std::ostream &stream) {
  // output shall be as follows:
  /*
   
   Model Properties:
      ## nodes (## tanks, ## reservoirs) in ## dmas
      ## links (## pumps, ## valves)
   Time Steps:
      ##s (hydraulic), ##s (quality)
   State storage:
   [point record properties]
   
   */
  
  stream << "Model Properties" << endl;
  stream << "\t" << _nodes.size() << " nodes (" << _tanks.size() << " tanks, " << _reservoirs.size() << " reservoirs ) in " << _dmas.size() << " dmas" << endl;
  stream << "\t" << _links.size() << " links (" << _pumps.size() << " pumps, " << _valves.size() << " valves)" << endl;
  stream << "Time Steps:" << endl;
  stream << "\t" << hydraulicTimeStep() << "s (hydraulic), " << qualityTimeStep() << "s (quality)" << endl;
//  if (_record) {
//    stream << "State Storage:" << endl << *_record;
//  }
  
  if (_dmas.size() > 0) {
    stream << "Demand DMAs:" << endl;
    BOOST_FOREACH(Dma::_sp d, _dmas) {
      stream << "## DMA: " << d->name() << endl;
      stream << *d << endl;
    }
  }
  
  
  return stream;
}

void Model::setSimulationParameters(time_t time) {
  // set all element parameters
  
  // allocate junction demands based on dmas, and set the junction demand values in the model.
  if (_doesOverrideDemands) {
    // by dma, insert demand point into each junction timeseries at the current simulation time
    BOOST_FOREACH(Dma::_sp dma, this->dmas()) {
      dma->allocateDemandToJunctions(time);
    }
    // hydraulic junctions - set demand values.
    BOOST_FOREACH(Junction::_sp junction, this->junctions()) {
      Point p = junction->demand()->pointAtOrBefore(time);
      if (p.isValid) {
        double demandValue = Units::convertValue(p.value, junction->demand()->units(), flowUnits());
        setJunctionDemand(junction->name(), demandValue);
      }
      else {
        // default when allocation doesn't/can't set demand -- should this happen?
        setJunctionDemand(junction->name(), 0.0);
      }
    }
  }
  
  // for reservoirs, set the boundary head and quality conditions
  BOOST_FOREACH(Reservoir::_sp reservoir, this->reservoirs()) {
    if (reservoir->boundaryHead()) {
      // get the head measurement parameter, and pass it through as a state.
      Point p = reservoir->boundaryHead()->pointAtOrBefore(time);
      if (p.isValid) {
        double headValue = Units::convertValue(p.value, reservoir->boundaryHead()->units(), headUnits());
        setReservoirHead( reservoir->name(), headValue );
      }
      else {
        cerr << "ERR: Invalid head point for Reservoir " << reservoir->name() << " at time " << time << endl;
      }
    }
    if (reservoir->boundaryQuality()) {
      // get the quality measurement parameter, and pass it through as a state.
      Point p = reservoir->boundaryQuality()->pointAtOrBefore(time);
      if (p.isValid) {
        double qualityValue = Units::convertValue(p.value, reservoir->boundaryQuality()->units(), qualityUnits());
        setReservoirQuality( reservoir->name(), qualityValue );
      }
      else {
        cerr << "ERR: Invalid quality point for Reservoir " << reservoir->name() << " at time " << time << endl;
      }
    }
  }
  
  // check for valid time with tank reset clock
  if (_tankResetClock && _tankResetClock->isValid(time)) {
    this->setTanksNeedReset(true);
  }
  
  _checkTanksForReset(time);

  // for valves, set status and setting
  BOOST_FOREACH(Valve::_sp valve, this->valves()) {
    // status can affect settings and vice-versa; status rules
    Pipe::status_t status = valve->fixedStatus();
    if (valve->statusParameter()) {
      Point p = valve->statusParameter()->pointAtOrBefore(time);
      if (p.isValid) {
        status = Pipe::status_t((int)(p.value));
        setPipeStatus( valve->name(), status );
      }
      else {
        cerr << "ERR: Invalid status point for Valve " << valve->name() << " at time " << time << endl;
      }
    }
    if (valve->settingParameter()) {
      if (status) {
        Point p = valve->settingParameter()->pointAtOrBefore(time);
        if (p.isValid) {
          // TODO -- set units based on type of valve (pressure or flow model units)
          setValveSetting( valve->name(), p.value );
        }
        else {
          cerr << "ERR: Invalid setting point for Valve " << valve->name() << " at time " << time << endl;
        }
      }
      else {
        cerr << "WARN: Ignoring setting for Valve " << valve->name() << " because status is Closed" << endl;
      }
    }
  }
  
  // for pumps, set status and setting
  BOOST_FOREACH(Pump::_sp pump, this->pumps()) {
    // status can affect settings and vice-versa; status rules
    Pipe::status_t status = pump->fixedStatus();
    if (pump->statusParameter()) {
      Point p = pump->statusParameter()->pointAtOrBefore(time);
      if (p.isValid) {
        status = Pipe::status_t((int)(p.value));
        setPumpStatus( pump->name(), status );
      }
      else {
        cerr << "ERR: Invalid status point for Pump " << pump->name() << " at time " << time << endl;
      }
    }
    if (pump->settingParameter()) {
      if (status == Pipe::OPEN) {
        Point p = pump->settingParameter()->pointAtOrBefore(time);
        if (p.isValid) {
          setPumpSetting( pump->name(), p.value );
        }
        else {
          cerr << "ERR: Invalid setting point for Pump " << pump->name() << " at time " << time << endl;
        }
      }
      else {
        cerr << "WARN: Ignoring setting for Pump " << pump->name() << " because status is Closed" << endl;
      }
    }
  }
  
  // for pipes, set status
  BOOST_FOREACH(Pipe::_sp pipe, this->pipes()) {
    if (pipe->statusParameter()) {
      Point p = pipe->statusParameter()->pointAtOrBefore(time);
      if (p.isValid) {
        setPipeStatus(pipe->name(), Pipe::status_t((int)(p.value)));
      }
      else {
        cerr << "ERR: Invalid status point for Pipe " << pipe->name() << " at time " << time << endl;
      }
    }
  }
  
  //////////////////////////////
  // water quality parameters //
  //////////////////////////////
  if (this->shouldRunWaterQuality()) {
    BOOST_FOREACH(Junction::_sp j, this->junctions()) {
      if (j->qualitySource()) {
        Point p = j->qualitySource()->pointAtOrBefore(time);
        if (p.isValid) {
          double quality = Units::convertValue(p.value, j->qualitySource()->units(), qualityUnits());
          setJunctionQuality(j->name(), quality);
        }
        else {
          cerr << "ERR: Invalid quality source point for junction " << j->name() << " at time " << time << endl;
        }
      }
    }
  }
  
  
  
}



void Model::saveNetworkStates(time_t time) {
  
  // retrieve results from the hydraulic sim 
  // then insert the state values into elements' time series.
  
  // junctions, tanks, reservoirs
  BOOST_FOREACH(Junction::_sp junction, junctions()) {
    double head;
    head = Units::convertValue(junctionHead(junction->name()), headUnits(), junction->head()->units());
    Point headPoint(time, head);
    junction->head()->insert(headPoint);
    junction->state_head = head;
    
    double pressure;
    pressure = Units::convertValue(junctionPressure(junction->name()), pressureUnits(), junction->pressure()->units());
    Point pressurePoint(time, pressure);
    junction->pressure()->insert(pressurePoint);
    junction->state_pressure = pressure;
    
    // todo - more fine-grained quality data? at wq step resolution...
    if (this->shouldRunWaterQuality()) {
      double quality;
      quality = Units::convertValue(junctionQuality(junction->name()), this->qualityUnits(), junction->quality()->units());
      Point qualityPoint(time, quality);
      junction->quality()->insert(qualityPoint);
      junction->state_quality = quality;
    }
  }
  
  // only save demand states if 
  if (!_doesOverrideDemands) {
    BOOST_FOREACH(Junction::_sp junction, junctions()) {
      double demand;
      demand = Units::convertValue(junctionDemand(junction->name()), flowUnits(), junction->demand()->units());
      Point demandPoint(time, demand);
      junction->demand()->insert(demandPoint);
      junction->state_demand = demand;
    }
  }
  
  BOOST_FOREACH(Reservoir::_sp reservoir, reservoirs()) {
    double head;
    head = Units::convertValue(junctionHead(reservoir->name()), headUnits(), reservoir->head()->units());
    Point headPoint(time, head);
    reservoir->head()->insert(headPoint);
    reservoir->state_head = head;
    
    double quality;
    quality = Units::convertValue(junctionQuality(reservoir->name()), this->qualityUnits(), reservoir->quality()->units());
    Point qualityPoint(time, quality);
    reservoir->quality()->insert(qualityPoint);
    reservoir->state_quality = quality;
  }
  
  BOOST_FOREACH(Tank::_sp tank, tanks()) {
    double head;
    head = Units::convertValue(junctionHead(tank->name()), headUnits(), tank->head()->units());
    Point headPoint(time, head);
    tank->head()->insert(headPoint);
    tank->state_head = head;
    
    double level;
    level = Units::convertValue(tankLevel(tank->name()), headUnits(), tank->head()->units());
    Point levelPoint(time, level);
    tank->level()->insert(levelPoint);
    tank->state_level = level;
    
    double quality;
    quality = Units::convertValue(junctionQuality(tank->name()), this->qualityUnits(), tank->quality()->units());
    Point qualityPoint(time, quality);
    tank->quality()->insert(qualityPoint);
    tank->state_quality = quality;
    
    double volume;
    volume = Units::convertValue(tankVolume(tank->name()), this->volumeUnits(), tank->volume()->units());
    Point volumePoint(time,volume);
    tank->volume()->insert(volumePoint);
    tank->state_volume = volume;
    
    double flow;
    flow = Units::convertValue(tankFlow(tank->name()), this->flowUnits(), tank->flow()->units());
    Point flowPoint(time,flow);
    tank->flow()->insert(flowPoint);
    tank->state_flow = flow;
    
  }
  
  // pipe elements
  BOOST_FOREACH(Pipe::_sp pipe, pipes()) {
    double flow;
    flow = Units::convertValue(pipeFlow(pipe->name()), flowUnits(), pipe->flow()->units());
    Point aPoint(time, flow);
    pipe->flow()->insert(aPoint);
    pipe->state_flow = flow;
  }
  
  BOOST_FOREACH(Valve::_sp valve, valves()) {
    double flow;
    flow = Units::convertValue(pipeFlow(valve->name()), flowUnits(), valve->flow()->units());
    Point aPoint(time, flow);
    valve->flow()->insert(aPoint);
    valve->state_flow = flow;
  }
  
  // pump energy
  BOOST_FOREACH(Pump::_sp pump, pumps()) {
    double flow;
    flow = Units::convertValue(pipeFlow(pump->name()), flowUnits(), pump->flow()->units());
    Point flowPoint(time, flow);
    pump->flow()->insert(flowPoint);
    pump->state_flow = flow;
    
    double energy;
    energy = pumpEnergy(pump->name());
    Point energyPoint(time, energy);
    pump->energy()->insert(energyPoint);
  }
  
}

void Model::setCurrentSimulationTime(time_t time) {
  scoped_lock<boost::signals2::mutex> bigLock(_simulationInProcessMutex);
  _currentSimulationTime = time;
}
time_t Model::currentSimulationTime() {
  scoped_lock<boost::signals2::mutex> bigLock(_simulationInProcessMutex);
  return _currentSimulationTime;
}

#define LOCAL_PI 3.1415926535897932385
double Model::toRadians(double degrees) {
  double radians = degrees * LOCAL_PI / 180.0;
  return radians;
}
double Model::nodeDirectDistance(Node::_sp n1, Node::_sp n2) {
  // distance in meters
  std::pair<double, double> x1 = n1->coordinates();
  std::pair<double, double> x2 = n2->coordinates();
  double lng1 = x1.first;
  double lat1 = x1.second;
  double lng2 = x2.first;
  double lat2 = x2.second;
  double earthRadius = 3958.75;
  double dLat = toRadians(lat2-lat1);
  double dLng = toRadians(lng2-lng1);
  double a = sin(dLat/2) * sin(dLat/2) + cos(toRadians(lat1)) * cos(toRadians(lat2)) * sin(dLng/2) * sin(dLng/2);
  double c = 2 * atan2(sqrt(a), sqrt(1-a));
  double dist = earthRadius * c;
  double meterConversion = 1609.00;
  return dist * meterConversion;
}

// get output states
vector<TimeSeries::_sp> Model::networkStatesWithOptions(elementOption_t options) {
  vector<TimeSeries::_sp> states;
  vector<Element::_sp> modelElements;
  modelElements = this->elements();
  
  if (options & ElementOptionMeasuredAll) {
    options = (elementOption_t)(options | ElementOptionMeasuredFlows | ElementOptionMeasuredPressures | ElementOptionMeasuredQuality | ElementOptionMeasuredTanks);
  }

  BOOST_FOREACH(Element::_sp element, modelElements) {
    switch (element->type()) {
      case Element::JUNCTION:
      case Element::TANK:
      {
        Tank::_sp t = boost::dynamic_pointer_cast<Tank>(element);
        if (t) {
          if ((t->levelMeasure() && (options & ElementOptionMeasuredTanks)) || (options & ElementOptionAllTanks) ) {
            states.push_back(t->level()); // with level we get volume and flow
            states.push_back(t->volume());
            states.push_back(t->flow());
          }
        }
      }
      case Element::RESERVOIR:
      {
        Junction::_sp junc;
        junc = boost::dynamic_pointer_cast<Junction>(element);
        if ((junc->headMeasure() && (options & ElementOptionMeasuredTanks))  ||  (options & ElementOptionAllTanks) ) {
          states.push_back(junc->head());
        }
        if ((junc->qualityMeasure() && (options & ElementOptionMeasuredQuality))  ||  (options & ElementOptionAllQuality) ) {
          states.push_back(junc->quality());
        }
        if ((junc->qualitySource() && (options & ElementOptionMeasuredQuality))  ||  (options & ElementOptionAllQuality) ) {
          states.push_back(junc->quality());
        }
        if ((junc->pressureMeasure() && (options & ElementOptionMeasuredPressures))  ||  (options & ElementOptionAllPressures) ) {
          states.push_back(junc->pressure());
        }
        break;
      }
      case Element::PIPE:
      case Element::VALVE:
      case Element::PUMP: {
        Pipe::_sp pipe;
        pipe = boost::static_pointer_cast<Pipe>(element);
        if ((pipe->flowMeasure() && (options & ElementOptionMeasuredFlows)) || (options & ElementOptionAllFlows) ) {
          states.push_back(pipe->flow());
        }
        break;
      }
      default:
        break;
    }
  }
  return states;
}

vector<TimeSeries::_sp> Model::networkInputSeries(elementOption_t options) {
  vector<TimeSeries::_sp> measures;
  vector<Element::_sp> modelElements;
  modelElements = this->elements();
  
  if (options & ElementOptionMeasuredAll) {
    options = (elementOption_t)(options | ElementOptionMeasuredFlows | ElementOptionMeasuredPressures | ElementOptionMeasuredQuality | ElementOptionMeasuredTanks);
  }
  
  BOOST_FOREACH(Element::_sp element, modelElements) {
    switch (element->type()) {
      case Element::TANK:
      {
        Tank::_sp t = boost::dynamic_pointer_cast<Tank>(element);
        if (t->levelMeasure() && (options & ElementOptionMeasuredTanks)) {
          measures.push_back(t->levelMeasure());
        }
        if (t->flowMeasure() && (options & ElementOptionMeasuredTanks)) {
          measures.push_back(t->flowMeasure());
        }
      }
      case Element::JUNCTION:
      case Element::RESERVOIR:
      {
        Junction::_sp junc;
        junc = boost::static_pointer_cast<Junction>(element);
        if (junc->headMeasure() && (options & ElementOptionMeasuredTanks)) {
          measures.push_back(junc->headMeasure());
        }
        if (junc->qualityMeasure() && (options & ElementOptionMeasuredQuality)) {
          measures.push_back(junc->qualityMeasure());
        }
        if (junc->qualitySource() && (options & ElementOptionMeasuredQuality)) {
          measures.push_back(junc->qualitySource());
        }
        if (junc->boundaryFlow() && (options & ElementOptionMeasuredFlows)) {
          measures.push_back(junc->boundaryFlow());
        }
        if (junc->pressureMeasure() && (options & ElementOptionMeasuredPressures)) {
          measures.push_back(junc->pressureMeasure());
        }
        break;
      }
      case Element::PIPE:
      case Element::VALVE:
      case Element::PUMP: {
        Pipe::_sp pipe;
        pipe = boost::static_pointer_cast<Pipe>(element);
        if (pipe->flowMeasure() && (options & ElementOptionMeasuredFlows)) {
          measures.push_back(pipe->flowMeasure());
        }
        if (pipe->settingParameter() && (options & ElementOptionMeasuredFlows)) {
          measures.push_back(pipe->settingParameter());
        }
        if (pipe->statusParameter() && (options & ElementOptionMeasuredFlows)) {
          measures.push_back(pipe->statusParameter());
        }
        break;
      }
      default:
        break;
    }
  }
  return measures;
}

// useful for pre-fetching simulation inputs
void Model::setRecordForElementInputs(PointRecord::_sp pr) {
  vector<TimeSeries::_sp> inputs = this->networkInputSeries(ElementOptionMeasuredAll);
  BOOST_FOREACH(TimeSeries::_sp ts, inputs) {
    ts->setRecord(pr);
  }
}

void Model::setRecordForElementOutput(PointRecord::_sp record, elementOption_t options) {
  vector<TimeSeries::_sp> outputs = this->networkStatesWithOptions(options);
  BOOST_FOREACH(TimeSeries::_sp ts, outputs) {
    ts->setRecord(record);
  }
}

void Model::fetchElementInputs(TimeRange range) {
  time_t chunkSize = 60*60*24*7;
  vector<TimeSeries::_sp> inputs = this->networkInputSeries(ElementOptionMeasuredAll);
  time_t t1 = range.start;
  time_t t2 = range.start + chunkSize;
  while (t1 < range.end) {
    TimeRange tr(t1,t2);
    BOOST_FOREACH(TimeSeries::_sp ts, inputs) {
      cout << "Pre-fetching " << ts->name()  << " :: Times " << t1 << "-" << t2 << endl;
      ts->points(tr);
    }
    t1 += chunkSize;
    t2 = min(t1 + chunkSize, range.end);
  }
}

bool _rtxmodel_isDbRecord(PointRecord::_sp record) {
  return (boost::dynamic_pointer_cast<DbPointRecord>(record)) ? true : false;
}



