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


#include <boost/config.hpp>
#include <vector>
#include <algorithm>
#include <utility>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/connected_components.hpp>
#include <boost/range/adaptors.hpp>

using namespace RTX;
using namespace std;


Model::Model() : _flowUnits(1), _headUnits(1), _pressureUnits(1) {
  // default reset clock is 24 hours, default reporting clock is 1 hour.
  // TODO -- configure boundary reset clock in a smarter way?
  _regularMasterClock.reset( new Clock(3600) );
  _boundaryResetClock = Clock::sharedPointer( new Clock(24 * 3600) );
  _relativeError.reset( new TimeSeries() );
  _iterations.reset( new TimeSeries() );
  
  _relativeError->setName("Relative Error");
  _iterations->setName("Iterations");
  _doesOverrideDemands = false;
  _shouldRunWaterQuality = false;
  
  _dmaShouldDetectClosedLinks = false;
  _dmaPipesToIgnore = vector<Pipe::sharedPointer>();
  
  // defaults
  setFlowUnits(RTX_LITER_PER_SECOND);
  setPressureUnits(RTX_PASCAL);
  setHeadUnits(RTX_METER);
  _name = "Model";
  
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
  BOOST_FOREACH(Junction::sharedPointer j, this->junctions()) {
    j->demand()->setUnits(units);
  }
  BOOST_FOREACH(Tank::sharedPointer t, this->tanks()) {
    t->flowMeasure()->setUnits(units);
  }
  BOOST_FOREACH(Pipe::sharedPointer p, this->pipes()) {
    p->flow()->setUnits(units);
  }
  BOOST_FOREACH(Pump::sharedPointer p, this->pumps()) {
    p->flow()->setUnits(units);
  }
  BOOST_FOREACH(Valve::sharedPointer v, this->valves()) {
    v->flow()->setUnits(units);
  }
}
void Model::setHeadUnits(Units units)    {
  _headUnits = units;
  
  BOOST_FOREACH(Junction::sharedPointer j, this->junctions()) {
    j->head()->setUnits(units);
  }
  BOOST_FOREACH(Tank::sharedPointer t, this->tanks()) {
    t->head()->setUnits(units);
    t->level()->setUnits(units);
  }
  BOOST_FOREACH(Reservoir::sharedPointer r, this->reservoirs()) {
    r->head()->setUnits(units);
  }
  
}
void Model::setPressureUnits(Units units) {
  _pressureUnits = units;
  BOOST_FOREACH(Junction::sharedPointer j, this->junctions()) {
    j->pressure()->setUnits(units);
  }
}
void Model::setQualityUnits(Units units) {
  _qualityUnits = units;
  BOOST_FOREACH(Junction::sharedPointer j, this->junctions()) {
    j->quality()->setUnits(units);
  }
  BOOST_FOREACH(Tank::sharedPointer t, this->tanks()) {
    t->quality()->setUnits(units);
  }
  BOOST_FOREACH(Reservoir::sharedPointer r, this->reservoirs()) {
    r->quality()->setUnits(units);
  }
  
}

void Model::setVolumeUnits(RTX::Units units) {
  _volumeUnits = units;
}

#pragma mark - Storage
void Model::setStorage(PointRecord::sharedPointer record) {
  _record = record;

  std::vector< Element::sharedPointer > elements = this->elements();
  BOOST_FOREACH(Element::sharedPointer element, elements) {
    //std::cout << "setting storage for: " << element->name() << std::endl;
    element->setRecord(_record);
  }
  
  BOOST_FOREACH(Dma::sharedPointer dma, dmas()) {
    dma->setRecord(_record);
  }
  
  _relativeError->setRecord(_record);
  _iterations->setRecord(_record);
  
}

void Model::setParameterSource(PointRecord::sharedPointer record) {

  BOOST_FOREACH(Junction::sharedPointer junction, this->junctions()) {
    if (junction->doesHaveBoundaryFlow()) {
      junction->boundaryFlow()->setRecord(record);
    }
    if (junction->doesHaveHeadMeasure()) {
      junction->headMeasure()->setRecord(record);
    }
  }

  BOOST_FOREACH(Reservoir::sharedPointer reservoir, this->reservoirs()) {
    if (reservoir->doesHaveBoundaryHead()) {
      reservoir->boundaryHead()->setRecord(record);
    }
  }

  BOOST_FOREACH(Valve::sharedPointer valve, this->valves()) {
    if (valve->doesHaveStatusParameter()) {
      valve->statusParameter()->setRecord(record);
    }
    if (valve->doesHaveSettingParameter()) {
      valve->settingParameter()->setRecord(record);
    }
  }
  
  BOOST_FOREACH(Pump::sharedPointer pump, this->pumps()) {
    if (pump->doesHaveStatusParameter()) {
      pump->statusParameter()->setRecord(record);
    }
  }
}


#pragma mark - Demand dmas


void Model::initDMAs() {
  
  _dmas.clear();
  
  set<Pipe::sharedPointer> boundaryPipes;
  
  using namespace boost;
  std::map<Node::sharedPointer, int> nodeIndexMap;
  boost::adjacency_list <boost::vecS, boost::vecS, boost::undirectedS> G;
  
  typedef struct {
    Link::sharedPointer link;
    int fromIdx, toIdx;
  } LinkDescriptor;
  
  // load up the link descriptor lookup table
  vector<LinkDescriptor> linkDescriptors;
  {
    int iNode = 0;
    BOOST_FOREACH(Node::sharedPointer node, _nodes | boost::adaptors::map_values) {
      nodeIndexMap[node] = iNode;
      ++iNode;
    }
    BOOST_FOREACH(Link::sharedPointer link, _links | boost::adaptors::map_values) {
      int from = nodeIndexMap[link->from()];
      int to = nodeIndexMap[link->to()];
      linkDescriptors.push_back((LinkDescriptor){link, from, to});
    }
  }
  
  //
  // build a boost graph of the network, ignoring links that are dma boundaries or explicitly ignored
  // is this faster than adapting the model to be BGL compliant? certainly faster dev time, but this should be tested in code.
  //
  vector<Pipe::sharedPointer> ignorePipes = this->dmaPipesToIgnore();
  
  BOOST_FOREACH(Link::sharedPointer link, _links | boost::adaptors::map_values) {
    Pipe::sharedPointer pipe = boost::static_pointer_cast<Pipe>(link);
    // flow measure?
    if (pipe->doesHaveFlowMeasure()) {
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
  vector<Dma::sharedPointer> newDmas;
  for (int iDma = 0; iDma < nDmas; ++iDma) {
    stringstream dmaNameStream("");
    dmaNameStream << "dma " << iDma;
    Dma::sharedPointer dma( new Dma(dmaNameStream.str()) );
    newDmas.push_back(dma);
  }
  
  // now that we have the dma list, go through the node membership and add each node object to the appropriate dma.
  vector<Node::sharedPointer> indexedNodes;
  indexedNodes.reserve(_nodes.size());
  BOOST_FOREACH(Node::sharedPointer j, _nodes | boost::adaptors::map_values) {
    indexedNodes.push_back(j);
  }
  
  for (int nodeIdx = 0; nodeIdx < _nodes.size(); ++nodeIdx) {
    int dmaIdx = componentMap[nodeIdx];
    Dma::sharedPointer dma = newDmas[dmaIdx];
    dma->addJunction(boost::static_pointer_cast<Junction>(indexedNodes[nodeIdx]));
  }
  
  // finally, let the dma assemble its aggregators
  BOOST_FOREACH(const Dma::sharedPointer dma, newDmas) {
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

void Model::setDmaPipesToIgnore(vector<Pipe::sharedPointer> ignorePipes) {
  _dmaPipesToIgnore = ignorePipes;
}

vector<Pipe::sharedPointer> Model::dmaPipesToIgnore() {
  return _dmaPipesToIgnore;
}

#pragma mark - Controls

void Model::overrideControls() throw(RtxException) {
  _doesOverrideDemands = true;
}

#pragma mark - Element Accessors

void Model::addJunction(Junction::sharedPointer newJunction) {
  if (!newJunction) {
    std::cerr << "junction not specified" << std::endl;
    return;
  }
  _junctions.push_back(newJunction);
  add(newJunction);
}
void Model::addTank(Tank::sharedPointer newTank) {
  if (!newTank) {
    std::cerr << "tank not specified" << std::endl;
    return;
  }
  _tanks.push_back(newTank);
  add(newTank);
}
void Model::addReservoir(Reservoir::sharedPointer newReservoir) {
  if (!newReservoir) {
    std::cerr << "reservoir not specified" << std::endl;
    return;
  }
  _reservoirs.push_back(newReservoir);
  add(newReservoir);
}
void Model::addPipe(Pipe::sharedPointer newPipe) {
  if (!newPipe) {
    std::cerr << "pipe not specified" << std::endl;
    return;
  }
  _pipes.push_back(newPipe);
  add(newPipe);
}
void Model::addPump(Pump::sharedPointer newPump) {
  if (!newPump) {
    std::cerr << "pump not specified" << std::endl;
    return;
  }
  _pumps.push_back(newPump);
  add(newPump);
}
void Model::addValve(Valve::sharedPointer newValve) {
  if (!newValve) {
    std::cerr << "valve not specified" << std::endl;
    return;
  }
  _valves.push_back(newValve);
  add(newValve);
}

void Model::addDma(Dma::sharedPointer dma) {
  _dmas.push_back(dma);
  dma->setJunctionFlowUnits(this->flowUnits());
  Clock::sharedPointer hydClock(new Clock(hydraulicTimeStep()));
  //dma->demand()->setClock(hydClock);
  
}

// add to master lists
void Model::add(Junction::sharedPointer newJunction) {
  _nodes[newJunction->name()] = newJunction;
  _elements.push_back(newJunction);
}
void Model::add(Pipe::sharedPointer newPipe) {
  // manually add the pipe to the nodes' lists.
  newPipe->from()->addLink(newPipe);
  newPipe->to()->addLink(newPipe);
  // add to master link and element lists.
  _links[newPipe->name()] = newPipe;
  _elements.push_back(newPipe);
}

Link::sharedPointer Model::linkWithName(const string& name) {
  if ( _links.find(name) == _links.end() ) {
    Link::sharedPointer emptyLink;
    return emptyLink;
  }
  else {
    return _links[name];
  }
}
Node::sharedPointer Model::nodeWithName(const string& name) {
  if ( _nodes.find(name) == _nodes.end() ) {
    Node::sharedPointer emptyNode;
    return emptyNode;
  }
  else {
    return _nodes[name];
  }
}

std::vector<Element::sharedPointer> Model::elements() {
  return _elements;
}

std::vector<Dma::sharedPointer> Model::dmas() {
  return _dmas;
}
std::vector<Junction::sharedPointer> Model::junctions() {
  return _junctions;
}
std::vector<Tank::sharedPointer> Model::tanks() {
  return _tanks;
}
std::vector<Reservoir::sharedPointer> Model::reservoirs() {
  return _reservoirs;
}
std::vector<Pipe::sharedPointer> Model::pipes() {
  return _pipes;
}
std::vector<Pump::sharedPointer> Model::pumps() {
  return _pumps;
}
std::vector<Valve::sharedPointer> Model::valves() {
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
  boundaryReset = _boundaryResetClock->validTime(time);
  start = RTX_MAX(lastSimulationPeriod, boundaryReset);
  
  // run the simulation to the requested time.
  runExtendedPeriod(start, time);
}

void Model::runExtendedPeriod(time_t start, time_t end) {
  time_t simulationTime = start;
  time_t nextClockTime = start;
  time_t nextSimulationTime = start;
  time_t stepToTime = start;
  
  while (simulationTime < end) {
    
    // reset tanks?
    if (_tanksNeedReset) {
      _tanksNeedReset = false;
      BOOST_FOREACH(Tank::sharedPointer t, this->tanks()) {
        t->setResetLevelNextTime(true);
      }
    }
    
    // get parameters from the RTX elements, and pull them into the simulation
    setSimulationParameters(simulationTime);
    // simulate this period, find the next timestep boundary.
    solveSimulation(simulationTime);
    // tell each element to update its derived states (simulation-computed values)
    saveNetworkStates(simulationTime);
    // get time to next simulation period
    nextSimulationTime = nextHydraulicStep(simulationTime);
    nextClockTime = _regularMasterClock->timeAfter(simulationTime);
    stepToTime = RTX_MIN(nextClockTime, nextSimulationTime);
    
    // and step the simulation to that time.
    stepSimulation(stepToTime);
    simulationTime = currentSimulationTime();
    }
  
  
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
  BOOST_FOREACH(Junction::sharedPointer junc, this->junctions()) {
    junc->setInitialQuality(qual);
  }
  BOOST_FOREACH(Tank::sharedPointer tank, this->tanks()) {
    tank->setInitialQuality(qual);
  }
}

bool Model::tanksNeedReset() {
  return _tanksNeedReset;
}

void Model::setTanksNeedReset(bool reset) {
  _tanksNeedReset = reset;
}

void Model::setInitialJunctionQualityFromMeasurements(time_t time) {
  // Measured initial quality of Junctions and Tanks (Reservoirs are boundary conditions)
  // using nearest neighbor interpolation of quality measurements
  
  // junction measurements
  std::vector< std::pair<Junction::sharedPointer, double> > measuredJunctions;
  BOOST_FOREACH(Junction::sharedPointer junc, this->junctions()) {
    if (junc->doesHaveQualityMeasure()) {
      TimeSeries::sharedPointer qualityTS = junc->qualityMeasure();
      Point aPoint = qualityTS->pointAtOrBefore(time);
      if (aPoint.isValid) {
        measuredJunctions.push_back(make_pair(junc, aPoint.value));
      }
    }
  }
  // tank measurements
  BOOST_FOREACH(Tank::sharedPointer tank, this->tanks()) {
    if (tank->doesHaveQualityMeasure()) {
      TimeSeries::sharedPointer qualityTS = tank->qualityMeasure();
      Point aPoint = qualityTS->pointAtOrBefore(time);
      if (aPoint.isValid) {
        measuredJunctions.push_back(make_pair(tank, aPoint.value));
      }
    }
  }
  
  // Nearest neighbor interpolation by enumeration
  // junctions
  BOOST_FOREACH(Junction::sharedPointer junc, this->junctions()) {
    std::pair<Junction::sharedPointer, double> mjunc;
    double minDistance = DBL_MAX;
    double initQuality = 0;
    BOOST_FOREACH(mjunc, measuredJunctions) {
      double d = nodeDistanceXY(junc, mjunc.first);
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
  BOOST_FOREACH(Tank::sharedPointer tank, this->tanks()) {
    std::pair<Junction::sharedPointer, double> mjunc;
    double minDistance = DBL_MAX;
    double initQuality = 0;
    BOOST_FOREACH(mjunc, measuredJunctions) {
      double d = nodeDistanceXY(tank, mjunc.first);
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
  if (_record) {
    stream << "State Storage:" << endl << *_record;
  }
  
  if (_dmas.size() > 0) {
    stream << "Demand DMAs:" << endl;
    BOOST_FOREACH(Dma::sharedPointer d, _dmas) {
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
    BOOST_FOREACH(Dma::sharedPointer dma, this->dmas()) {
      dma->allocateDemandToJunctions(time);
    }
    // hydraulic junctions - set demand values.
    BOOST_FOREACH(Junction::sharedPointer junction, this->junctions()) {
      if (junction->doesHaveBoundaryFlow()) {
        // junction is separate from the allocation scheme
        double demandValue = Units::convertValue(junction->boundaryFlow()->pointAtOrBefore(time).value, junction->boundaryFlow()->units(), flowUnits());
        setJunctionDemand(junction->name(), demandValue);
      }
      else {
        double demandValue = Units::convertValue(junction->demand()->pointAtOrBefore(time).value, junction->demand()->units(), flowUnits());
        setJunctionDemand(junction->name(), demandValue);
      }
    }
  }
  
  // for reservoirs, set the boundary head and quality conditions
  BOOST_FOREACH(Reservoir::sharedPointer reservoir, this->reservoirs()) {
    if (reservoir->doesHaveBoundaryHead()) {
      // get the head measurement parameter, and pass it through as a state.
      double headValue = Units::convertValue(reservoir->boundaryHead()->pointAtOrBefore(time).value, reservoir->boundaryHead()->units(), headUnits());
      setReservoirHead( reservoir->name(), headValue );
    }
    if (reservoir->doesHaveBoundaryQuality()) {
      // get the quality measurement parameter, and pass it through as a state.
      double qualityValue = Units::convertValue(reservoir->boundaryQuality()->pointAtOrBefore(time).value, reservoir->boundaryQuality()->units(), qualityUnits());
      setReservoirQuality( reservoir->name(), qualityValue );
    }
  }
  
  // for tanks, set the boundary head, but only if the tank reset clock has fired.
  BOOST_FOREACH(Tank::sharedPointer tank, this->tanks()) {
    if (tank->doesResetLevelUsingClock() && tank->levelResetClock()->isValid(time) && tank->doesHaveHeadMeasure()) {
      double levelValue = Units::convertValue(tank->levelMeasure()->pointAtOrBefore(time).value, tank->levelMeasure()->units(), headUnits());
      // adjust for model limits (epanet rejects otherwise, for example)
      levelValue = (levelValue <= tank->maxLevel()) ? levelValue : tank->maxLevel();
      levelValue = (levelValue >= tank->minLevel()) ? levelValue : tank->minLevel();
      setTankLevel(tank->name(), levelValue);
    }
  }

  // or, set the boundary head if someone has specifically requested it
  BOOST_FOREACH(Tank::sharedPointer tank, this->tanks()) {
    if (tank->resetLevelNextTime() && tank->doesHaveHeadMeasure()) {
      double levelValue = Units::convertValue(tank->levelMeasure()->pointAtOrBefore(time).value, tank->levelMeasure()->units(), headUnits());
      // adjust for model limits (epanet rejects otherwise, for example)
      levelValue = (levelValue <= tank->maxLevel()) ? levelValue : tank->maxLevel();
      levelValue = (levelValue >= tank->minLevel()) ? levelValue : tank->minLevel();
      setTankLevel(tank->name(), levelValue);
      tank->setResetLevelNextTime(false);      
    }
  }

  // for valves, set status and setting
  BOOST_FOREACH(Valve::sharedPointer valve, this->valves()) {
    // status can affect settings and vice-versa; status rules
    Pipe::status_t status = Pipe::OPEN;
    if (valve->doesHaveStatusParameter()) {
      status = Pipe::status_t((int)(valve->statusParameter()->pointAtOrBefore(time).value));
      setPipeStatus( valve->name(), status );
    }
    if (valve->doesHaveSettingParameter() && status) {
      setValveSetting( valve->name(), valve->settingParameter()->pointAtOrBefore(time).value );
    }
  }
  
  // for pumps, set status and setting
  BOOST_FOREACH(Pump::sharedPointer pump, this->pumps()) {
    // status can affect settings and vice-versa; status rules
    Pipe::status_t status = Pipe::OPEN;
    if (pump->doesHaveStatusParameter()) {
      TimeSeries::sharedPointer statusTs = pump->statusParameter();
      status = Pipe::status_t((int)(statusTs->pointAtOrBefore(time).value));
      setPumpStatus( pump->name(), status );
    }
    if (pump->doesHaveSettingParameter() && status) {
      setPumpSetting( pump->name(), pump->settingParameter()->pointAtOrBefore(time).value );
    }
  }
  
  // for pipes, set status
  BOOST_FOREACH(Pipe::sharedPointer pipe, this->pipes()) {
    if (pipe->doesHaveStatusParameter()) {
      setPipeStatus( pipe->name(), Pipe::status_t((int)(pipe->statusParameter()->pointAtOrBefore(time).value)) );
    }
  }
  
  //////////////////////////////
  // water quality parameters //
  //////////////////////////////
  BOOST_FOREACH(Junction::sharedPointer j, this->junctions()) {
    if (j->doesHaveQualitySource()) {
      double quality = Units::convertValue(j->qualitySource()->pointAtOrBefore(time).value, j->qualitySource()->units(), qualityUnits());
      setJunctionQuality(j->name(), quality);
    }
  }
  
  
  
}



void Model::saveNetworkStates(time_t time) {
  
  // retrieve results from the hydraulic sim 
  // then insert the state values into elements' time series.
  
  // junctions, tanks, reservoirs
  BOOST_FOREACH(Junction::sharedPointer junction, junctions()) {
    double head;
    head = Units::convertValue(junctionHead(junction->name()), headUnits(), junction->head()->units());
    Point headPoint(time, head);
    junction->head()->insert(headPoint);
    
    double pressure;
    pressure = Units::convertValue(junctionPressure(junction->name()), pressureUnits(), junction->pressure()->units());
    
    // todo - more fine-grained quality data? at wq step resolution...
    double quality;
    quality = Units::convertValue(junctionQuality(junction->name()), this->qualityUnits(), junction->quality()->units());
    Point qualityPoint(time, quality);
    junction->quality()->insert(qualityPoint);
  }
  
  // only save demand states if 
  if (!_doesOverrideDemands) {
    BOOST_FOREACH(Junction::sharedPointer junction, junctions()) {
      double demand;
      demand = Units::convertValue(junctionDemand(junction->name()), flowUnits(), junction->demand()->units());
      Point demandPoint(time, demand);
      junction->demand()->insert(demandPoint);
    }
  }
  
  BOOST_FOREACH(Reservoir::sharedPointer reservoir, reservoirs()) {
    double head;
    head = Units::convertValue(junctionHead(reservoir->name()), headUnits(), reservoir->head()->units());
    Point headPoint(time, head);
    reservoir->head()->insert(headPoint);
  }
  
  BOOST_FOREACH(Tank::sharedPointer tank, tanks()) {
    double head;
    head = Units::convertValue(junctionHead(tank->name()), headUnits(), tank->head()->units());
    Point headPoint(time, head);
    tank->head()->insert(headPoint);
    tank->level()->point(headPoint.time);
    
    double quality;
    quality = Units::convertValue(junctionQuality(tank->name()), this->qualityUnits(), tank->quality()->units());
    Point qualityPoint(time, quality);
    tank->quality()->insert(qualityPoint);
    
  }
  
  // pipe elements
  BOOST_FOREACH(Pipe::sharedPointer pipe, pipes()) {
    double flow;
    flow = Units::convertValue(pipeFlow(pipe->name()), flowUnits(), pipe->flow()->units());
    Point aPoint(time, flow);
    pipe->flow()->insert(aPoint);
  }
  
  BOOST_FOREACH(Valve::sharedPointer valve, valves()) {
    double flow;
    flow = Units::convertValue(pipeFlow(valve->name()), flowUnits(), valve->flow()->units());
    Point aPoint(time, flow);
    valve->flow()->insert(aPoint);
  }
  
  // pump energy
  BOOST_FOREACH(Pump::sharedPointer pump, pumps()) {
    double flow;
    flow = Units::convertValue(pipeFlow(pump->name()), flowUnits(), pump->flow()->units());
    Point flowPoint(time, flow);
    pump->flow()->insert(flowPoint);
    
    double energy;
    energy = pumpEnergy(pump->name());
    Point energyPoint(time, energy);
    pump->energy()->insert(energyPoint);
  }
  
  // save the timestep information
  Point error(time, relativeError(time));
  _relativeError->insert(error);
  Point iterationCount(time, iterations(time));
  _iterations->insert(iterationCount);

}

void Model::setCurrentSimulationTime(time_t time) {
  _currentSimulationTime = time;
}
time_t Model::currentSimulationTime() {
  return _currentSimulationTime;
}

double Model::nodeDistanceXY(Node::sharedPointer n1, Node::sharedPointer n2) {
  std::pair<double, double> x1 = n1->coordinates();
  std::pair<double, double> x2 = n2->coordinates();
  return sqrt( pow(x1.first - x2.first, 2) + pow(x1.second - x2.second, 2) );
}


