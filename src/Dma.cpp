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
#include <boost/algorithm/string/join.hpp>
#include <algorithm>
#include <utility>
#include <boost/graph/adjacency_list.hpp>

#include <openssl/sha.h>
#include <openssl/evp.h>

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

  for (auto cp : _closedBoundaryPipesDirectional) {
    double multiplier = cp.second;
    Pipe::_sp p = cp.first;
    string dir = (multiplier > 0)? "(+)" : "(-)";
    stream << "    " << dir << " " << p->name() << endl;
  }
  stream << "Time Series Aggregation:" << endl;
  stream << *_demand << endl;
  
  return stream;
}

void Dma::setRecord(PointRecord::_sp record) {
  if (_demand) {
    _demand->setRecord(record);
  }
}

void Dma::setJunctionFlowUnits(RTX::Units units) {
  _flowUnits = units;
}

void Dma::addJunction(Junction::_sp junction) {
  
  if (false) { //this->doesHaveJunction(junction)) {
    //cerr << "err: junction already exists" << endl;
  }
  else {
    _junctions.insert(junction);
    if (isTank(junction)) {
      _tanks.insert(std::static_pointer_cast<Tank>(junction));
    }
    if (isBoundaryFlowJunction(junction)) {
      //this->removeJunction(j);
      _boundaryFlowJunctions.push_back(junction);
      //cout << "found boundary flow: " << j->name() << endl;
    }
  }
}

void Dma::removeJunction(Junction::_sp junction) {
//  map<string, Junction::_sp>::iterator jIt = _junctions.find(junction->name());
//  if (jIt != _junctions.end()) {
//    _junctions.erase(jIt);
//    cout << "removed junction: " << junction->name() << endl;
//  }
}


bool Dma::doesContainReservoir() {
  bool hasReservoir = false;
  
  for(Junction::_sp j : _junctions) {
    // is this a reservoir? if so, that's bad news -- we can't compute a control volume. the volume is infinite.
    if (j->type() == Element::RESERVOIR) {
      hasReservoir = true;
    }
  }
  
  return hasReservoir;
}

//
//list<Dma::_sp> Dma::enumerateDmas(std::vector<Node::_sp> nodes) {
//  
//  list<Dma::_sp> dmas;
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


//void Dma::enumerateJunctionsWithRootNode(Junction::_sp junction, bool stopAtClosedLinks, vector<Pipe::_sp> ignorePipes) {
//  
//  bool doesContainReservoir = false;
//  
//  //cout << "Starting At Root Junction: " << junction->name() << endl;
//  
//  // breadth-first search.
//  deque<Junction::_sp> candidateJunctions;
//  candidateJunctions.push_back(junction);
//  
//  while (!candidateJunctions.empty()) {
//    Junction::_sp thisJ = candidateJunctions.front();
//    //cout << " - adding: " << thisJ->name() << endl;
//    this->addJunction(thisJ);
//    vector<Link::_sp> connectedLinks = thisJ->links();
//    for(Link::_sp l : connectedLinks) {
//      // follow this link?
//      Pipe::_sp p = std::static_pointer_cast<Pipe>(l);
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
//      pair<Node::_sp, Node::_sp> nodes = l->nodes();
//      vector<Junction::_sp> juncs;
//      juncs.push_back(std::static_pointer_cast<Junction>(nodes.first));
//      juncs.push_back(std::static_pointer_cast<Junction>(nodes.second));
//      for(Junction::_sp candidateJ : juncs) {
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
//  map<Pipe::_sp, Pipe::direction_t> measuredBoundaryPipesDirectional = measuredBoundaryPipes();
//  for(Pipe::_sp p : measuredBoundaryPipesDirectional | boost::adaptors::map_keys) {
//    if (this->doesHaveJunction(std::static_pointer_cast<Junction>(p->from())) && this->doesHaveJunction(std::static_pointer_cast<Junction>(p->to()))) {
//      //cout << "removing orphaned pipe: " << p->name() << endl;
//      _measuredBoundaryPipesDirectional.erase(p);
//      _measuredInteriorPipes.push_back(p);
//    }
//  }
//  map<Pipe::_sp, Pipe::direction_t> closedBoundaryPipesDirectional = closedBoundaryPipes();
//  for(Pipe::_sp p : closedBoundaryPipesDirectional | boost::adaptors::map_keys) {
//    if (this->doesHaveJunction(std::static_pointer_cast<Junction>(p->from())) && this->doesHaveJunction(std::static_pointer_cast<Junction>(p->to()))) {
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
//  for(Junction::_sp j : _junctions) {
//    // is this a reservoir? if so, that's bad news -- we can't compute a control volume. the volume is infinite.
//    if (j->type() == Element::RESERVOIR) {
//      doesContainReservoir = true;
//    }
//    // check if it's a tank or metered junction
//    if (isTank(j)) {
//      //this->removeJunction(j);
//      _tanks.push_back(std::static_pointer_cast<Tank>(j));
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
//    AggregatorTimeSeries::_sp dmaDemand( new AggregatorTimeSeries() );
//    dmaDemand->setUnits(RTX_GALLON_PER_MINUTE);
//    dmaDemand->setName("DMA " + this->name() + " demand");
//    for(Tank::_sp t : _tanks) {
//      dmaDemand->addSource(t->flowMeasure(), -1.);
//    }
//    /* boundary flows are accounted for in the allocation method
//     for(Junction::_sp j : _boundaryFlowJunctions) {
//     dmaDemand->addSource(j->boundaryFlow(), -1.);
//     }
//     */
//    
//    for(pipeDirPair_t pd : _measuredBoundaryPipesDirectional) {
//      Pipe::_sp p = pd.first;
//      Pipe::direction_t dir = pd.second;
//      double dirMult = ( dir == Pipe::inDirection ? 1. : -1. );
//      dmaDemand->addSource(p->flowMeasure(), dirMult);
//    }
//    
//    this->setDemand(dmaDemand);
//  }
//  else {
//    ConstantTimeSeries::_sp constDma(new ConstantTimeSeries());
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


void Dma::initDemandTimeseries(const set<Pipe::_sp> &boundarySet) {
  // set up a fixed 1-m clock for constant series
  Clock::_sp fixed_minute_clock(new Clock(60));
  
  // we've supplied a list of candidate boundary pipes. prune the list of pipes that don't connect to this dma.
  for(const Pipe::_sp p : boundarySet) {
    Junction::_sp j1, j2;
    j1 = std::static_pointer_cast<Junction>(p->from());
    j2 = std::static_pointer_cast<Junction>(p->to());
    
    bool j1member = this->doesHaveJunction(j1);
    bool j2member = this->doesHaveJunction(j2);
    
    if (j1member || j2member) {
      if (!j1member || !j2member) {
        // only one side connects to me. it's a boundary
        Junction::_sp myJ = j1member ? j1 : j2;
        Pipe::direction_t jDir = p->directionRelativeToNode(myJ);
        
        // figure out why this pipe is included here. is it flow measured? is it closed?
        if (p->flowMeasure()) {
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
        if (p->flowMeasure()) {
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
  
  
  
  AggregatorTimeSeries::_sp boundaryDemandSum(new AggregatorTimeSeries());
  boundaryDemandSum->setUnits(RTX_GALLON_PER_MINUTE);
  for(auto ts : _boundaryFlowJunctions) {
    boundaryDemandSum->addSource(ts->boundaryFlow());
  }
  if (_boundaryFlowJunctions.size() > 0) {
    _boundaryDemand = boundaryDemandSum;
  }
  else {
    ConstantTimeSeries::_sp c(new ConstantTimeSeries());
    c->setValue(0);
    c->setClock(fixed_minute_clock);
    c->setUnits(RTX_GALLON_PER_MINUTE);
    _boundaryDemand = c;
  }
  
  // separate junctions into:
  // -- demand junctions
  // -- boundary flow junctions
  // -- storage tanks
  
  if (!this->doesContainReservoir()) {
    // assemble the aggregated demand time series
    
    AggregatorTimeSeries::_sp dmaDemand( new AggregatorTimeSeries() );
    dmaDemand->setUnits(RTX_GALLON_PER_MINUTE);
    dmaDemand->setName("DMA " + this->name() + " demand");
    for(Tank::_sp t : _tanks) {
      dmaDemand->addSource(t->flowCalc(), -1.);
    }
    /* boundary flows are accounted for in the allocation method
     for(Junction::_sp j : _boundaryFlowJunctions) {
     dmaDemand->addSource(j->boundaryFlow(), -1.);
     }
     */
    
    if (_measuredBoundaryPipesDirectional.size() == 0) {
      ConstantTimeSeries::_sp zero( new ConstantTimeSeries() );
      zero->setUnits(RTX_GALLON_PER_MINUTE);
      zero->setClock(fixed_minute_clock);
      zero->setValue(0.);
      this->setDemand(zero);
    }
    else {
      for(pipeDirPair_t pd : _measuredBoundaryPipesDirectional) {
        Pipe::_sp p = pd.first;
        Pipe::direction_t dir = pd.second;
        double dirMult = ( dir == Pipe::inDirection ? 1. : -1. );
        dmaDemand->addSource(p->flowMeasure(), dirMult);
      }
      this->setDemand(dmaDemand);
    }

  }
  else {
    ConstantTimeSeries::_sp constDma(new ConstantTimeSeries());
    constDma->setName("Zero Demand");
    constDma->setValue(0.);
    constDma->setUnits(RTX_GALLON_PER_MINUTE);
    constDma->setClock(fixed_minute_clock);
    this->setDemand(constDma);
  }
  
  
  
  
  set<string> flowMeasuredPipes, closedBoundaryPipes, tanks, junctions;
  for (auto p : this->measuredBoundaryPipes()) {
    const char* dirChar = (p.second == Pipe::direction_t::inDirection ? "+" : "-");
    flowMeasuredPipes.insert( string(dirChar) + p.first->name());
  }
  for (auto p : this->closedBoundaryPipes()) {
    closedBoundaryPipes.insert(p.first->name());
  }
  for (auto t : this->tanks()) {
    tanks.insert(t->name());
  }
  for (auto j : this->junctions()) {
    junctions.insert(j->name());
  }
  
  
  
  // hash formulation. entries are ordered using built-in std::set sorting.
  // m:[+/-]<mbpName>,[+/-]<mbpName>,[...],c:<cbpName>,<cbpName>,[...],t:<tankName>,<tankName>,[...],j:<juncName>,<juncName>,[...]
  
  EVP_MD_CTX *ctx = EVP_MD_CTX_new();
  const EVP_MD *hashptr = EVP_get_digestbyname("SHA1");
  EVP_DigestInit(ctx, hashptr);
  auto updateSHA = [&](set<string> &strings) {
    auto it = strings.begin(), itEnd = strings.end();
    if (it != itEnd) {
      EVP_DigestUpdate(ctx, (unsigned char *)it->c_str(), it->length());
      ++it;
    }
    for(; it != itEnd; ++it) {
      EVP_DigestUpdate(ctx, (unsigned char *)(","), 1);
      EVP_DigestUpdate(ctx, (unsigned char *)it->c_str(), it->length());
    }
  };

  EVP_DigestUpdate(ctx, (unsigned char *)("m:"), 2);
  updateSHA(flowMeasuredPipes);
  EVP_DigestUpdate(ctx, (unsigned char *)("c:"), 2);
  updateSHA(closedBoundaryPipes);
  EVP_DigestUpdate(ctx, (unsigned char *)("t:"), 2);
  updateSHA(tanks);
  EVP_DigestUpdate(ctx, (unsigned char *)("j:"), 2);
  updateSHA(junctions);

  unsigned char digest[SHA_DIGEST_LENGTH]; // len == 20
  EVP_DigestFinal(ctx, digest, NULL);
  
  int len = 0;
  char hashedNameCh[SHA_DIGEST_LENGTH*2+1];
  for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
    len += sprintf(hashedNameCh+len, "%02x", digest[i]);
  }
  this->hashedName = string(hashedNameCh);
}


bool Dma::isAlwaysClosed(Pipe::_sp pipe) {
  return ((pipe->fixedStatus() == Pipe::CLOSED) && (pipe->type() != Element::PUMP));
}

bool Dma::isTank(Junction::_sp junction) {
  return (junction->type() == Element::TANK);
}

bool Dma::isBoundaryFlowJunction(Junction::_sp junction) {
  return (junction->boundaryFlow() ? true : false);
}

/* deprecated
void Dma::followJunction(Junction::_sp junction) {
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
    Tank::_sp thisTank = std::static_pointer_cast<Tank>(junction);
    TimeSeries::_sp flow = thisTank->flowMeasure();
    
    // flow is positive into the tank (out of the dma), so its sign for demand aggregation purposes should be negative.
    AggregatorTimeSeries::_sp dmaDemand = std::static_pointer_cast<AggregatorTimeSeries>(this->demand());
    cout << "dma " << this->name() << " : adding tank source : " << flow->name() << endl;
    dmaDemand->addSource(flow, -1.);
  }
  
  
  // for each link connected to the junction, follow it and add its junctions
  for(Link::_sp link : junction->links()) {
    cout << "... examining pipe " << link->name() << endl;
    Pipe::_sp pipe = std::static_pointer_cast<Pipe>(link);
    
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
        followJunction(std::static_pointer_cast<Junction>( pipe->to() ) );
      }
      else {
        followJunction(std::static_pointer_cast<Junction>( pipe->from() ) );
      }
    }
    else {
      // we have found a measurement.
      // add it to the control volume calculation.
      double direction = (directionIsOut? -1. : 1.);
      AggregatorTimeSeries::_sp dmaDemand = std::static_pointer_cast<AggregatorTimeSeries>(this->demand());
      if (!dmaDemand) {
        cerr << "dma time series wrong type: " << *(this->demand()) << endl;
      }
      cout << "dma " << this->name() << " : adding source " << pipe->flowMeasure()->name() << endl;
      dmaDemand->addSource(pipe->flowMeasure(), direction);
    }
  }
  
}
*/

Junction::_sp Dma::findJunction(std::string name) {
  for(Junction::_sp j : _junctions) {
    if (RTX_STRINGS_ARE_EQUAL(j->name(), name)) {
      return j;
    }
  }
  
  Junction::_sp aJunction;
  return aJunction;
}

bool Dma::doesHaveJunction(Junction::_sp j) {
//  return (find(_junctions.begin(), _junctions.end(), j) != _junctions.end());
  return _junctions.find(j) != _junctions.end();
}

std::set<Junction::_sp> Dma::junctions() {

  return _junctions;
}

std::set<Tank::_sp> Dma::tanks() {
  return _tanks;
}

std::vector<Dma::pipeDirPair_t> Dma::measuredBoundaryPipes() {
  return _measuredBoundaryPipesDirectional;
}

std::vector<Junction::_sp> Dma::measuredBoundaryJunctions() {
  return _boundaryFlowJunctions;
}

std::vector<Dma::pipeDirPair_t> Dma::closedBoundaryPipes() {
  return _closedBoundaryPipesDirectional;
}

std::vector<Pipe::_sp> Dma::closedInteriorPipes() {
  return _closedInteriorPipes;
}

std::vector<Pipe::_sp> Dma::measuredInteriorPipes() {
  return _measuredInteriorPipes;
}

bool Dma::isMeasuredBoundaryPipe(Pipe::_sp pipe) {
  
  for(const pipeDirPair_t& pdp : _measuredBoundaryPipesDirectional) {
    if (pdp.first == pipe) {
      return true;
    }
  }
  
  return false;
}

bool Dma::isMeasuredInteriorPipe(Pipe::_sp pipe) {
  
  for(const Pipe::_sp p : _measuredInteriorPipes) {
    if (p == pipe) {
      return true;
    }
  }
  
  return false;
}

bool Dma::isMeasuredPipe(Pipe::_sp pipe) {
  
  if (isMeasuredBoundaryPipe(pipe)) {
    return true;
  }

  if (isMeasuredInteriorPipe(pipe)) {
    return true;
  }
  
  return false;
}

bool Dma::isClosedBoundaryPipe(Pipe::_sp pipe) {
  
  for(const pipeDirPair_t& pdp : _closedBoundaryPipesDirectional) {
    if (pdp.first == pipe) {
      return true;
    }
  }
  
  return false;
}

bool Dma::isClosedInteriorPipe(Pipe::_sp pipe) {
  
  for(const Pipe::_sp p : _closedInteriorPipes) {
    if (p == pipe) {
      return true;
    }
  }
  
  return false;
}

bool Dma::isClosedPipe(Pipe::_sp pipe) {

  if (isClosedBoundaryPipe(pipe)) {
    return true;
  }
  
  if (isClosedInteriorPipe(pipe)) {
    return true;
  }
  
  return false;
}

bool Dma::isBoundaryPipe(Pipe::_sp pipe) {
  
  if (isClosedBoundaryPipe(pipe)) {
    return true;
  }
  
  if (isMeasuredBoundaryPipe(pipe)) {
    return true;
  }

  return false;
}

void Dma::setDemand(TimeSeries::_sp demand) {
  if (demand->units().isSameDimensionAs(RTX_CUBIC_METER_PER_SECOND)) {
    _demand = demand;
  }
  else {
    cerr << "could not set demand -- dimension must be volumetric rate" << endl;
  }
}

TimeSeries::_sp Dma::demand() {
  return _demand;
}

TimeSeries::_sp Dma::boundaryDemand() {
  return _boundaryDemand;
}

int Dma::allocateDemandToJunctions(time_t time) {
  // get each node's base demand for the current time
  // add the base demands together. this is the total base demand.
  // get the input demand value for the current time - from the demand() method
  // compute the global scaling factor
  // apply this to each base demand
  // add each scaled base demand to the appropriate node's demand pattern.
  int err = 0;
  double totalBaseDemand = 0;
  double dmaDemand = 0;
  double allocableDemand = 0;
  double meteredDemand = 0;
  
  Units myUnits = demand()->units();
  Units modelUnits = _flowUnits;
  
  
  // if the junction has a boundary flow condition, add it to the "known" demand pool.
  // otherwise, add the nominal base demand to the totalBaseDemand pool.
  for(Junction::_sp junction : _junctions) {
    
    if ( junction->boundaryFlow() ) {
      Point dp = junction->boundaryFlow()->pointAtOrBefore(time);
      if (dp.isValid) {
        double demand = Units::convertValue(dp.value, junction->boundaryFlow()->units(), myUnits);
        meteredDemand += demand;
      }
      else {
        err = 1;
        cerr << "ERR: invalid junction boundary flow point -- " << this->name() << endl;
      }
    }
    else {
      double demand = Units::convertValue(junction->baseDemand(), modelUnits, myUnits);
      totalBaseDemand += demand;
    }
    
  }
  
  // now we have the total (nominal) base demand for the dma.
  // total demand for the dma (includes metered and unmetered) -- already in myUnits.
  Point dPoint = this->demand()->pointAtOrBefore(time);
  if (dPoint.isValid) {
    dmaDemand = dPoint.value;
    allocableDemand = dmaDemand - meteredDemand; // the total unmetered demand
  }
  else {
    err = 1;
    cerr << "ERR: invalid total demand point -- " << this->name() << endl;
  }
  
//  cout << "+++-------------------+++" << endl;
//  cout << "dma: " << this->name() << endl;
//  cout << "time: " << time << endl;
//  cout << "measured dma demand: " << dmaDemand << endl;
//  cout << "metered: " << meteredDemand << endl;
//  cout << "allocable: " << allocableDemand << endl;
//  cout << "dma base demand: " << totalBaseDemand << endl;
  // insert junction demand points at current simulation time
  for(Junction::_sp junction : _junctions) {
    if (junction->boundaryFlow()) {
      // junction does have boundary flow...
      // just need to copy the boundary flow into the junction's demand time series
      Point dp = junction->boundaryFlow()->pointAtOrBefore(time);
      if (dp.isValid) {
        Point newDemandPoint = Point::convertPoint(dp, junction->boundaryFlow()->units(), junction->demand()->units());
        newDemandPoint.time = time;
        junction->state_demand = newDemandPoint.value;
      }
      else {
        err = 1;
        cerr << "ERR: invalid junction boundary flow point -- " << this->name() << endl;
      }
    }
    else if (totalBaseDemand > 0) {
      // junction relies on us to set its demand value... but only set it if the total base demand > 0
      double baseDemand = Units::convertValue(junction->baseDemand(), modelUnits, myUnits);
      double newDemand = baseDemand * ( allocableDemand / totalBaseDemand );
      newDemand = Units::convertValue(newDemand, myUnits, junction->demand()->units());
      junction->state_demand = newDemand;
    }
    else {
      // for instance if totalBaseDemand == 0 then this demand = 0
      junction->state_demand = 0;
    }
  }
  
  return err;
  
}


