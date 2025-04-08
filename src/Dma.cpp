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
#include <ConstantTimeSeries.h>
#include <AggregatorTimeSeries.h>

#include <boost/config.hpp>
#include <boost/algorithm/string/join.hpp>
#include <algorithm>
#include <utility>
#include <boost/graph/adjacency_list.hpp>

#include <openssl/sha.h>

using namespace RTX;
using namespace TSF;
using namespace std;
using std::cout;

Dma::Dma(const std::string& name) : Element(name), _flowUnits(1) {
  this->setType(DMA);
  _flowUnits = TSF_LITER_PER_SECOND;
  // set to aggregator type because that's the most likely scenario.
  // presumably, we will use Dma::enumerateJunctionsWithRootNode to populate the aggregation.
  _demand.reset(new AggregatorTimeSeries() );
  _demand->setName("DMA " + name + " demand");
  _demand->setUnits(TSF_LITER_PER_SECOND);
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

void Dma::setJunctionFlowUnits(TSF::Units units) {
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


void Dma::initDemandTimeseries(const set<Pipe::_sp> &boundarySet) {
  // set up a fixed 1-m clock for constant series
  Clock::_sp fixed_minute_clock(new Clock(60));
  
  // we've supplied a list of candidate boundary pipes. prune the list of pipes that don't connect to this dma.
  for(const Pipe::_sp &p : boundarySet) {
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
        if (p->dmaFlowMeasure()) {
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
        if (p->dmaFlowMeasure()) {
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
  boundaryDemandSum->setUnits(TSF_GALLON_PER_MINUTE);
  for(auto j : _boundaryFlowJunctions) {
    boundaryDemandSum->addSource(j->boundaryFlow());
  }
  if (_boundaryFlowJunctions.size() > 0) {
    _boundaryDemand = boundaryDemandSum;
  }
  else {
    ConstantTimeSeries::_sp c(new ConstantTimeSeries());
    c->setValue(0);
    c->setClock(fixed_minute_clock);
    c->setUnits(TSF_GALLON_PER_MINUTE);
    _boundaryDemand = c;
  }
  
  // separate junctions into:
  // -- demand junctions
  // -- boundary flow junctions
  // -- storage tanks
  
  if (!this->doesContainReservoir()) {
    // assemble the aggregated demand time series
    
    AggregatorTimeSeries::_sp dmaDemand( new AggregatorTimeSeries() );
    dmaDemand->setUnits(TSF_GALLON_PER_MINUTE);
    dmaDemand->setName("DMA " + this->name() + " demand");
    for(Tank::_sp t : _tanks) {
      dmaDemand->addSource(t->dmaFlowCalc(), -1.);
    }
    /* boundary flows are accounted for in the allocation method
     for(Junction::_sp j : _boundaryFlowJunctions) {
     dmaDemand->addSource(j->boundaryFlow(), -1.);
     }
     */
    
    if (_measuredBoundaryPipesDirectional.size() == 0) {
      ConstantTimeSeries::_sp zero( new ConstantTimeSeries() );
      zero->setUnits(TSF_GALLON_PER_MINUTE);
      zero->setClock(fixed_minute_clock);
      zero->setValue(0.);
      this->setDemand(zero);
    }
    else {
      for(pipeDirPair_t pd : _measuredBoundaryPipesDirectional) {
        Pipe::_sp p = pd.first;
        Pipe::direction_t dir = pd.second;
        double dirMult = ( dir == Pipe::inDirection ? 1. : -1. );
        dmaDemand->addSource(p->dmaFlowMeasure(), dirMult);
      }
      this->setDemand(dmaDemand);
    }

  }
  else {
    ConstantTimeSeries::_sp constDma(new ConstantTimeSeries());
    constDma->setName("Zero Demand");
    constDma->setValue(0.);
    constDma->setUnits(TSF_GALLON_PER_MINUTE);
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
  
  SHA_CTX ctx;
  SHA1_Init(&ctx);
  
  auto updateSha = [&](set<string> &strings){
    bool firstEntry = true;
    for (auto str : strings) {
      if (!firstEntry) {
        SHA1_Update(&ctx, (unsigned char *)(","), 1);
      }
      SHA1_Update(&ctx, (unsigned char *)str.c_str(), str.length());
      firstEntry = false;
    }
  };
  
  SHA1_Update(&ctx, (unsigned char *)("m:"), 2);
  updateSha(flowMeasuredPipes);
  SHA1_Update(&ctx, (unsigned char *)("c:"), 2);
  updateSha(closedBoundaryPipes);
  SHA1_Update(&ctx, (unsigned char *)("t:"), 2);
  updateSha(tanks);
  SHA1_Update(&ctx, (unsigned char *)("j:"), 2);
  updateSha(junctions);
  
  unsigned char digest[SHA_DIGEST_LENGTH]; // len == 20
  SHA1_Final(digest, &ctx); 
  
  int len = 0;
  char hashedNameCh[SHA_DIGEST_LENGTH*2+1];
  for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
    len += sprintf(hashedNameCh+len, "%02x",digest[i]);
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
  
  for(const Pipe::_sp &p : _measuredInteriorPipes) {
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
  
  for(const Pipe::_sp &p : _closedInteriorPipes) {
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
  if (demand->units().isSameDimensionAs(TSF_CUBIC_METER_PER_SECOND)) {
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

std::shared_ptr<Dma::DemandAllocationDelegate> Dma::allocationDelegate() {
  return _allocationDelegate;
}

void Dma::setAllocationDelegate(std::shared_ptr<Dma::DemandAllocationDelegate> delegate) {
  _allocationDelegate = delegate;
}

int Dma::allocateDemandToJunctions(time_t time) {
  if (_allocationDelegate && _allocationDelegate->allocateDemands(share_me(this), time)) {
      return 0;  // delegate succeeded. no error and early out.
  }

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


