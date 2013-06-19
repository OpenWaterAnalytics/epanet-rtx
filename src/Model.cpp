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

using namespace RTX;
using namespace std;


Model::Model() : _flowUnits(1), _headUnits(1) {
  // default reset clock is 24 hours, default reporting clock is 1 hour.
  // TODO -- configure boundary reset clock in a smarter way?
  _regularMasterClock.reset( new Clock(3600) );
  _boundaryResetClock = Clock::sharedPointer( new Clock(24 * 3600) );
  _relativeError.reset( new TimeSeries() );
  _iterations.reset( new TimeSeries() );
  
  _relativeError->setName("Relative Error");
  _iterations->setName("Iterations");
  _doesOverrideDemands = false;
  
  // defaults
  setFlowUnits(RTX_LITER_PER_SECOND);
  setHeadUnits(RTX_METER);
}
Model::~Model() {
  
}
std::ostream& RTX::operator<< (std::ostream &out, Model &model) {
  return model.toStream(out);
}

void Model::loadModelFromFile(const std::string& filename) throw(std::exception) {
  _modelFile = filename;
}

std::string Model::modelFile() {
  return _modelFile;
}

#pragma mark - Units
Units Model::flowUnits()    { return _flowUnits; }
Units Model::headUnits()    { return _headUnits; }
void Model::setFlowUnits(Units units)   { _flowUnits = units; }
void Model::setHeadUnits(Units units)   { _headUnits = units; }


#pragma mark - Storage
void Model::setStorage(PointRecord::sharedPointer record) {
  _record = record;

  std::vector< Element::sharedPointer > elements = this->elements();
  BOOST_FOREACH(Element::sharedPointer element, elements) {
    //std::cout << "setting storage for: " << element->name() << std::endl;
    element->setRecord(_record);
  }
  
  BOOST_FOREACH(Zone::sharedPointer zone, zones()) {
    zone->setRecord(_record);
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


#pragma mark - Demand Zones

void Model::initDemandZones(bool detectClosedLinks) {
  
  _zones.clear();
  
  // load up the node set
  std::set<Junction::sharedPointer> nodeSet;
  BOOST_FOREACH(Junction::sharedPointer junction, _junctions) {
    nodeSet.insert(junction);
  }
  BOOST_FOREACH(Tank::sharedPointer tank, _tanks) {
    nodeSet.insert(tank);
  }
  BOOST_FOREACH(Junction::sharedPointer reservoir, _reservoirs) {
    nodeSet.insert(reservoir);
  }
  int iZone = 0;
    
  // if the set is not empty, then there's work to be done:
  while (!nodeSet.empty()) {
    iZone++;
    
    // pick a random node, and build out that zone.
    set<Junction::sharedPointer>::iterator nodeIt;
    nodeIt = nodeSet.begin();
    Junction::sharedPointer rootNode = *nodeIt;
    
    string zoneName = boost::lexical_cast<string>(iZone);
    Zone::sharedPointer newZone(new Zone(zoneName));
    
    
    // specifiy the root node and populate the tree.
    try {
      newZone->enumerateJunctionsWithRootNode(rootNode,detectClosedLinks);
    } catch (exception &e) {
      cerr << "Zone could not be enumerated: " << e.what() << endl;
    } catch (...) {
      cerr << "Zone could not be enumerated, and I don't know why" << endl;
    }
    
    
    // get the list of junctions that were just added.
    vector<Junction::sharedPointer> addedJunctions = newZone->junctions();
    
    if (addedJunctions.size() < 1) {
      cerr << "Could not add any junctions to zone " << iZone << endl;
      continue;
    }
    
    // remove the added junctions from the nodeSet.
    BOOST_FOREACH(Junction::sharedPointer addedJunction, addedJunctions) {
      size_t changed = nodeSet.erase(addedJunction);
      if (changed != 1) {
        // whoops, something is wrong
        cerr << "Could not find junction in set: " << addedJunction->name() << endl;
      }
    }
    
    cout << "adding zone: " << *newZone << endl;
    this->addZone(newZone);
  }
  
  
  
  
  
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

void Model::addZone(Zone::sharedPointer zone) {
  _zones.push_back(zone);
  zone->setJunctionFlowUnits(this->flowUnits());
  Clock::sharedPointer hydClock(new Clock(hydraulicTimeStep()));
  zone->demand()->setClock(hydClock);
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

std::vector<Zone::sharedPointer> Model::zones() {
  return _zones;
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
    // get parameters from the RTX elements, and pull them into the simulation
    setSimulationParameters(simulationTime);
    // simulate this period, find the next timestep boundary.
    solveSimulation(simulationTime);
    // tell each element to update its derived states (simulation-computed values)
    saveHydraulicStates(simulationTime);
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


#pragma mark - Protected Methods

std::ostream& Model::toStream(std::ostream &stream) {
  // output shall be as follows:
  /*
   
   Model Properties:
      ## nodes (## tanks, ## reservoirs) in ## zones
      ## links (## pumps, ## valves)
   Time Steps:
      ##s (hydraulic), ##s (quality)
   State storage:
   [point record properties]
   
   */
  
  stream << "Model Properties" << endl;
  stream << "\t" << _nodes.size() << " nodes (" << _tanks.size() << " tanks, " << _reservoirs.size() << " reservoirs ) in " << _zones.size() << " zones" << endl;
  stream << "\t" << _links.size() << " links (" << _pumps.size() << " pumps, " << _valves.size() << " valves)" << endl;
  stream << "Time Steps:" << endl;
  stream << "\t" << hydraulicTimeStep() << "s (hydraulic), " << qualityTimeStep() << "s (quality)" << endl;
  if (_record) {
    stream << "State Storage:" << endl << *_record;
  }
  
  if (_zones.size() > 0) {
    stream << "Demand Zones:" << endl;
    BOOST_FOREACH(Zone::sharedPointer z, _zones) {
      stream << "## Zone: " << z->name() << endl;
      stream << *z << endl;
    }
  }
  
  
  return stream;
}

void Model::setSimulationParameters(time_t time) {
  // set all element parameters
  
  // allocate junction demands based on zones, and set the junction demand values in the model.
  if (_doesOverrideDemands) {
    BOOST_FOREACH(Zone::sharedPointer zone, this->zones()) {
      zone->allocateDemandToJunctions(time);
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
  // for reservoirs, set the boundary head condition
  BOOST_FOREACH(Reservoir::sharedPointer reservoir, this->reservoirs()) {
    if (reservoir->doesHaveBoundaryHead()) {
      // get the head measurement parameter, and pass it through as a state.
      double headValue = Units::convertValue(reservoir->boundaryHead()->pointAtOrBefore(time).value, reservoir->boundaryHead()->units(), headUnits());
      setReservoirHead( reservoir->name(), headValue );
    }
  }
  // for tanks, set the boundary head, but only if the tank reset clock has fired.
  BOOST_FOREACH(Tank::sharedPointer tank, this->tanks()) {
    if (tank->doesResetLevel() && tank->levelResetClock()->isValid(time) && tank->doesHaveHeadMeasure()) {
      double levelValue = Units::convertValue(tank->level()->pointAtOrBefore(time).value, tank->level()->units(), headUnits());
      setTankLevel(tank->name(), levelValue);
    }
  }
  
  // for valves, set status and setting
  BOOST_FOREACH(Valve::sharedPointer valve, this->valves()) {
    if (valve->doesHaveStatusParameter()) {
      setPipeStatus( valve->name(), Pipe::status_t((int)(valve->statusParameter()->pointAtOrBefore(time).value)) );
    }
    if (valve->doesHaveSettingParameter()) {
      setValveSetting( valve->name(), valve->settingParameter()->pointAtOrBefore(time).value );
    }
  }
  
  // for pumps, set status
  BOOST_FOREACH(Pump::sharedPointer pump, this->pumps()) {
    if (pump->doesHaveStatusParameter()) {
      setPumpStatus( pump->name(), Pipe::status_t((int)(pump->statusParameter()->pointAtOrBefore(time).value)) );
    }
  }
  
  
}



void Model::saveHydraulicStates(time_t time) {
  
  // retrieve results from the hydraulic sim 
  // then insert the state values into elements' time series.
  
  // junctions, tanks, reservoirs
  BOOST_FOREACH(Junction::sharedPointer junction, junctions()) {
    double head;
    head = Units::convertValue(junctionHead(junction->name()), headUnits(), junction->head()->units());
    Point headPoint(time, head, Point::good);
    junction->head()->insert(headPoint);
    
    // todo - more fine-grained quality data? at wq step resolution...
    double quality;
    quality = 0; // Units::convertValue(junctionQuality(junction->name()), RTX_MILLIGRAMS_PER_LITER, junction->quality()->units());
    Point qualityPoint(time, quality, Point::good);
    junction->quality()->insert(qualityPoint);
  }
  
  // only save demand states if 
  if (!_doesOverrideDemands) {
    BOOST_FOREACH(Junction::sharedPointer junction, junctions()) {
      double demand;
      demand = Units::convertValue(junctionDemand(junction->name()), flowUnits(), junction->demand()->units());
      Point demandPoint(time, demand, Point::good);
      junction->demand()->insert(demandPoint);
    }
  }
  
  BOOST_FOREACH(Reservoir::sharedPointer reservoir, reservoirs()) {
    double head;
    head = Units::convertValue(junctionHead(reservoir->name()), headUnits(), reservoir->head()->units());
    Point headPoint(time, head, Point::good);
    reservoir->head()->insert(headPoint);
  }
  
  BOOST_FOREACH(Tank::sharedPointer tank, tanks()) {
    double head;
    head = Units::convertValue(junctionHead(tank->name()), headUnits(), tank->head()->units());
    Point headPoint(time, head, Point::good);
    tank->head()->insert(headPoint);
  }
  
  // pipe elements
  BOOST_FOREACH(Pipe::sharedPointer pipe, pipes()) {
    double flow;
    flow = Units::convertValue(pipeFlow(pipe->name()), flowUnits(), pipe->flow()->units());
    Point aPoint(time, flow, Point::good);
    pipe->flow()->insert(aPoint);
  }
  
  BOOST_FOREACH(Valve::sharedPointer valve, valves()) {
    double flow;
    flow = Units::convertValue(pipeFlow(valve->name()), flowUnits(), valve->flow()->units());
    Point aPoint(time, flow, Point::good);
    valve->flow()->insert(aPoint);
  }
  
  // pump energy
  BOOST_FOREACH(Pump::sharedPointer pump, pumps()) {
    double flow;
    flow = Units::convertValue(pipeFlow(pump->name()), flowUnits(), pump->flow()->units());
    Point flowPoint(time, flow, Point::good);
    pump->flow()->insert(flowPoint);
    
    double energy;
    energy = pumpEnergy(pump->name());
    Point energyPoint(time, energy, Point::good);
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



