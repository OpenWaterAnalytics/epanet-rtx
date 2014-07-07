//
//  Dma.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#include <boost/foreach.hpp>
#include <boost/range/adaptors.hpp>

#include "Dma.h"
#include "ConstantTimeSeries.h"
#include "AggregatorTimeSeries.h"

#include <boost/config.hpp>
#include <algorithm>
#include <utility>
#include <boost/graph/adjacency_list.hpp>

using namespace RTX;
using namespace std;
using std::cout;

Dma::Dma(const std::string& name) : Element(name), _flowUnits(1) {
  this->setType(DMA);
  _flowUnits = RTX_LITER_PER_SECOND;
  // set to aggregator type because that's the most likely scenario.
  // presumably, we will use Dma::enumerateJunctionsWithRootNode to populate the aggregation.
  _demand.reset(new AggregatorTimeSeries() );
  _demand->setName("DMA " + name + " demand");
  _demand->setUnits(RTX_LITER_PER_SECOND);
}
Dma::~Dma() {
  
}

std::ostream& Dma::toStream(std::ostream &stream) {
  stream << "DMA: \"" << this->name() << "\"\n";
  stream << " - " << junctions().size() << " Junctions" << endl;
  stream << " - " << _tanks.size() << " Tanks" << endl;
  stream << " - " << _measuredBoundaryPipesDirectional.size() << " Measured Boundary Pipes" << endl;
  stream << " - " << _closedBoundaryPipesDirectional.size() << " Closed Boundary Pipes" << endl;
  stream << " - " << _measuredInteriorPipes.size() << " Measured Interior Pipes" << endl;
  stream << " - " << _closedInteriorPipes.size() << " Closed Interior Pipes" << endl;
  stream << "Closed Boundary Pipes:" << endl;
  std::pair<Pipe::sharedPointer, Pipe::direction_t> cp;
  BOOST_FOREACH(cp, _closedBoundaryPipesDirectional) {
    double multiplier = cp.second;
    Pipe::sharedPointer p = cp.first;
    string dir = (multiplier > 0)? "(+)" : "(-)";
    stream << "    " << dir << " " << p->name() << endl;
  }
  stream << "Time Series Aggregation:" << endl;
  stream << *_demand << endl;
  
  return stream;
}

void Dma::setRecord(PointRecord::sharedPointer record) {
  if (_demand) {
    _demand->setRecord(record);
  }
}

void Dma::setJunctionFlowUnits(RTX::Units units) {
  _flowUnits = units;
}

void Dma::addJunction(Junction::sharedPointer junction) {
  
  if (false) { //this->doesHaveJunction(junction)) {
    //cerr << "err: junction already exists" << endl;
  }
  else {
    _junctions.insert(junction);
    if (isTank(junction)) {
      _tanks.insert(boost::static_pointer_cast<Tank>(junction));
    }
    if (isBoundaryFlowJunction(junction)) {
      //this->removeJunction(j);
      _boundaryFlowJunctions.push_back(junction);
      //cout << "found boundary flow: " << j->name() << endl;
    }
  }
}

void Dma::removeJunction(Junction::sharedPointer junction) {
//  map<string, Junction::sharedPointer>::iterator jIt = _junctions.find(junction->name());
//  if (jIt != _junctions.end()) {
//    _junctions.erase(jIt);
//    cout << "removed junction: " << junction->name() << endl;
//  }
}

//
//list<Dma::sharedPointer> Dma::enumerateDmas(std::vector<Node::sharedPointer> nodes) {
//  
//  list<Dma::sharedPointer> dmas;
//  
//  bool stopForClosedPipes = true;
//  bool stopForMeasuredPipes = true;
//  bool stopForPumps = false;
//  
//  
//  // construct a boost::graph representation of the network graph
//  typedef boost::adjacency_list <boost::vecS, boost::vecS, boost::undirectedS> NetworkGraph;
//  NetworkGraph netGraph;
//  add_edge(0, 1, netGraph);
//  add_edge(1, 4, netGraph);
//  add_edge(4, 0, netGraph);
//  add_edge(2, 5, netGraph);
//  
//  
//  
//  return dmas;
//}
//


//void Dma::enumerateJunctionsWithRootNode(Junction::sharedPointer junction, bool stopAtClosedLinks, vector<Pipe::sharedPointer> ignorePipes) {
//  
//  bool doesContainReservoir = false;
//  
//  //cout << "Starting At Root Junction: " << junction->name() << endl;
//  
//  // breadth-first search.
//  deque<Junction::sharedPointer> candidateJunctions;
//  candidateJunctions.push_back(junction);
//  
//  while (!candidateJunctions.empty()) {
//    Junction::sharedPointer thisJ = candidateJunctions.front();
//    //cout << " - adding: " << thisJ->name() << endl;
//    this->addJunction(thisJ);
//    vector<Link::sharedPointer> connectedLinks = thisJ->links();
//    BOOST_FOREACH(Link::sharedPointer l, connectedLinks) {
//      // follow this link?
//      Pipe::sharedPointer p = boost::static_pointer_cast<Pipe>(l);
//      if (p->doesHaveFlowMeasure()) {
//        // look here - it's a potential dma perimeter pipe.
//        
//        // capture the pipe and direction - this list will be pruned later
//        Link::direction_t dir = p->directionRelativeToNode(thisJ);
//        _measuredBoundaryPipesDirectional.insert(make_pair(p, dir));
//        // should we ignore it?
//        if (std::find(ignorePipes.begin(), ignorePipes.end(), p) == ignorePipes.end()) { // not ignored
//          // not on the ignore list - so we assume that the pipe could actually be a boundary pipe. do not go through the pipe.
//          // if the pipe IS on the ignore list, then follow it with the BFS.
//          continue;
//        }
//        
//      }
//      else if ( stopAtClosedLinks && isAlwaysClosed(p) ) {
//        // stop here as well - a potential closed perimeter pipe
//        Pipe::direction_t dir = p->directionRelativeToNode(thisJ);
//        _closedBoundaryPipesDirectional.insert(make_pair(p, dir));
//        if (std::find(ignorePipes.begin(), ignorePipes.end(), p) == ignorePipes.end()) { // not ignored
//          continue;
//        }
//      }
//      
//      pair<Node::sharedPointer, Node::sharedPointer> nodes = l->nodes();
//      vector<Junction::sharedPointer> juncs;
//      juncs.push_back(boost::static_pointer_cast<Junction>(nodes.first));
//      juncs.push_back(boost::static_pointer_cast<Junction>(nodes.second));
//      BOOST_FOREACH(Junction::sharedPointer candidateJ, juncs) {
//        if (candidateJ != thisJ && !this->doesHaveJunction(candidateJ)) {
//          // add to follow list
//          candidateJunctions.push_back(candidateJ);
//        }
//      }
//    } // foreach connected link
//    candidateJunctions.pop_front();
//  }
//  
//  // cleanup orphaned pipes (pipes which have been identified as perimeters, but have both start/end nodes listed inside the dma)
//  // this set may include "ignored" pipes.
//  
//  map<Pipe::sharedPointer, Pipe::direction_t> measuredBoundaryPipesDirectional = measuredBoundaryPipes();
//  BOOST_FOREACH(Pipe::sharedPointer p, measuredBoundaryPipesDirectional | boost::adaptors::map_keys) {
//    if (this->doesHaveJunction(boost::static_pointer_cast<Junction>(p->from())) && this->doesHaveJunction(boost::static_pointer_cast<Junction>(p->to()))) {
//      //cout << "removing orphaned pipe: " << p->name() << endl;
//      _measuredBoundaryPipesDirectional.erase(p);
//      _measuredInteriorPipes.push_back(p);
//    }
//  }
//  map<Pipe::sharedPointer, Pipe::direction_t> closedBoundaryPipesDirectional = closedBoundaryPipes();
//  BOOST_FOREACH(Pipe::sharedPointer p, closedBoundaryPipesDirectional | boost::adaptors::map_keys) {
//    if (this->doesHaveJunction(boost::static_pointer_cast<Junction>(p->from())) && this->doesHaveJunction(boost::static_pointer_cast<Junction>(p->to()))) {
//      //cout << "removing orphaned pipe: " << p->name() << endl;
//      _closedBoundaryPipesDirectional.erase(p);
//      _closedInteriorPipes.push_back(p);
//    }
//  }
//
//  // separate junctions into:
//  // -- demand junctions
//  // -- boundary flow junctions
//  // -- storage tanks
//  
//  
//  BOOST_FOREACH(Junction::sharedPointer j, _junctions) {
//    // is this a reservoir? if so, that's bad news -- we can't compute a control volume. the volume is infinite.
//    if (j->type() == Element::RESERVOIR) {
//      doesContainReservoir = true;
//    }
//    // check if it's a tank or metered junction
//    if (isTank(j)) {
//      //this->removeJunction(j);
//      _tanks.push_back(boost::static_pointer_cast<Tank>(j));
//      //cout << "found tank: " << j->name() << endl;
//    }
//    else if (isBoundaryFlowJunction(j)) {
//      //this->removeJunction(j);
//      _boundaryFlowJunctions.push_back(j);
//      //cout << "found boundary flow: " << j->name() << endl;
//    }
//    
//  }
//  
//  
//  if (!doesContainReservoir) {
//    // assemble the aggregated demand time series
//    
//    AggregatorTimeSeries::sharedPointer dmaDemand( new AggregatorTimeSeries() );
//    dmaDemand->setUnits(RTX_GALLON_PER_MINUTE);
//    dmaDemand->setName("DMA " + this->name() + " demand");
//    BOOST_FOREACH(Tank::sharedPointer t, _tanks) {
//      dmaDemand->addSource(t->flowMeasure(), -1.);
//    }
//    /* boundary flows are accounted for in the allocation method
//     BOOST_FOREACH(Junction::sharedPointer j, _boundaryFlowJunctions) {
//     dmaDemand->addSource(j->boundaryFlow(), -1.);
//     }
//     */
//    
//    BOOST_FOREACH(pipeDirPair_t pd, _measuredBoundaryPipesDirectional) {
//      Pipe::sharedPointer p = pd.first;
//      Pipe::direction_t dir = pd.second;
//      double dirMult = ( dir == Pipe::inDirection ? 1. : -1. );
//      dmaDemand->addSource(p->flowMeasure(), dirMult);
//    }
//    
//    this->setDemand(dmaDemand);
//  }
//  else {
//    ConstantTimeSeries::sharedPointer constDma(new ConstantTimeSeries());
//    constDma->setName("Zero Demand");
//    constDma->setValue(0.);
//    constDma->setUnits(RTX_GALLON_PER_MINUTE);
//    this->setDemand(constDma);
//  }
//  
//  
//  
//  //cout << this->name() << " dma Description:" << endl;
//  //cout << *this->demand() << endl;
//  
//}


void Dma::initDemandTimeseries(const set<Pipe::sharedPointer> &boundarySet) {
  
  // we've supplied a list of candidate boundary pipes. prune the list of pipes that don't connect to this dma.
  BOOST_FOREACH(const Pipe::sharedPointer p, boundarySet) {
    Junction::sharedPointer j1, j2;
    j1 = boost::static_pointer_cast<Junction>(p->from());
    j2 = boost::static_pointer_cast<Junction>(p->to());
    
    bool j1member = this->doesHaveJunction(j1);
    bool j2member = this->doesHaveJunction(j2);
    
    if (j1member || j2member) {
      if (!j1member || !j2member) {
        // only one side connects to me. it's a boundary
        Junction::sharedPointer myJ = j1member ? j1 : j2;
        Pipe::direction_t jDir = p->directionRelativeToNode(myJ);
        
        // figure out why this pipe is included here. is it flow measured? is it closed?
        if (p->doesHaveFlowMeasure()) {
          _measuredBoundaryPipesDirectional.push_back(make_pair(p, jDir));
        }
        else if (p->fixedStatus() == Pipe::CLOSED) {
          _closedBoundaryPipesDirectional.push_back(make_pair(p, jDir));
        }
        else {
          cerr << "could not resolve reason for boundary pipe inclusion" << endl;
        }
        
      }
      else {  // completely internal
        if (p->doesHaveFlowMeasure()) {
          _measuredInteriorPipes.push_back(p);
        }
        else if (p->fixedStatus() == Pipe::CLOSED) {
          _closedInteriorPipes.push_back(p);
        }
        else {
          cerr << "could not resolve reason for internal pipe inclusion" << endl;
        }
      }
      
    } // pipe --> no junctions are members of this dma
  } // for each candidate boundary pipe
  
  
  
  
  
  // separate junctions into:
  // -- demand junctions
  // -- boundary flow junctions
  // -- storage tanks
  
  bool doesContainReservoir = false;
  
  BOOST_FOREACH(Junction::sharedPointer j, _junctions) {
    // is this a reservoir? if so, that's bad news -- we can't compute a control volume. the volume is infinite.
    if (j->type() == Element::RESERVOIR) {
      doesContainReservoir = true;
    }
  }
  
  
  if (!doesContainReservoir) {
    // assemble the aggregated demand time series
    
    AggregatorTimeSeries::sharedPointer dmaDemand( new AggregatorTimeSeries() );
    dmaDemand->setUnits(RTX_GALLON_PER_MINUTE);
    dmaDemand->setName("DMA " + this->name() + " demand");
    BOOST_FOREACH(Tank::sharedPointer t, _tanks) {
      dmaDemand->addSource(t->flowMeasure(), -1.);
    }
    /* boundary flows are accounted for in the allocation method
     BOOST_FOREACH(Junction::sharedPointer j, _boundaryFlowJunctions) {
     dmaDemand->addSource(j->boundaryFlow(), -1.);
     }
     */
    
    BOOST_FOREACH(pipeDirPair_t pd, _measuredBoundaryPipesDirectional) {
      Pipe::sharedPointer p = pd.first;
      Pipe::direction_t dir = pd.second;
      double dirMult = ( dir == Pipe::inDirection ? 1. : -1. );
      dmaDemand->addSource(p->flowMeasure(), dirMult);
    }
    
    this->setDemand(dmaDemand);
  }
  else {
    ConstantTimeSeries::sharedPointer constDma(new ConstantTimeSeries());
    constDma->setName("Zero Demand");
    constDma->setValue(0.);
    constDma->setUnits(RTX_GALLON_PER_MINUTE);
    this->setDemand(constDma);
  }
  
  
}


bool Dma::isAlwaysClosed(Pipe::sharedPointer pipe) {
  return ((pipe->fixedStatus() == Pipe::CLOSED) && (pipe->type() != Element::PUMP));
}

bool Dma::isTank(Junction::sharedPointer junction) {
  return (junction->type() == Element::TANK);
}

bool Dma::isBoundaryFlowJunction(Junction::sharedPointer junction) {
  return (junction->doesHaveBoundaryFlow());
}

/* deprecated
void Dma::followJunction(Junction::sharedPointer junction) {
  // don't let us add the same junction twice.
  if (!junction || findJunction(junction->name())) {
    return;
  }
  
  cout << "adding junction " << junction->name() << std::endl;
  // perform dfs
  // add the junction to my list
  addJunction(junction);
  
  // see if the junction is a tank -- if so, add in the tank's flowrate.
  if (junction->type() == Element::TANK) {
    Tank::sharedPointer thisTank = boost::static_pointer_cast<Tank>(junction);
    TimeSeries::sharedPointer flow = thisTank->flowMeasure();
    
    // flow is positive into the tank (out of the dma), so its sign for demand aggregation purposes should be negative.
    AggregatorTimeSeries::sharedPointer dmaDemand = boost::static_pointer_cast<AggregatorTimeSeries>(this->demand());
    cout << "dma " << this->name() << " : adding tank source : " << flow->name() << endl;
    dmaDemand->addSource(flow, -1.);
  }
  
  
  // for each link connected to the junction, follow it and add its junctions
  BOOST_FOREACH(Link::sharedPointer link, junction->links()) {
    cout << "... examining pipe " << link->name() << endl;
    Pipe::sharedPointer pipe = boost::static_pointer_cast<Pipe>(link);
    
    // get the link direction. into the dma is positive.
    bool directionIsOut = (junction == pipe->from());
    
    // sanity
    if (!directionIsOut && junction != pipe->to()) {
      cerr << "Could not resolve start/end node(s) for pipe: " << pipe->name() << endl;
      continue;
    }
    
    // make sure we can follow this link,
    // then get this link's nodes
    if ( !(pipe->doesHaveFlowMeasure()) ) {
      // follow each of the link's nodes.
      if (directionIsOut) {
        followJunction(boost::static_pointer_cast<Junction>( pipe->to() ) );
      }
      else {
        followJunction(boost::static_pointer_cast<Junction>( pipe->from() ) );
      }
    }
    else {
      // we have found a measurement.
      // add it to the control volume calculation.
      double direction = (directionIsOut? -1. : 1.);
      AggregatorTimeSeries::sharedPointer dmaDemand = boost::static_pointer_cast<AggregatorTimeSeries>(this->demand());
      if (!dmaDemand) {
        cerr << "dma time series wrong type: " << *(this->demand()) << endl;
      }
      cout << "dma " << this->name() << " : adding source " << pipe->flowMeasure()->name() << endl;
      dmaDemand->addSource(pipe->flowMeasure(), direction);
    }
  }
  
}
*/

Junction::sharedPointer Dma::findJunction(std::string name) {
  BOOST_FOREACH(Junction::sharedPointer j, _junctions) {
    if (RTX_STRINGS_ARE_EQUAL(j->name(), name)) {
      return j;
    }
  }
  
  Junction::sharedPointer aJunction;
  return aJunction;
}

bool Dma::doesHaveJunction(Junction::sharedPointer j) {
//  return (find(_junctions.begin(), _junctions.end(), j) != _junctions.end());
  return _junctions.find(j) != _junctions.end();
}

std::set<Junction::sharedPointer> Dma::junctions() {

  return _junctions;
}

std::set<Tank::sharedPointer> Dma::tanks() {
  return _tanks;
}

std::vector<Dma::pipeDirPair_t> Dma::measuredBoundaryPipes() {
  return _measuredBoundaryPipesDirectional;
}

std::vector<Dma::pipeDirPair_t> Dma::closedBoundaryPipes() {
  return _closedBoundaryPipesDirectional;
}

std::vector<Pipe::sharedPointer> Dma::closedInteriorPipes() {
  return _closedInteriorPipes;
}

std::vector<Pipe::sharedPointer> Dma::measuredInteriorPipes() {
  return _measuredInteriorPipes;
}

bool Dma::isMeasuredPipe(Pipe::sharedPointer pipe) {
  
  BOOST_FOREACH(const pipeDirPair_t& pdp, _measuredBoundaryPipesDirectional) {
    if (pdp.first == pipe) {
      return true;
    }
  }
  
  std::vector<Pipe::sharedPointer>::iterator it;
  for (it = _measuredInteriorPipes.begin(); it != _measuredInteriorPipes.end(); ++it) {
    if (*it == pipe) {
      return true;
    }
  }
  
  return false;
}

bool Dma::isClosedPipe(Pipe::sharedPointer pipe) {

  BOOST_FOREACH(const pipeDirPair_t& pdp, _closedBoundaryPipesDirectional) {
    if (pdp.first == pipe) {
      return true;
    }
  }
  
  std::vector<Pipe::sharedPointer>::iterator it;
  for (it = _closedInteriorPipes.begin(); it != _closedInteriorPipes.end(); ++it) {
    if (*it == pipe) {
      return true;
    }
  }
  
  return false;
}

void Dma::setDemand(TimeSeries::sharedPointer demand) {
  if (demand->units().isSameDimensionAs(RTX_CUBIC_METER_PER_SECOND)) {
    _demand = demand;
  }
  else {
    cerr << "could not set demand -- dimension must be volumetric rate" << endl;
  }
}

TimeSeries::sharedPointer Dma::demand() {
  return _demand;
}

void Dma::allocateDemandToJunctions(time_t time) {
  // get each node's base demand for the current time
  // add the base demands together. this is the total base demand.
  // get the input demand value for the current time - from the demand() method
  // compute the global scaling factor
  // apply this to each base demand
  // add each scaled base demand to the appropriate node's demand pattern.
  
  typedef std::map< std::string, Junction::sharedPointer > JunctionMapType;
  double totalBaseDemand = 0;
  double dmaDemand = 0;
  double allocableDemand = 0;
  double meteredDemand = 0;
  
  Units myUnits = demand()->units();
  Units modelUnits = _flowUnits;
  
  
  // if the junction has a boundary flow condition, add it to the "known" demand pool.
  // otherwise, add the nominal base demand to the totalBaseDemand pool.
  BOOST_FOREACH(Junction::sharedPointer junction, _junctions) {
    
    if ( junction->doesHaveBoundaryFlow() ) {
      Point dp = junction->boundaryFlow()->point(time);
      double dval = dp.isValid ? dp.value : junction->boundaryFlow()->pointBefore(time).value;
      double demand = Units::convertValue(dval, junction->boundaryFlow()->units(), myUnits);
      meteredDemand += demand;
    }
    else {
      double demand = Units::convertValue(junction->baseDemand(), modelUnits, myUnits);
      totalBaseDemand += demand;
    }
    
  }
  
  // now we have the total (nominal) base demand for the dma.
  // total demand for the dma (includes metered and unmetered) -- already in myUnits.
  Point dPoint = this->demand()->pointAtOrBefore(time);
  
  dmaDemand = ( dPoint.isValid ? dPoint.value : this->demand()->pointBefore(time).value );
  allocableDemand = dmaDemand - meteredDemand; // the total unmetered demand
  /*
  cout << "-------------------" << endl;
  cout << "dma: " << this->name() << endl;
  cout << "time: " << time << endl;
  cout << "dma demand: " << dmaDemand << endl;
  cout << "metered: " << meteredDemand << endl;
  cout << "allocable: " << allocableDemand << endl;
  cout << "dma base demand: " << totalBaseDemand << endl;
  */
  // set the demand values for unmetered junctions, according to their base demands.
  BOOST_FOREACH(Junction::sharedPointer junction, _junctions) {
    if (junction->doesHaveBoundaryFlow()) {
      // junction does have boundary flow...
      // just need to copy the boundary flow into the junction's demand time series
      Point dp = junction->boundaryFlow()->pointAtOrBefore(time);
      Point newDemandPoint = Point::convertPoint(dp, junction->boundaryFlow()->units(), junction->demand()->units());
      junction->demand()->insert( newDemandPoint );
    }
    else {
      // junction relies on us to set its demand value... but only set it if the total base demand > 0
      if (totalBaseDemand > 0) {
        double baseDemand = Units::convertValue(junction->baseDemand(), modelUnits, myUnits);
        double newDemand = baseDemand * ( allocableDemand / totalBaseDemand );
        Point demandPoint(time, newDemand);
        junction->demand()->insert( Point::convertPoint(demandPoint, myUnits, junction->demand()->units()) );
      }
    }
  }
  
}

