//
//  Model.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  


#include <iostream>
#include <set>
#include <boost/lexical_cast.hpp>
#include "Model.h"
#include <Units.h>

#include <DbPointRecord.h>


#include <boost/config.hpp>
#include <vector>
#include <algorithm>
#include <utility>

#include <boost/variant/get.hpp>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/connected_components.hpp>
#include <boost/graph/graphviz.hpp>

#include <boost/range/adaptors.hpp>
#include <future>

#include <thread>
#include <mutex>
#include <shared_mutex>
#include "openssl/sha.h"
#include "oatpp/core/base/Environment.hpp"

using namespace RTX;
using namespace TSF;
using namespace std;

bool _rtxmodel_isDbRecord(PointRecord::_sp record);


Model::Model() : _flowUnits(1), _headUnits(1), _pressureUnits(1) {
  this->initObj();
}
Model::~Model() {
  
}

void Model::initObj() {
  // default reset clock is 24 hours, default reporting clock is 1 hour.
  // TODO -- configure boundary reset clock in a smarter way?
  _regularMasterClock.reset( new Clock(3600) );
  _tankResetClock = Clock::_sp();
  _simReportClock = Clock::_sp();
  _relativeError.reset( new TimeSeries() );
  _iterations.reset( new TimeSeries() );
  _convergence.reset( new TimeSeries() );
  _heartbeat.reset( new TimeSeries() );
  
  _relativeError->setName("rel_err,generator=simulation");
  _relativeError->setUnits(TSF_DIMENSIONLESS);
  _iterations->setName("iterations,generator=simulation");
  _iterations->setUnits(TSF_DIMENSIONLESS);
  _convergence->setName("convergence,generator=simulation");
  _convergence->setUnits(TSF_DIMENSIONLESS);
  _heartbeat->name("heartbeat,generator=simulation")->units(TSF_DIMENSIONLESS);
  
  _simWallTime.reset(new TimeSeries);
  _simWallTime->name("duration,component=simulate,generator=simulation")->units(TSF_SECOND);
  _saveWallTime.reset(new TimeSeries);
  _saveWallTime->name("duration,component=save,generator=simulation")->units(TSF_SECOND);
  
  _filterWallTime.reset(new TimeSeries);
  _filterWallTime->name("duration,component=filter,generator=simulation")->units(TSF_SECOND);
  
  _doesOverrideDemands = false;
  _shouldRunWaterQuality = false;
  
  _dmaShouldDetectClosedLinks = false;
  _dmaPipesToIgnore = vector<Pipe::_sp>();
  
  // defaults
  setFlowUnits(TSF_LITER_PER_SECOND);
  setPressureUnits(TSF_PASCAL);
  setHeadUnits(TSF_METER);
  _name = "Model";
  _shouldCancelSimulation = false;
  _tanksNeedReset = false;
  
  _simLogCallback = NULL;
  _didSimulateCallback = NULL;
  
  _saveStateFuture = async(launch::async, [&](){return;});
}


Model::Model(const std::string &filename) {
  this->initObj();
  _modelFile = filename;
}

std::ostream& RTX::operator<< (std::ostream &out, Model &model) {
  return model.toStream(out);
}


void Model::useModelFromPath(const std::string& path) {
  
}

string Model::name() {
  return _name;
}

void Model::setName(string name) {
  _name = name;
}
//
//void Model::loadModelFromFile(const std::string& filename) throw(std::exception) {
//  _modelFile = filename;
//}

std::string Model::modelFile() {
  return _modelFile;
}

std::string Model::modelHash() {
  std::ifstream f(_modelFile);
  std::stringstream buffer;
  buffer << f.rdbuf();
  std::string modelString = buffer.str();
  
  unsigned char hash[SHA256_DIGEST_LENGTH];
  SHA256_CTX sha256;
  SHA256_Init(&sha256);
  SHA256_Update(&sha256, modelString.c_str(), modelString.length());
  SHA256_Final(hash, &sha256);
  
  std::stringstream ss;
  for(int i=0; i<SHA256_DIGEST_LENGTH; ++i)
        ss << std::hex << std::setfill('0') << std::setw(2) << (int)hash[i];
  return ss.str();
}

bool Model::shouldRunWaterQuality() {
  return _shouldRunWaterQuality;
}

void Model::setShouldRunWaterQuality(bool run) {
  _shouldRunWaterQuality = run;
}


// logging

Model::RTX_Logging_Callback_Block Model::simulationLoggingCallback()
{
  return _simLogCallback;
}
void Model::setSimulationLoggingCallback(Model::RTX_Logging_Callback_Block block) {
  
  _simLogCallback = block;
  
  try {
    this->logLine("Logging Simulation Activity");
  } catch (...) {
    _simLogCallback = NULL;
    std::cerr << "callback setting failed" << endl;
  }
  
}

void Model::setWillSimulateCallback(std::function<void(time_t)> cb) {
  _willSimulateCallback = cb;
}

void Model::setDidSimulateCallback(std::function<void(time_t)> cb) {
  _didSimulateCallback = cb;
}

void Model::logLine(const std::string& line) {
  DebugLog << line << EOL << flush;
  string myLine(line);
  if (_simLogCallback != NULL) {
    size_t loc = myLine.find("\n");
    if (loc == string::npos) {
      myLine += "\n";
    }
    const char *msg = myLine.c_str();
    _simLogCallback(msg);
  }
}

std::string Model::getProjectionString(){
  return _projectionString;
}

void Model::setProjectionString(std::string projectionString){
  _projectionString = projectionString;
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
  if (!units.isSameDimensionAs(TSF_LITER_PER_SECOND)) {
    cerr << "units not dimensionally consistent with flow" << endl;
    return;
  }
  _flowUnits = units;
  for(Node::_sp n : this->nodes()) {
    auto j = dynamic_pointer_cast<Junction>(n);
    j->demand()->setUnits(units);
  }
  for(Tank::_sp t : this->tanks()) {
    t->flowCalc()->setUnits(units);
    t->flow()->setUnits(units);
  }
  for(Link::_sp l : this->links()) {
    auto p = dynamic_pointer_cast<Pipe>(l);
    p->flow()->setUnits(units);
  }
}
void Model::setHeadUnits(Units units)    {
  _headUnits = units;
  for(Node::_sp n : this->nodes()) {
    auto j = dynamic_pointer_cast<Junction>(n);
    j->head()->setUnits(units);
  }
  for(Tank::_sp t : this->tanks()) {
    t->level()->setUnits(units);
  }  
}
void Model::setPressureUnits(Units units) {
  _pressureUnits = units;
  for(Node::_sp n : this->nodes()) {
    Junction::_sp j = dynamic_pointer_cast<Junction>(n);
    j->pressure()->setUnits(units);
  }
}
void Model::setQualityUnits(Units units) {
  _qualityUnits = units;
  for(Node::_sp n : this->nodes()) {
    Junction::_sp j = dynamic_pointer_cast<Junction>(n);
    j->quality()->setUnits(units);
  }
  for( auto t : this->tanks() ) {
    t->inletQuality()->setUnits(units);
  }
}

void Model::setVolumeUnits(TSF::Units units) {
  _volumeUnits = units;
  for(Tank::_sp t : this->tanks()) {
    t->volume()->setUnits(units);
    t->volumeCalc()->setUnits(units);
  }
}

#pragma mark - Storage

void Model::setRecordForDmaDemands(PointRecord::_sp record) {
  
  for(Dma::_sp dma : dmas()) {
    dma->setRecord(record);
  }
  
}

void Model::setRecordForSimulationStats(PointRecord::_sp record) {
  _relativeError->setRecord(record);
  _iterations->setRecord(record);
  _convergence->setRecord(record);
  _heartbeat->setRecord(record);
  _simWallTime->setRecord(record);
  _saveWallTime->setRecord(record);
  _filterWallTime->setRecord(record);
}

TimeSeries::_sp Model::heartbeat() {
  return _heartbeat;
}


void Model::refreshRecordsForModeledStates() {
  set<PointRecord::_sp> stateRecordsUsed;
  for(Junction::_sp e: this->junctions()) {
    vector<PointRecord::_sp> elementRecords;
    elementRecords.push_back(e->pressure()->record());
    elementRecords.push_back(e->head()->record());
    elementRecords.push_back(e->quality()->record());
    elementRecords.push_back(e->demand()->record());
    
    for(PointRecord::_sp r: elementRecords) {
      if (_rtxmodel_isDbRecord(r)) {
        stateRecordsUsed.insert(r);
      }
    }
  }
  for(Tank::_sp t: this->tanks()) {
    vector<PointRecord::_sp> recVec;
    recVec.push_back(t->level()->record());
    recVec.push_back(t->flow()->record());
    for(PointRecord::_sp r: recVec) {
      if (_rtxmodel_isDbRecord(r)) {
        stateRecordsUsed.insert(r);
      }
    }
  }
  for(Pipe::_sp p: this->pipes()) {
    vector<PointRecord::_sp> recVec;
    recVec.push_back(p->flow()->record());
    recVec.push_back(p->setting()->record());
    recVec.push_back(p->status()->record());
    for(PointRecord::_sp r: recVec) {
      if (_rtxmodel_isDbRecord(r)) {
        stateRecordsUsed.insert(r);
      }
    }
  }
  for(Valve::_sp p: this->valves()) {
    vector<PointRecord::_sp> recVec;
    recVec.push_back(p->flow()->record());
    for(PointRecord::_sp r: recVec) {
      if (_rtxmodel_isDbRecord(r)) {
        stateRecordsUsed.insert(r);
      }
    }
  }
  for(Pump::_sp p: this->pumps()) {
    vector<PointRecord::_sp> recVec;
    recVec.push_back(p->flow()->record());
    for(PointRecord::_sp r: recVec) {
      if (_rtxmodel_isDbRecord(r)) {
        stateRecordsUsed.insert(r);
      }
    }
  }
  
  _recordsForModeledStates = stateRecordsUsed;

}

#pragma mark - Demand dmas


void Model::initDMAs() {
  
  _dmas.clear();
  
  set<Pipe::_sp> boundaryPipes;
  
  using namespace boost;

  struct bNodeProp {
    string name;
    bool isMeasured;
  };
  struct bLinkProp {
    string name;
    bool isMeasured;
  };
  
  // build a boost graph of the network
  typedef boost::adjacency_list <boost::vecS, boost::vecS, boost::undirectedS, bNodeProp, bLinkProp> bNetwork;
  bNetwork G;
  
  std::map<Node::_sp, bNetwork::vertex_descriptor> nodeIndexMap;
  
  for (auto node : this->nodes()) {
    bNetwork::vertex_descriptor v = add_vertex(G);
    nodeIndexMap[node] = v;
    Junction::_sp j = dynamic_pointer_cast<Junction>(node);
    G[v].name = j->name();
    G[v].isMeasured = (j->headMeasure() ? true : false);
  }
  
  vector<Pipe::_sp> ignorePipes = this->dmaPipesToIgnore();
  
  for (auto link : this->links()) {
    Pipe::_sp pipe = std::static_pointer_cast<Pipe>(link);
    if (pipe->type() == Element::PIPE && pipe->fixedStatus() == Pipe::CLOSED) {
      continue;
    }
    bNetwork::vertex_descriptor from = nodeIndexMap[pipe->from()];
    bNetwork::vertex_descriptor to = nodeIndexMap[pipe->to()];
    
    // selectively ignore pipes
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
    pair<bNetwork::edge_descriptor,bool> edgePair = add_edge(from, to, G); // BGL add edge to graph
    auto e = edgePair.first;
    G[e].name = pipe->name();
    G[e].isMeasured = (pipe->flowMeasure() ? true : false);
    
  } // done building edges
  
  
  
  // use the BGL to find the number of connected components and get the membership list
  vector<int> componentMap(nodeIndexMap.size());
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
  for (auto nodePair : _nodes) {
    indexedNodes.push_back(nodePair.second);
  }
  
  for (int nodeIdx = 0; nodeIdx < _nodes.size(); ++nodeIdx) {
    int dmaIdx = componentMap[nodeIdx];
    Dma::_sp dma = newDmas[dmaIdx];
    dma->addJunction(std::static_pointer_cast<Junction>(indexedNodes[nodeIdx]));
  }
  
  // finally, let the dma assemble its aggregators
  for(const Dma::_sp &dma : newDmas) {
    dma->initDemandTimeseries(boundaryPipes);
    dma->demand()->setUnits(this->flowUnits());
    dma->demand()->setClock(this->_regularMasterClock);
    this->addDma(dma);
  }
  
  
  for (auto dma : newDmas) {
    string hash = dma->hashedName;
    if (dmaNameHashes.count(hash) > 0) {
      const string name = dmaNameHashes.at(hash);
      dma->setName(name);
      dma->demand()->setName("demand,dma=" + hash);
    }
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

void Model::overrideControls() {
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

void Model::addCurve(Curve::_sp newCurve) {
  if (!newCurve) {
    cerr << "curve not specified" << endl;
    return;
  }
  _curves.push_back(newCurve);
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
  for (auto nodePair : _nodes) {
    nodes.push_back(nodePair.second);
  }
  return nodes;
}

std::vector<Link::_sp> Model::links() {
  std::vector<Link::_sp> links;
  for (auto linkPair : _links) {
    links.push_back(linkPair.second);
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
vector<Curve::_sp> Model::curves() {
  return _curves;
}

void Model::removeNode(Node::_sp n) {
  
  switch (n->type()) {
    case Element::JUNCTION:
      _junctions.erase( remove(_junctions.begin(), _junctions.end(), n), _junctions.end() );
      break;
    case Element::TANK:
      _tanks.erase( remove(_tanks.begin(), _tanks.end(), n), _tanks.end() );
      break;
    case Element::RESERVOIR:
      _reservoirs.erase( remove(_reservoirs.begin(), _reservoirs.end(), n), _reservoirs.end() );
      break;
    default:
      break;
  }
  
  _nodes.erase(n->name());
  
  for (auto l : n->links()) {
    this->removeLink(l);
  }
  
}
void Model::removeLink(Link::_sp l) {
  
  switch (l->type()) {
    case Element::PIPE:
      _pipes.erase( remove(_pipes.begin(), _pipes.end(), l), _pipes.end() );
      break;
    case Element::PUMP:
      _pumps.erase( remove(_pumps.begin(), _pumps.end(), l), _pumps.end() );
      break;
    case Element::VALVE:
      _valves.erase( remove(_valves.begin(), _valves.end(), l), _valves.end() );
      break;
    default:
      break;
  }
  
  _links.erase(l->name());
  
  auto nodes = l->nodes();
  nodes.first->removeLink(l);
  nodes.second->removeLink(l);
}


#pragma mark - Engine

void Model::updateEngineWithElementProperties(Element::_sp e) {
  return;
}


#pragma mark - Publicly Accessible Simulation Methods

void Model::runSinglePeriod(time_t time) {
  cerr << "whoops, not implemented" << endl;
  return;
  /*
  // FIXME
  time_t lastSimulationPeriod, boundaryReset, start;
  // to run a single period, we need boundary conditions.
  // so back up to either the most recent set of results, or the most recent boundary-reset event
  // (whichever is nearer) and simulate through the requested time.
  //lastSimulationPeriod = _masterClock->validTime(time);
  boundaryReset = _tankResetClock->validTime(time);
  start = RTX_MAX(lastSimulationPeriod, boundaryReset);
  
  // run the simulation to the requested time.
  runExtendedPeriod(start, time);
   */
}


bool Model::solveInitial(time_t simTime) {
  _regularMasterClock->setStart(simTime);
  this->setCurrentSimulationTime(simTime);
  
  if (_willSimulateCallback != NULL) {
    _willSimulateCallback(simTime);
  }
  
  return ( this->solveAndSaveOutputAtTime(simTime) );
}


bool Model::solveAndSaveOutputAtTime(time_t simulationTime) {
  {
    std::lock_guard l(_simulationInProcessMutex);
    if (_shouldCancelSimulation) {
      return false;
    }
  }
  auto t1 = time(NULL);
  
  // get parameters from the RTX elements, and pull them into the simulation
  
  try {
    setSimulationParameters(simulationTime);
  } catch (const std::string& errorMsg) {
    stringstream ss;
    struct tm * timeinfo;
    timeinfo = localtime(&simulationTime);
    ss << std::string(errorMsg) << " :: " << asctime(timeinfo);
    this->logLine(ss.str());
    return false;
  }
  
  
  auto filterDuration = time(NULL) - t1;
  
  // _filterWallTime->insert(Point(simulationTime, (double)filterDuration));
  
  t1 = time(NULL);
  // simulate this period, find the next timestep boundary.
  bool success = false;
  try {
    success = solveSimulation(simulationTime);
  } catch (string& exc) {
    success = false;
    cerr << "exception in solver: " << exc << endl;
  }
  
  auto simWallDuration = time(NULL) - t1;
  _simWallTime->insert(Point(simulationTime, (double)simWallDuration));
  
  
  // save simulation stats here so we can track convergence issues
  Point error(simulationTime, relativeError(simulationTime));
  _relativeError->insert(error);
  Point iterationCount(simulationTime, iterations(simulationTime));
  _iterations->insert(iterationCount);
  Point convergenceStatus(simulationTime, (double)success);
  _convergence->insert(convergenceStatus);
  
  {
    lock_guard l(_simulationInProcessMutex);
    if (_shouldCancelSimulation) {
      return false;
    }
  }
  
  if (success) {
    // get the record(s) being used
    auto stateRecordsUsed = _recordsForModeledStates;
    // tell each element to update its derived states (simulation-computed values)
    if (!_simReportClock || _simReportClock->isValid(simulationTime)) {
      if (_saveStateFuture.valid()) {
        _saveStateFuture.wait();
      }
      this->fetchSimulationStates();
      
      if (_didSimulateCallback != NULL) {
        this->_didSimulateCallback(simulationTime);
      }
      _saveStateFuture = async(launch::async, &Model::saveNetworkStates, this, simulationTime, stateRecordsUsed);
      
    }
  }
  return success;
}

bool Model::updateSimulationToTime(time_t updateToTime) {
  
  if (updateToTime <= this->currentSimulationTime()) {
    return false;
  }
  
  _shouldCancelSimulation = false;
  
  while (this->currentSimulationTime() < updateToTime && !_shouldCancelSimulation) {
    
    {
      lock_guard l(_simulationInProcessMutex);
      if (_shouldCancelSimulation) {
        return false;
      }
    }
    
    // what is the next step time? go there, and solve the network.
    time_t nextSimNative, nextMasterClock, nextReport, nextTankReset;
    time_t myTime = this->currentSimulationTime();
    
    nextSimNative = nextHydraulicStep(myTime);
    nextMasterClock = myTime + _regularMasterClock->period(); // ignoring start-offset
    nextReport = (_simReportClock) ? _simReportClock->timeAfter(myTime) : nextSimNative;
    nextTankReset = (_tankResetClock) ? _tankResetClock->timeAfter(myTime) : nextSimNative;
    
    time_t stepToTime = min( min( min( min( nextSimNative, nextMasterClock ), nextReport ), nextTankReset), updateToTime);
    
    // and step the simulation to that time.
    stepSimulation(stepToTime);
    
    struct tm * timeinfo;
    timeinfo = localtime(&stepToTime);
    stringstream ss;
    ss << "INFO: Simulation step to :: " << asctime(timeinfo);
    this->logLine(ss.str());
    
    // solve the simulation at this new time.
    bool simOk = this->solveAndSaveOutputAtTime(this->currentSimulationTime());
    
    if(!simOk) {
      timeinfo = localtime (&myTime);
      stringstream ss;
      ss << "ERROR: Simulation failed :: " << asctime(timeinfo);
      this->logLine(ss.str());
      this->logLine("INFO: Resetting Tank Levels due to model non-convergence");
      
      // simulation failed -- advance the time and reset tank levels
      this->setCurrentSimulationTime(_regularMasterClock->timeAfter(myTime));
      this->setTanksNeedReset(true);
    }
  }
  
  {
    lock_guard l(_simulationInProcessMutex);
    _shouldCancelSimulation = false;
  }
  
  return true;
}




void Model::runExtendedPeriod(time_t start, time_t end) {
  
  this->solveInitial(start);
  this->updateSimulationToTime(end);
  this->cleanupModelAfterSimulation();
  
  _shouldCancelSimulation = false;
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
  this->refreshRecordsForModeledStates();
  auto stateRecordsUsed = _recordsForModeledStates;
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
        saveNetworkStates(simulationTime, stateRecordsUsed);
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
      cout << "Simulation time :: " << asctime(timeinfo) << EOL << flush;
    }
    else {
      // simulation failed -- advance the time and reset tank levels
      nextClockTime = _regularMasterClock->timeAfter(simulationTime);
      simulationTime = nextClockTime;
      setCurrentSimulationTime(simulationTime);
      cout << "will reset tanks" << EOL << flush;
      this->setTanksNeedReset(true);
    }
    
  } // simulation loop
  
  this->cleanupModelAfterSimulation();
  
}


void Model::cancelSimulation() {
  lock_guard l(_simulationInProcessMutex);
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

double Model::initialUniformQuality() {
  return _initialQuality;
}

void Model::setInitialJunctionUniformQuality(double qual) {
  _initialQuality = qual;
  // Constant initial quality of Junctions and Tanks (Reservoirs are boundary conditions)
  for(Junction::_sp junc : this->junctions()) {
    junc->state_quality = qual;
  }
  for(Tank::_sp tank : this->tanks()) {
    tank->state_quality = qual;
  }
  for(auto r : this->reservoirs()) {
    r->state_quality = qual;
  }
  this->applyInitialQuality();
}

void Model::setTankResetClock(Clock::_sp resetClock) {
  _tankResetClock = resetClock;
}

bool Model::tanksNeedReset() {
  return _tanksNeedReset;
}

void Model::setTanksNeedReset(bool reset) {
  _tanksNeedReset = reset;
  
  for(Tank::_sp tank : this->tanks()) {
    tank->setNeedsReset(reset);
  }
  
}

void Model::_checkTanksForReset(time_t time) {
  
  if (!tanksNeedReset()) {
    return;
  }
  
  for(Tank::_sp tank : this->tanks()) {
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
  for(Junction::_sp junc : this->junctions()) {
    if (junc->qualityMeasure()) {
      TimeSeries::_sp qualityTS = junc->qualityMeasure();
      Point aPoint = qualityTS->pointAtOrBefore(time);
      if (aPoint.isValid) {
        measuredJunctions.push_back(make_pair(junc, aPoint.value));
      }
    }
  }
  // tank measurements
  for(Tank::_sp tank : this->tanks()) {
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
  for(Junction::_sp junc : this->junctions()) {
    double minDistance = DBL_MAX;
    double initQuality = 0;
    for(auto mjunc : measuredJunctions) {
      double d = nodeDirectDistance(junc, mjunc.first);
      if (d < minDistance) {
        minDistance = d;
        initQuality = mjunc.second;
      }
    }
    // initialize the junction quality
    junc->state_quality = initQuality;
//    cout << "Junction " << junc->name() << " Quality: " << initQuality << endl;
  }
  // tanks
  for(Tank::_sp tank : this->tanks()) {
    double minDistance = DBL_MAX;
    double initQuality = 0;
    for(auto mjunc : measuredJunctions) {
      double d = nodeDirectDistance(tank, mjunc.first);
      if (d < minDistance) {
        minDistance = d;
        initQuality = mjunc.second;
      }
    }
    // initialize the tank quality
    tank->state_quality = initQuality;
//    cout << "Tank " << tank->name() << " Quality: " << initQuality << endl;
  }
  
}

std::vector<Node::_sp> Model::nearestNodes(Node::_sp node, double maxDistance) {
  // simple enumeration -- max distance in meters
  vector<Node::_sp> nodeList;
  for(auto nodePair : _nodes) {
    if (nodeDirectDistance(nodePair.second, node) <= maxDistance) {
      nodeList.push_back(nodePair.second);
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
    for(Dma::_sp d : _dmas) {
      stream << "## DMA: " << d->name() << endl;
      stream << *d << endl;
    }
  }
  
  
  return stream;
}

void Model::setSimulationParameters(time_t time) {
  struct tm * timeinfo = localtime (&time);
  std::stringstream time_str;
  time_str << put_time(timeinfo, "%F");
  OATPP_LOGD("Model", "Setting model inputs: %s", time_str.str().c_str());
//  cout << EOL << "*** SETTING MODEL INPUTS *** " << asctime(timeinfo) << " - " << time << EOL;
  // set all element parameters
  
  // allocate junction demands based on dmas, and set the junction demand values in the model.
  if (_doesOverrideDemands) {
    // by dma, insert demand point into each junction timeseries at the current simulation time
    for(Dma::_sp dma: this->dmas()) {
      if ( dma->allocateDemandToJunctions(time) ) {
        stringstream ss;
        ss << "ERROR: Invalid demand value for DMA " << dma->name() << "(" << dma->junctions().size() << "junctions)" << " :: " << asctime(timeinfo);
        this->logLine(ss.str());
      }
      else {
        Point dPoint = dma->demand()->pointAtOrBefore(time);
        DebugLog << "*  DMA: " << dma->name() << " demand --> " << dPoint.value << EOL;
      }
      
    }
    // hydraulic junctions - set demand values.
    for(Junction::_sp junction: this->junctions()) {
      double demandValue = Units::convertValue(junction->state_demand, junction->demand()->units(), flowUnits());
      setJunctionDemand(junction->name(), demandValue);
    }
  }
  
  // for reservoirs, set the boundary head
  for(Reservoir::_sp reservoir: this->reservoirs()) {
    if (reservoir->boundaryHead()) {
      // get the head measurement parameter, and pass it through as a state.
      Point p = reservoir->boundaryHead()->pointAtOrBefore(time);
      if (p.isValid) {
        double headValue = Units::convertValue(p.value, reservoir->boundaryHead()->units(), headUnits());
        setReservoirHead( reservoir->name(), headValue );
        DebugLog << "*  Reservoir " << reservoir->name() << " head --> " << p.value << EOL;
      }
      else {
        stringstream ss;
        ss << "ERROR: Invalid head value for reservoir: " << reservoir->name() << " :: " << asctime(timeinfo);
        this->logLine(ss.str());
      }
    }
  }
  
  // check for valid time with tank reset clock
  if (_tankResetClock && _tankResetClock->isValid(time)) {
    this->setTanksNeedReset(true);
    OATPP_LOGD("Model", "Tanks will be reset");
//    cout << "*  INFO :: TANKS WILL BE RESET" << EOL;
  }
  
  _checkTanksForReset(time);

  // for valves, set status and setting
  for(Valve::_sp valve: this->valves()) {
    // status can affect settings and vice-versa; status rules
    Pipe::status_t status = valve->fixedStatus();
    if (valve->statusBoundary()) {
      Point p = valve->statusBoundary()->pointAtOrBefore(time);
      if (p.isValid) {
        status = Pipe::status_t((int)(p.value));
        setPipeStatusControl( valve->name(), status, enable );
        DebugLog << "*  Valve " << valve->name() << " status --> " << (p.value > 0 ? "ON" : "OFF") << EOL;
      }
      else {
        stringstream ss;
        ss << "ERROR: Invalid status value for valve: " << valve->name() << " :: " << asctime(timeinfo);
        this->logLine(ss.str());
      }
    }
    if (valve->settingBoundary()) {
      Units settingUnits = valve->settingBoundary()->units();
      if (status) {
        Point p = valve->settingBoundary()->pointAtOrBefore(time);
        if (p.isValid) {
          if (settingUnits.isSameDimensionAs(TSF_PSI)) {
            p = Point::convertPoint(p, settingUnits, this->pressureUnits());
          }
          else if (settingUnits.isSameDimensionAs(TSF_GALLON_PER_MINUTE)) {
            p = Point::convertPoint(p, settingUnits, this->flowUnits());
          }
          setValveSettingControl( valve->name(), p.value, enable );
          DebugLog << "*  Valve " << valve->name() << " setting --> " << p.value << EOL;
        }
        else {
          stringstream ss;
          ss << "ERROR: Invalid setting value for Valve: " << valve->name() << " :: " << asctime(timeinfo);
          this->logLine(ss.str());
        }
      }
      else {
        setValveSettingControl( valve->name(), 0.0, disable );
        stringstream ss;
        ss << "WARN: Ignoring setting for Valve because status is Closed: " << valve->name() << " :: " << asctime(timeinfo);
//        this->logLine(ss.str());
      }
    }
  }
  
  // for pumps, set status and setting
  for(Pump::_sp pump: this->pumps()) {
    // status can affect settings and vice-versa; status rules
    Pipe::status_t status = pump->fixedStatus();
    if (pump->statusBoundary()) {
      Point p = pump->statusBoundary()->pointAtOrBefore(time);
      if (p.isValid) {
        status = Pipe::status_t((int)(p.value));
        setPumpStatusControl( pump->name(), status, enable );
        DebugLog << "*  Pump " << pump->name() << " status --> " << (p.value > 0 ? "ON" : "OFF") << EOL;
      }
      else {
        stringstream ss;
        ss << "ERROR: Invalid status value for pump: " << pump->name() << " :: " << asctime(timeinfo);
        this->logLine(ss.str());
      }
    }
    if (pump->settingBoundary()) {
      if (status == Pipe::OPEN) {
        Point p = pump->settingBoundary()->pointAtOrBefore(time);
        // edge case where series is in % or purely dimensionless
        if (pump->settingBoundary()->units() == TSF_PERCENT) {
          p.value /= 100.0;
        }
        if (p.isValid) {
          setPumpSettingControl( pump->name(), p.value, enable );
          DebugLog << "*  Pump " << pump->name() << " setting --> " << p.value << EOL;
        }
        else {
          stringstream ss;
          ss << "ERROR: Invalid setting value for pump: " << pump->name() << " :: " << asctime(timeinfo);
          this->logLine(ss.str());
        }
      }
      else {
        setPumpSettingControl( pump->name(), 0.0, disable );
        stringstream ss;
        ss << "WARN: Ignoring setting for Pump because status is Closed: " << pump->name() << " :: " << asctime(timeinfo);
//        this->logLine(ss.str());
      }
    }
  }
  
  // for pipes, set status
  for(Pipe::_sp pipe: this->pipes()) {
    if (pipe->statusBoundary()) {
      Point p = pipe->statusBoundary()->pointAtOrBefore(time);
      if (p.isValid) {
        Pipe::status_t status = Pipe::status_t((int)(p.value));
        setPipeStatusControl(pipe->name(), status, enable);
        DebugLog << "*  Pipe " << pipe->name() << " status --> " << (p.value > 0 ? "ON" : "OFF") << EOL;
      }
      else {
        stringstream ss;
        ss << "WARN: Invalid status value for pipe" << pipe->name() << " :: " << asctime(timeinfo);
        this->logLine(ss.str());
      }
    }
  }
  
  //////////////////////////////
  // water quality parameters //
  //////////////////////////////
  if (this->shouldRunWaterQuality()) {
    for(Junction::_sp j: this->junctions()) {
      if (j->qualitySource()) {
        Point p = j->qualitySource()->pointAtOrBefore(time);
        if (p.isValid) {
          double quality = Units::convertValue(p.value, j->qualitySource()->units(), qualityUnits());
          setJunctionQuality(j->name(), quality);
          DebugLog << "*  Junction " << j->name() << " quality --> " << p.value << EOL;
        }
        else {
          stringstream ss;
          ss << "ERROR: Invalid quality value for junction" << j->name() << " :: " << asctime(timeinfo);
          this->logLine(ss.str());
        }
      }
    }
    for(Reservoir::_sp reservoir: this->reservoirs()) {
      if (reservoir->boundaryQuality()) {
        // get the quality measurement parameter, and pass it through as a state.
        Point p = reservoir->boundaryQuality()->pointAtOrBefore(time);
        if (p.isValid) {
          double qualityValue = Units::convertValue(p.value, reservoir->boundaryQuality()->units(), qualityUnits());
          setReservoirQuality( reservoir->name(), qualityValue );
          DebugLog << "*  Reservoir " << reservoir->name() << " quality --> " << p.value << EOL;
        }
        else {
          stringstream ss;
          ss << "ERROR: Invalid quality value for reservoir: " << reservoir->name() << " :: " << asctime(timeinfo);
          this->logLine(ss.str());
        }
      }
    }
    for(Tank::_sp tank: this->tanks()) {
      if (tank->qualitySource()) {
        Point p = tank->qualitySource()->pointAtOrBefore(time);
        if (p.isValid) {
          double qualityValue = Units::convertValue(p.value, tank->qualitySource()->units(), qualityUnits());
          setReservoirQuality( tank->name(), qualityValue ); // using setReservoirQuality to set concentration rather than mass addition
          DebugLog << "*  Tank " << tank->name() << " quality --> " << p.value << EOL;
        }
        else {
          stringstream ss;
          ss << "ERROR: Invalid quality value for tank: " << tank->name() << " :: " << asctime(timeinfo);
          this->logLine(ss.str());
        }
      }
    }
  }
  DebugLog << "****************************" << EOL << flush;
}



void Model::fetchSimulationStates() {
  
  // retrieve results from the hydraulic sim
  // then insert the state values into elements' "short-term" memory
  
  // junctions, tanks, reservoirs
  for(Junction::_sp junction : junctions()) {
    double head;
    head = Units::convertValue(junctionHead(junction->name()), headUnits(), junction->head()->units());
    junction->state_head = head;
    
    double pressure;
    pressure = Units::convertValue(junctionPressure(junction->name()), pressureUnits(), junction->pressure()->units());
    junction->state_pressure = pressure;
    
    // todo - more fine-grained quality data? at wq step resolution...
    if (this->shouldRunWaterQuality()) {
      double quality;
      quality = Units::convertValue(junctionQuality(junction->name()), this->qualityUnits(), junction->quality()->units());
      junction->state_quality = quality;
    }
  }
  
  if (!_doesOverrideDemands) { // otherwise this state ivar is set by the containing DMA object
    for(Junction::_sp junction : junctions()) {
      double demand = Units::convertValue(junctionDemand(junction->name()), flowUnits(), junction->demand()->units());
      junction->state_demand = demand;
    }
  }
  
  for(Reservoir::_sp reservoir : reservoirs()) {
    double head;
    head = Units::convertValue(junctionHead(reservoir->name()), headUnits(), reservoir->head()->units());
    reservoir->state_head = head;
    
    double quality;
    quality = Units::convertValue(junctionQuality(reservoir->name()), this->qualityUnits(), reservoir->quality()->units());
    reservoir->state_quality = quality;
  }
  
  for(Tank::_sp tank : tanks()) {
    double head;
    head = Units::convertValue(junctionHead(tank->name()), headUnits(), tank->head()->units());
    tank->state_head = head;
    
    double level;
    level = Units::convertValue(tankLevel(tank->name()), headUnits(), tank->head()->units());
    tank->state_level = level;
    
    double volume;
    volume = Units::convertValue(tankVolume(tank->name()), this->volumeUnits(), tank->volume()->units());
    tank->state_volume = volume;
    
    double flow;
    flow = Units::convertValue(tankFlow(tank->name()), this->flowUnits(), tank->flow()->units());
    tank->state_flow = flow;
    
    if (this->shouldRunWaterQuality()) {
      double quality;
      quality = Units::convertValue(junctionQuality(tank->name()), this->qualityUnits(), tank->quality()->units());
      tank->state_quality = quality;
      
      double inletQuality;
      inletQuality = Units::convertValue(tankInletQuality(tank->name()), this->qualityUnits(), tank->inletQuality()->units());
      tank->state_inlet_quality = inletQuality;
    }
  }
  
  // link elements
  for(Link::_sp link : this->links()) {
    auto pipe = dynamic_pointer_cast<Pipe>(link);
    
    double flow;
    flow = Units::convertValue(pipeFlow(pipe->name()), flowUnits(), pipe->flow()->units());
    pipe->state_flow = flow;
    
    double setting = pipeSetting(pipe->name());
    pipe->state_setting = setting;
    
    double status = pipeStatus(pipe->name());
    pipe->state_status = status;
    
    double energy = pipeEnergy(pipe->name());
    pipe->state_energy = energy;
  }
  
  for(Valve::_sp valve : valves()) {
    // nothing extra
  }
  
  
}


void Model::saveNetworkStates(time_t simtime, std::set<PointRecord::_sp> bulkRecords) {
  struct tm * timeinfo = localtime (&simtime);
  OATPP_LOGD("Model", "saving network states: %s", put_time(timeinfo, "%c"));
//  cout << "*** saving network states ***" << asctime(timeinfo) << " - " << simtime << EOL << flush;
  auto t1 = time(NULL);

  for(PointRecord::_sp r: bulkRecords) {
    r->beginBulkOperation();
  }
  // retrieve results from the hydraulic sim
  // then insert the state values into elements' time series.
  // junctions, tanks, reservoirs
  for(Junction::_sp junction : junctions()) {
    junction->head()->insert(Point(simtime, junction->state_head));
    junction->pressure()->insert(Point(simtime, junction->state_pressure));
    // todo - more fine-grained quality data? at wq step resolution...
    if (this->shouldRunWaterQuality()) {
      junction->quality()->insert(Point(simtime, junction->state_quality));
    }
  }
  
  for(Junction::_sp junction : junctions()) {
    junction->demand()->insert(Point(simtime, junction->state_demand));
  }
  
  for(Reservoir::_sp reservoir : reservoirs()) {
    reservoir->head()->insert(Point(simtime, reservoir->state_head));
    if (this->shouldRunWaterQuality()) {
      reservoir->quality()->insert(Point(simtime, reservoir->state_quality));
    }
  }
  
  for(Tank::_sp tank : tanks()) {
    tank->head()->insert(Point(simtime, tank->state_head));
    tank->level()->insert(Point(simtime, tank->state_level));
    tank->volume()->insert(Point(simtime,tank->state_volume));
    tank->flow()->insert(Point(simtime,tank->state_flow));
    if (this->shouldRunWaterQuality()) {
      tank->quality()->insert(Point(simtime, tank->state_quality));
      if (!isnan(tank->state_inlet_quality)) {
        tank->inletQuality()->insert(Point(simtime, tank->state_inlet_quality));
      }
    }
  }
  
  
  // pipe elements
  
  if (this->shouldRunWaterQuality()) {
    for(Pipe::_sp pipe : pipes()) {
      pipe->quality()->insert(Point(simtime, pipe->state_quality()));
    }
    for(Valve::_sp valve : valves()) {
      valve->quality()->insert(Point(simtime, valve->state_quality()));
    }
    for(Pump::_sp pump : pumps()) {
      pump->quality()->insert(Point(simtime, pump->state_quality()));
    }
  }
  
  for(Pipe::_sp pipe : pipes()) {
    pipe->flow()->insert(Point(simtime, pipe->state_flow));
    pipe->setting()->insert(Point(simtime, pipe->state_setting));
    pipe->status()->insert(Point(simtime, pipe->state_status));
  }
  
  for(Valve::_sp valve : valves()) {
    valve->flow()->insert(Point(simtime, valve->state_flow));
    valve->setting()->insert(Point(simtime, valve->state_setting));
    valve->status()->insert(Point(simtime, valve->state_status));
  }
  
  // pump energy
  for(Pump::_sp pump : pumps()) {
    pump->flow()->insert(Point(simtime, pump->state_flow));
    pump->energy()->insert(Point(simtime, pump->state_energy));
    pump->setting()->insert(Point(simtime, pump->state_setting));
    pump->status()->insert(Point(simtime, pump->state_status));
  }
  
  
  for(PointRecord::_sp r : bulkRecords) {
    r->endBulkOperation();
  }
  
  
  auto saveWallDuration = time(NULL) - t1;
  // _saveWallTime->insert(Point(simtime, (double)saveWallDuration));
  
  // beating heart just after everything else is done.
  // _heartbeat->insert(Point(simtime,1.0));
  OATPP_LOGD("Model", "finished saving states");
//  cout << "*** finished saving states ****" << EOL << flush;
}

void Model::setCurrentSimulationTime(time_t time) {
  lock_guard bigLock(_simulationInProcessMutex);
  _currentSimulationTime = time;
}
time_t Model::currentSimulationTime() {
  lock_guard bigLock(_simulationInProcessMutex);
  return _currentSimulationTime;
}

#define LOCAL_PI 3.1415926535897932385
double Model::toRadians(double degrees) {
  double radians = degrees * LOCAL_PI / 180.0;
  return radians;
}
double Model::nodeDirectDistance(Node::_sp n1, Node::_sp n2) {
  // distance in meters
  Node::location_t x1 = n1->coordinates();
  Node::location_t x2 = n2->coordinates();
  double lng1 = x1.longitude;
  double lat1 = x1.latitude;
  double lng2 = x2.longitude;
  double lat2 = x2.latitude;
  double earthRadius = 3958.75;
  double dLat = toRadians(lat2-lat1);
  double dLng = toRadians(lng2-lng1);
  double a = sin(dLat/2) * sin(dLat/2) + cos(toRadians(lat1)) * cos(toRadians(lat2)) * sin(dLng/2) * sin(dLng/2);
  double c = 2 * atan2(sqrt(a), sqrt(1-a));
  double dist = earthRadius * c;
  double meterConversion = 1609.00;
  return dist * meterConversion;
}
//
//// get output states
//vector<TimeSeries::_sp> Model::networkStatesWithOptions(elementOption_t options) {
//  vector<TimeSeries::_sp> states;
//  vector<Element::_sp> modelElements;
//  modelElements = this->elements();
//
//  if (options & ElementOptionMeasuredAll) {
//    options = (elementOption_t)(options | ElementOptionMeasuredFlows | ElementOptionMeasuredPressures | ElementOptionMeasuredQuality | ElementOptionMeasuredTanks);
//  }
//
//  for(Element::_sp element : modelElements) {
//    switch (element->type()) {
//      case Element::JUNCTION:
//      case Element::TANK:
//      {
//        Tank::_sp t = std::dynamic_pointer_cast<Tank>(element);
//        if (t) {
//          if ((t->levelMeasure() && (options & ElementOptionMeasuredTanks)) || (options & ElementOptionAllTanks) ) {
//            states.push_back(t->level()); // with level we get volume and flow
//            states.push_back(t->volume());
//            states.push_back(t->flow());
//          }
//        }
//      }
//      case Element::RESERVOIR:
//      {
//        Junction::_sp junc;
//        junc = std::dynamic_pointer_cast<Junction>(element);
//        if ((junc->headMeasure() && (options & ElementOptionMeasuredTanks))  ||  (options & ElementOptionAllTanks) ) {
//          states.push_back(junc->head());
//        }
//        if ((junc->qualityMeasure() && (options & ElementOptionMeasuredQuality))  ||  (options & ElementOptionAllQuality) ) {
//          states.push_back(junc->quality());
//        }
//        if ((junc->qualitySource() && (options & ElementOptionMeasuredQuality))  ||  (options & ElementOptionAllQuality) ) {
//          states.push_back(junc->quality());
//        }
//        if ((junc->pressureMeasure() && (options & ElementOptionMeasuredPressures))  ||  (options & ElementOptionAllPressures) ) {
//          states.push_back(junc->pressure());
//        }
//        break;
//      }
//      case Element::PIPE:
//      case Element::VALVE:
//      case Element::PUMP: {
//        Pipe::_sp pipe;
//        pipe = std::static_pointer_cast<Pipe>(element);
//        if ((pipe->flowMeasure() && (options & ElementOptionMeasuredFlows)) || (options & ElementOptionAllFlows) ) {
//          states.push_back(pipe->flow());
//        }
//        break;
//      }
//      default:
//        break;
//    }
//  }
//  return states;
//}
//
//vector<TimeSeries::_sp> Model::networkInputSeries(elementOption_t options) {
//  vector<TimeSeries::_sp> measures;
//  vector<Element::_sp> modelElements;
//  modelElements = this->elements();
//
//  if (options & ElementOptionMeasuredAll) {
//    options = (elementOption_t)(options | ElementOptionMeasuredFlows | ElementOptionMeasuredPressures | ElementOptionMeasuredQuality | ElementOptionMeasuredTanks | ElementOptionMeasuredSettings | ElementOptionMeasuredStatuses);
//  }
//
//  for(Element::_sp element : modelElements) {
//    switch (element->type()) {
//      case Element::TANK:
//      {
//        Tank::_sp t = std::dynamic_pointer_cast<Tank>(element);
//        if (t->levelMeasure() && (options & ElementOptionMeasuredTanks)) {
//          measures.push_back(t->levelMeasure());
//        }
//        if (t->headMeasure() && (options & ElementOptionMeasuredTanks)) {
//          measures.push_back(t->headMeasure());
//        }
//        if (t->qualityMeasure() && (options & ElementOptionMeasuredQuality)) {
//          measures.push_back(t->qualityMeasure());
//        }
//        if (t->qualitySource() && (options & ElementOptionMeasuredQuality)) {
//          measures.push_back(t->qualitySource());
//        }
//        break;
//      }
//      case Element::RESERVOIR:
//      {
//        Reservoir::_sp r = std::dynamic_pointer_cast<Reservoir>(element);
//        if (r->headMeasure() && (options & ElementOptionMeasuredTanks)) {
//          measures.push_back(r->headMeasure());
//        }
//        if (r->qualityMeasure() && (options & ElementOptionMeasuredQuality)) {
//          measures.push_back(r->qualityMeasure());
//        }
//        if (r->qualitySource() && (options & ElementOptionMeasuredQuality)) {
//          measures.push_back(r->qualitySource());
//        }
//        break;
//      }
//      case Element::JUNCTION:
//      {
//        Junction::_sp junc;
//        junc = std::static_pointer_cast<Junction>(element);
//        if (junc->qualityMeasure() && (options & ElementOptionMeasuredQuality)) {
//          measures.push_back(junc->qualityMeasure());
//        }
//        if (junc->qualitySource() && (options & ElementOptionMeasuredQuality)) {
//          measures.push_back(junc->qualitySource());
//        }
//        if (junc->boundaryFlow() && (options & ElementOptionMeasuredFlows)) {
//          measures.push_back(junc->boundaryFlow());
//        }
//        if (junc->pressureMeasure() && (options & ElementOptionMeasuredPressures)) {
//          measures.push_back(junc->pressureMeasure());
//        }
//        if (junc->headMeasure() && (options & ElementOptionMeasuredPressures)) {
//          measures.push_back(junc->headMeasure());
//        }
//        break;
//      }
//      case Element::PIPE:
//      case Element::VALVE:
//      case Element::PUMP: {
//        Pipe::_sp pipe;
//        pipe = std::static_pointer_cast<Pipe>(element);
//        if (pipe->flowMeasure() && (options & ElementOptionMeasuredFlows)) {
//          measures.push_back(pipe->flowMeasure());
//        }
//        if (pipe->settingBoundary() && (options & ElementOptionMeasuredSettings)) {
//          measures.push_back(pipe->settingBoundary());
//        }
//        if (pipe->statusBoundary() && (options & ElementOptionMeasuredStatuses)) {
//          measures.push_back(pipe->statusBoundary());
//        }
//        break;
//      }
//      default:
//        break;
//    }
//  }
//  return measures;
//}
//
//set<TimeSeries::_sp> Model::networkInputRootSeries(elementOption_t options) {
//  vector<TimeSeries::_sp> inputs = this->networkInputSeries(options);
//  set<TimeSeries::_sp> rootTs;
//  for(TimeSeries::_sp ts : inputs) {
//    TimeSeries::_sp rTs = ts->rootTimeSeries();
//    rootTs.insert(rTs);
//  }
//  return rootTs;
//}


// useful for pre-fetching simulation inputs
//void Model::setRecordForElementInputs(PointRecord::_sp pr) {
//  vector<TimeSeries::_sp> inputs = this->networkInputSeries(ElementOptionMeasuredAll);
//  for(TimeSeries::_sp ts : inputs) {
//    ts->setRecord(pr);
//  }
//}
//
//void Model::setRecordForElementOutput(PointRecord::_sp record, elementOption_t options) {
//  vector<TimeSeries::_sp> outputs = this->networkStatesWithOptions(options);
//  for(TimeSeries::_sp ts : outputs) {
//    ts->setRecord(record);
//  }
//}
//
//void Model::fetchElementInputs(TimeRange range) {
//  time_t chunkSize = 60*60*24*7;
//  vector<TimeSeries::_sp> inputs = this->networkInputSeries(ElementOptionMeasuredAll);
//  time_t t1 = range.start;
//  time_t t2 = range.start + chunkSize;
//  while (t1 < range.end) {
//    TimeRange tr(t1,t2);
//    for(TimeSeries::_sp ts : inputs) {
//      cout << "Pre-fetching " << ts->name()  << " :: Times " << t1 << "-" << t2 << endl << flush;
//      ts->points(tr);
//    }
//    t1 += chunkSize;
//    t2 = min(t1 + chunkSize, range.end);
//  }
//}

bool _rtxmodel_isDbRecord(PointRecord::_sp record) {
  return (std::dynamic_pointer_cast<DbPointRecord>(record)) ? true : false;
}



