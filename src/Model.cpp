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
  _boundaryResetClock = Clock::_sp( new Clock(24 * 3600) );
  _relativeError.reset( new TimeSeries() );
  _iterations.reset( new TimeSeries() );
  
  _relativeError->setName("Relative Error");
  _iterations->setName("Iterations");
  _doesOverrideDemands = false;
  _shouldRunWaterQuality = false;
  
  _dmaShouldDetectClosedLinks = false;
  _dmaPipesToIgnore = vector<Pipe::_sp>();
  
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
  BOOST_FOREACH(Junction::_sp j, this->junctions()) {
    j->demand()->setUnits(units);
  }
  BOOST_FOREACH(Tank::_sp t, this->tanks()) {
    t->flowMeasure()->setUnits(units);
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
}

#pragma mark - Storage
void Model::setStorage(PointRecord::_sp record) {
  _record = record;

  std::vector< Element::_sp > elements = this->elements();
  BOOST_FOREACH(Element::_sp element, elements) {
    //std::cout << "setting storage for: " << element->name() << std::endl;
    element->setRecord(_record);
  }
  
  BOOST_FOREACH(Dma::_sp dma, dmas()) {
    dma->setRecord(_record);
  }
  
  _relativeError->setRecord(_record);
  _iterations->setRecord(_record);
  
}

void Model::setParameterSource(PointRecord::_sp record) {

  BOOST_FOREACH(Junction::_sp junction, this->junctions()) {
    if (junction->doesHaveBoundaryFlow()) {
      junction->boundaryFlow()->setRecord(record);
    }
    if (junction->doesHaveHeadMeasure()) {
      junction->headMeasure()->setRecord(record);
    }
  }

  BOOST_FOREACH(Reservoir::_sp reservoir, this->reservoirs()) {
    if (reservoir->doesHaveBoundaryHead()) {
      reservoir->boundaryHead()->setRecord(record);
    }
  }

  BOOST_FOREACH(Valve::_sp valve, this->valves()) {
    if (valve->doesHaveStatusParameter()) {
      valve->statusParameter()->setRecord(record);
    }
    if (valve->doesHaveSettingParameter()) {
      valve->settingParameter()->setRecord(record);
    }
  }
  
  BOOST_FOREACH(Pump::_sp pump, this->pumps()) {
    if (pump->doesHaveStatusParameter()) {
      pump->statusParameter()->setRecord(record);
    }
  }
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
  bool success;
  
  while (simulationTime < end) {
    
    // reset tanks?
    if (_tanksNeedReset) {
      _tanksNeedReset = false;
      BOOST_FOREACH(Tank::_sp t, this->tanks()) {
        t->setResetLevelNextTime(true);
      }
    }
    
    // get parameters from the RTX elements, and pull them into the simulation
    setSimulationParameters(simulationTime);
    // simulate this period, find the next timestep boundary.
    success = solveSimulation(simulationTime);
    if (success) {
      // simulation succeeded
      
//      // debugging the DMA demands
//      BOOST_FOREACH(Dma::_sp dma, this->dmas()) {
//        cout << endl << "*** DMA " << dma->name() << " ***" << endl;
//        
//        double dmaDemand = 0.0;
//        double dmaInflow = 0.0;
//        double dmaOutflow = 0.0;
//        double tankDemand = 0.0;
//        double reservoirDemand = 0.0;
//        
//        // what should be there
//        Units myUnits = dma->demand()->units();
//        Units modelUnits = _flowUnits;
//        Point dPoint = dma->demand()->pointAtOrBefore(simulationTime);
//        double dmaDemandFromData = Units::convertValue(dPoint.value, myUnits, modelUnits);
//        
//        // demand accumulation by element type
//        set<Junction::_sp> juncs = dma->junctions();
//        BOOST_FOREACH(Junction::_sp junc, juncs) {
//          double juncDemand = junctionDemand(junc->name());
//          if (junc->type() == Element::JUNCTION) {
//            //          cout << "      " << junc->name() << ": " << juncDemand << endl;
//            dmaDemand += juncDemand;
//          }
//          else if (junc->type() == Element::RESERVOIR) {
//            //          cout << "      RESERVOIR " << junc->name() << ": " << juncDemand << endl;
//            reservoirDemand += juncDemand;
//          }
//          else if (junc->type() == Element::TANK) {
//                      cout << "      TANK " << junc->name() << ": " << juncDemand << endl;
//            tankDemand += juncDemand;
//          }
//        }
//        
//        // accumulate the boundary flows
//        vector<Dma::pipeDirPair_t> boundaryPipes = dma->measuredBoundaryPipes();
//        BOOST_FOREACH(const Dma::pipeDirPair_t& pd, boundaryPipes) {
//          Pipe::_sp p = pd.first;
//          double q = pipeFlow(p->name());
//          Pipe::direction_t dir = pd.second;
//          if (dir == Pipe::inDirection) {
//            dmaInflow += q;
//          }
//          else {
//            dmaOutflow += q;
//          }
//        }
//        
//        cout << "TOTAL DMA DEMAND (SCADA): " << dmaDemandFromData << endl;
//        cout << "TOTAL DMA DEMAND (EPANET): " << dmaDemand << endl;
//        cout << "   Inflow: " << dmaInflow << endl;
//        cout << "   Outflow: " << dmaOutflow << endl;
//        cout << "   Tank Demand: " << tankDemand << endl;
//        cout << "   Reservoir Demand: " << reservoirDemand << endl;
//        cout << "   NET Demand (In - Out - Demand): " << dmaInflow - dmaOutflow - tankDemand - reservoirDemand - dmaDemand << endl;
//      }
      
      // tell each element to update its derived states (simulation-computed values)
      if (_regularMasterClock->isValid(simulationTime)) {
        saveNetworkStates(simulationTime);
      }
      // get time to next simulation period
      nextSimulationTime = nextHydraulicStep(simulationTime);
      nextClockTime = _regularMasterClock->timeAfter(simulationTime);
      stepToTime = RTX_MIN(nextClockTime, nextSimulationTime);
      
      // and step the simulation to that time.
      stepSimulation(stepToTime);
      simulationTime = currentSimulationTime();
      
      cout << "Simulation time :: " << (double)(simulationTime-start)/60./60. << " hours" << endl;
    }
    else {
      // simulation failed -- advance the time and restart
      nextClockTime = _regularMasterClock->timeAfter(simulationTime);
      simulationTime = nextClockTime;
      setCurrentSimulationTime(simulationTime);
      _tanksNeedReset = true;
    }
    
  } // simulation loop
  
  
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
  std::vector< std::pair<Junction::_sp, double> > measuredJunctions;
  BOOST_FOREACH(Junction::_sp junc, this->junctions()) {
    if (junc->doesHaveQualityMeasure()) {
      TimeSeries::_sp qualityTS = junc->qualityMeasure();
      Point aPoint = qualityTS->pointAtOrBefore(time);
      if (aPoint.isValid) {
        measuredJunctions.push_back(make_pair(junc, aPoint.value));
      }
    }
  }
  // tank measurements
  BOOST_FOREACH(Tank::_sp tank, this->tanks()) {
    if (tank->doesHaveQualityMeasure()) {
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
  BOOST_FOREACH(Tank::_sp tank, this->tanks()) {
    std::pair<Junction::_sp, double> mjunc;
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
    BOOST_FOREACH(Dma::_sp dma, this->dmas()) {
      dma->allocateDemandToJunctions(time);
    }
    // hydraulic junctions - set demand values.
    BOOST_FOREACH(Junction::_sp junction, this->junctions()) {
      if (junction->doesHaveBoundaryFlow()) {
        // junction is separate from the allocation scheme (but allocateDemandToJunctions already inserts this into demand() series?)
        Point p = junction->boundaryFlow()->pointAtOrBefore(time);
        if (p.isValid) {
          double demandValue = Units::convertValue(p.value, junction->boundaryFlow()->units(), flowUnits());
          setJunctionDemand(junction->name(), demandValue);
        }
        else {
          cerr << "ERR: Invalid boundary flow point for Junction " << junction->name() << " at time " << time << endl;
        }
      }
      else {
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
  }
  
  // for reservoirs, set the boundary head and quality conditions
  BOOST_FOREACH(Reservoir::_sp reservoir, this->reservoirs()) {
    if (reservoir->doesHaveBoundaryHead()) {
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
    if (reservoir->doesHaveBoundaryQuality()) {
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
  
  // for tanks, set the boundary head, but only if the tank reset clock has fired.
  BOOST_FOREACH(Tank::_sp tank, this->tanks()) {
    if (tank->doesResetLevelUsingClock() && tank->levelResetClock()->isValid(time) && tank->doesHaveHeadMeasure()) {
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

  // or, set the boundary head if someone has specifically requested it
  BOOST_FOREACH(Tank::_sp tank, this->tanks()) {
    if (tank->resetLevelNextTime() && tank->doesHaveHeadMeasure()) {
      Point p = tank->levelMeasure()->pointAtOrBefore(time);
      if (p.isValid) {
        double levelValue = Units::convertValue(p.value, tank->levelMeasure()->units(), headUnits());
        // adjust for model limits (epanet rejects otherwise, for example)
        levelValue = (levelValue <= tank->maxLevel()) ? levelValue : tank->maxLevel();
        levelValue = (levelValue >= tank->minLevel()) ? levelValue : tank->minLevel();
        setTankLevel(tank->name(), levelValue);
        tank->setResetLevelNextTime(false);
      }
      else {
        cerr << "ERR: Invalid head point for Tank " << tank->name() << " at time " << time << endl;
      }
    }
  }

  // for valves, set status and setting
  BOOST_FOREACH(Valve::_sp valve, this->valves()) {
    // status can affect settings and vice-versa; status rules
    Pipe::status_t status = Pipe::OPEN;
    if (valve->doesHaveStatusParameter()) {
      Point p = valve->statusParameter()->pointAtOrBefore(time);
      if (p.isValid) {
        status = Pipe::status_t((int)(p.value));
        setPipeStatus( valve->name(), status );
      }
      else {
        cerr << "ERR: Invalid status point for Valve " << valve->name() << " at time " << time << endl;
      }
    }
    if (valve->doesHaveSettingParameter() && status) {
      Point p = valve->settingParameter()->pointAtOrBefore(time);
      if (p.isValid) {
        // TODO -- set units based on type of valve (pressure or flow model units)
        setValveSetting( valve->name(), p.value );
      }
      else {
        cerr << "ERR: Invalid setting point for Valve " << valve->name() << " at time " << time << endl;
      }
    }
  }
  
  // for pumps, set status and setting
  BOOST_FOREACH(Pump::_sp pump, this->pumps()) {
    // status can affect settings and vice-versa; status rules
    Pipe::status_t status = Pipe::OPEN;
    if (pump->doesHaveStatusParameter()) {
      Point p = pump->statusParameter()->pointAtOrBefore(time);
      if (p.isValid) {
        status = Pipe::status_t((int)(p.value));
        setPumpStatus( pump->name(), status );
      }
      else {
        cerr << "ERR: Invalid status point for Pump " << pump->name() << " at time " << time << endl;
      }
    }
    if (pump->doesHaveSettingParameter() && status) {
      Point p = pump->settingParameter()->pointAtOrBefore(time);
      if (p.isValid) {
        setPumpSetting( pump->name(), p.value );
      }
      else {
        cerr << "ERR: Invalid setting point for Pump " << pump->name() << " at time " << time << endl;
      }
    }
  }
  
  // for pipes, set status
  BOOST_FOREACH(Pipe::_sp pipe, this->pipes()) {
    if (pipe->doesHaveStatusParameter()) {
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
  BOOST_FOREACH(Junction::_sp j, this->junctions()) {
    if (j->doesHaveQualitySource()) {
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



void Model::saveNetworkStates(time_t time) {
  
  // retrieve results from the hydraulic sim 
  // then insert the state values into elements' time series.
  
  // junctions, tanks, reservoirs
  BOOST_FOREACH(Junction::_sp junction, junctions()) {
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
    BOOST_FOREACH(Junction::_sp junction, junctions()) {
      double demand;
      demand = Units::convertValue(junctionDemand(junction->name()), flowUnits(), junction->demand()->units());
      Point demandPoint(time, demand);
      junction->demand()->insert(demandPoint);
    }
  }
  
  BOOST_FOREACH(Reservoir::_sp reservoir, reservoirs()) {
    double head;
    head = Units::convertValue(junctionHead(reservoir->name()), headUnits(), reservoir->head()->units());
    Point headPoint(time, head);
    reservoir->head()->insert(headPoint);
  }
  
  BOOST_FOREACH(Tank::_sp tank, tanks()) {
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
  BOOST_FOREACH(Pipe::_sp pipe, pipes()) {
    double flow;
    flow = Units::convertValue(pipeFlow(pipe->name()), flowUnits(), pipe->flow()->units());
    Point aPoint(time, flow);
    pipe->flow()->insert(aPoint);
  }
  
  BOOST_FOREACH(Valve::_sp valve, valves()) {
    double flow;
    flow = Units::convertValue(pipeFlow(valve->name()), flowUnits(), valve->flow()->units());
    Point aPoint(time, flow);
    valve->flow()->insert(aPoint);
  }
  
  // pump energy
  BOOST_FOREACH(Pump::_sp pump, pumps()) {
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

double Model::nodeDistanceXY(Node::_sp n1, Node::_sp n2) {
  std::pair<double, double> x1 = n1->coordinates();
  std::pair<double, double> x2 = n2->coordinates();
  return sqrt( pow(x1.first - x2.first, 2) + pow(x1.second - x2.second, 2) );
}

vector<TimeSeries::_sp> Model::networkStatesWithMeasures() {
  vector<TimeSeries::_sp> measures;
  vector<Element::_sp> modelElements;
  modelElements = this->elements();
  
  BOOST_FOREACH(Element::_sp element, modelElements) {
    switch (element->type()) {
      case Element::JUNCTION:
      case Element::TANK:
      case Element::RESERVOIR: {
        Junction::_sp junc;
        junc = boost::static_pointer_cast<Junction>(element);
        if (junc->doesHaveHeadMeasure()) {
          measures.push_back(junc->head());
        }
        if (junc->doesHaveQualityMeasure()) {
          measures.push_back(junc->quality());
        }
        break;
      }
        
      case Element::PIPE:
      case Element::VALVE:
      case Element::PUMP: {
        Pipe::_sp pipe;
        pipe = boost::static_pointer_cast<Pipe>(element);
        if (pipe->doesHaveFlowMeasure()) {
          measures.push_back(pipe->flow());
        }
        break;
      }
        
      default:
        break;
    }
  }
  
  return measures;
}

void Model::setRecordForNetworkStatesWithMeasures(PointRecord::_sp pr) {
  vector<Element::_sp> modelElements;
  modelElements = this->elements();
  
  BOOST_FOREACH(Element::_sp element, modelElements) {
    switch (element->type()) {
      case Element::JUNCTION:
      case Element::TANK:
      case Element::RESERVOIR: {
        Junction::_sp junc;
        junc = boost::static_pointer_cast<Junction>(element);
        if (junc->doesHaveHeadMeasure()) {
          TimeSeries::_sp ts = junc->head();
          ts->setRecord(pr);
        }
        if (junc->doesHaveQualityMeasure()) {
          TimeSeries::_sp ts = junc->quality();
          ts->setRecord(pr);
        }
        break;
      }
        
      case Element::PIPE:
      case Element::VALVE:
      case Element::PUMP: {
        Pipe::_sp pipe;
        pipe = boost::static_pointer_cast<Pipe>(element);
        if (pipe->doesHaveFlowMeasure()) {
          TimeSeries::_sp ts = pipe->flow();
          ts->setRecord(pr);
        }
        break;
      }
        
      default:
        break;
    }
  }
}

void Model::setRecordForNetworkBoundariesAndMeasures(PointRecord::_sp pr) {
  vector<Element::_sp> modelElements;
  modelElements = this->elements();
  
  // connect the time series
  BOOST_FOREACH(Element::_sp element, modelElements) {
    switch (element->type()) {
      case Element::JUNCTION:
      case Element::TANK:
      case Element::RESERVOIR: {
        Junction::_sp junc;
        junc = boost::static_pointer_cast<Junction>(element);
        if (junc->doesHaveBoundaryFlow()) {
          TimeSeries::_sp ts = junc->boundaryFlow();
          ts->setRecord(pr);
        }
        if (junc->doesHaveHeadMeasure()) {
          TimeSeries::_sp ts = junc->headMeasure();
          ts->setRecord(pr);
        }
        if (junc->doesHaveQualityMeasure()) {
          TimeSeries::_sp ts = junc->qualityMeasure();
          ts->setRecord(pr);
        }
        if (junc->doesHaveQualitySource()) {
          TimeSeries::_sp ts = junc->qualitySource();
          ts->setRecord(pr);
        }
        break;
      }
        
      case Element::PIPE:
      case Element::VALVE:
      case Element::PUMP: {
        Pipe::_sp pipe;
        pipe = boost::static_pointer_cast<Pipe>(element);
        if (pipe->doesHaveFlowMeasure()) {
          TimeSeries::_sp ts = pipe->flowMeasure();
          ts->setRecord(pr);
        }
        if (pipe->doesHaveSettingParameter()) {
          TimeSeries::_sp ts = pipe->settingParameter();
          ts->setRecord(pr);
        }
        if (pipe->doesHaveStatusParameter()) {
          TimeSeries::_sp ts = pipe->statusParameter();
          ts->setRecord(pr);
        }
        break;
      }
        
      default:
        break;
    }
  }

}

