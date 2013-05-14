//
//  Zone.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#include <boost/foreach.hpp>
#include <boost/range/adaptors.hpp>

#include "Zone.h"
#include "ConstantTimeSeries.h"
#include "AggregatorTimeSeries.h"

using namespace RTX;
using namespace std;
using std::cout;

Zone::Zone(const std::string& name) : Element(name), _flowUnits(1) {
  this->setType(ZONE);
  _flowUnits = RTX_LITER_PER_SECOND;
  // set to aggregator type because that's the most likely scenario.
  // presumably, we will use Zone::enumerateJunctionsWithRootNode to populate the aggregation.
  _demand.reset(new AggregatorTimeSeries() );
  _demand->setName("Z " + name + " demand");
  _demand->setUnits(RTX_LITER_PER_SECOND);
}
Zone::~Zone() {
  
}

std::ostream& Zone::toStream(std::ostream &stream) {
  stream << "Zone: \"" << this->name() << "\"\n";
  stream << " - " << junctions().size() << " Junctions" << endl;
  stream << " - " << _boundaryPipesDirectional.size() << " Boundary Pipes:" << endl;
  typedef pair<Pipe::sharedPointer, direction_t> pipeDir_t;
  BOOST_FOREACH(pipeDir_t pd, _boundaryPipesDirectional) {
    Pipe::sharedPointer pipe = pd.first;
    string dir = (pd.second == inDirection)? "(+)" : "(-)";
    stream << "   -- " << dir << " " << pipe->name() << endl;
  }
  stream << "Time Series Aggregation:" << endl;
  stream << _demand << endl;
  
  
  return stream;
}

void Zone::setRecord(PointRecord::sharedPointer record) {
  if (_demand) {
    _demand->setRecord(record);
  }
}

void Zone::setJunctionFlowUnits(RTX::Units units) {
  _flowUnits = units;
}

void Zone::addJunction(Junction::sharedPointer junction) {
  if (_junctions.find(junction->name()) != _junctions.end()) {
    cerr << "err: junction already exists" << endl;
  }
  else {
    _junctions.insert(make_pair(junction->name(), junction));
  }
}

void Zone::removeJunction(Junction::sharedPointer junction) {
  map<string, Junction::sharedPointer>::iterator jIt = _junctions.find(junction->name());
  if (jIt != _junctions.end()) {
    _junctions.erase(jIt);
    cout << "removed junction: " << junction->name() << endl;
  }
}

void Zone::enumerateJunctionsWithRootNode(Junction::sharedPointer junction, bool stopAtClosedLinks) {
  
  bool doesContainReservoir = false;
  
  cout << "Starting At Root Junction: " << junction->name() << endl;
  
  // breadth-first search.
  deque<Junction::sharedPointer> candidateJunctions;
  candidateJunctions.push_back(junction);
  
  while (!candidateJunctions.empty()) {
    Junction::sharedPointer thisJ = candidateJunctions.front();
    cout << " - adding: " << thisJ->name() << endl;
    this->addJunction(thisJ);
    vector<Link::sharedPointer> connectedLinks = thisJ->links();
    BOOST_FOREACH(Link::sharedPointer l, connectedLinks) {
      // follow this link?
      Pipe::sharedPointer p = boost::static_pointer_cast<Pipe>(l);
      if (p->doesHaveFlowMeasure()) {
        // stop here - it's a potential zone perimeter pipe.
        // but first, capture the pipe and direction
        
        cout << " - perimeter pipe: " << p->name() << endl;
        
        direction_t dir;
        if (p->from() == thisJ) {
          dir = outDirection;
        }
        else if (p->to() == thisJ) {
          dir = inDirection;
        }
        else {
          // should not happen?
          cerr << "direction could not be found for pipe: " << p->name() << endl;
        }
        _boundaryPipesDirectional.insert(make_pair(p, dir));
        continue;
      }
      else if (stopAtClosedLinks && p->fixedStatus() == Pipe::CLOSED) {
        cout << "detected fixed status (closed) pipe: " << p->name() << endl;
        continue;
      }
      
      pair<Node::sharedPointer, Node::sharedPointer> nodes = l->nodes();
      vector<Junction::sharedPointer> juncs;
      juncs.push_back(boost::static_pointer_cast<Junction>(nodes.first));
      juncs.push_back(boost::static_pointer_cast<Junction>(nodes.second));
      BOOST_FOREACH(Junction::sharedPointer candidateJ, juncs) {
        if (candidateJ != thisJ && !this->doesHaveJunction(candidateJ)) {
          // add to follow list
          candidateJunctions.push_back(candidateJ);
        }
      }
    } // foreach connected link
    candidateJunctions.pop_front();
  }
  
  // cleanup orphaned pipes (pipes which have been identified as perimeters, but have both start/end nodes listed inside the zone)
  
  BOOST_FOREACH(Pipe::sharedPointer p, _boundaryPipesDirectional | boost::adaptors::map_keys) {
    if (this->doesHaveJunction(boost::static_pointer_cast<Junction>(p->from())) && this->doesHaveJunction(boost::static_pointer_cast<Junction>(p->to()))) {
      cout << "removing orphaned pipe: " << p->name() << endl;
      _boundaryPipesDirectional.erase(p);
    }
  }
  
  // separate junctions into:
  // -- demand junctions
  // -- boundary flow junctions
  // -- storage tanks
  
  
  BOOST_FOREACH(Junction::sharedPointer j, _junctions | boost::adaptors::map_values) {
    // is this a reservoir? if so, that's bad news -- we can't compute a control volume. the volume is infinite.
    if (j->type() == Element::RESERVOIR) {
      doesContainReservoir = true;
    }
    // check if it's a tank or metered junction
    if (isTank(j)) {
      //this->removeJunction(j);
      _tanks.push_back(boost::static_pointer_cast<Tank>(j));
      cout << "found tank: " << j->name() << endl;
    }
    else if (isBoundaryFlowJunction(j)) {
      //this->removeJunction(j);
      _boundaryFlowJunctions.push_back(j);
      cout << "found boundary flow: " << j->name() << endl;
    }
    
  }
  
  
  if (!doesContainReservoir) {
    // assemble the aggregated demand time series
    
    AggregatorTimeSeries::sharedPointer zoneDemand( new AggregatorTimeSeries() );
    zoneDemand->setUnits(RTX_GALLON_PER_MINUTE);
    zoneDemand->setName("Zone " + this->name() + " demand");
    BOOST_FOREACH(Tank::sharedPointer t, _tanks) {
      zoneDemand->addSource(t->flowMeasure(), -1.);
    }
    /* boundary flows are accounted for in the allocation method
     BOOST_FOREACH(Junction::sharedPointer j, _boundaryFlowJunctions) {
     zoneDemand->addSource(j->boundaryFlow(), -1.);
     }
     */
    
    typedef pair<Pipe::sharedPointer, direction_t> pipeDirPair_t;
    BOOST_FOREACH(pipeDirPair_t pd, _boundaryPipesDirectional) {
      Pipe::sharedPointer p = pd.first;
      direction_t dir = pd.second;
      double dirMult = ( dir == inDirection ? 1. : -1. );
      zoneDemand->addSource(p->flowMeasure(), dirMult);
    }
    
    this->setDemand(zoneDemand);
  }
  else {
    ConstantTimeSeries::sharedPointer constZone(new ConstantTimeSeries());
    constZone->setUnits(RTX_GALLON_PER_MINUTE);
    this->setDemand(constZone);
  }
  
  
  
  cout << this->name() << " Zone Description:" << endl;
  cout << *this->demand() << endl;
  
}

bool Zone::isTank(Junction::sharedPointer junction) {
  if (junction->type() == Element::TANK) {
    return true;
  }
  else {
    return false;
  }
}

bool Zone::isBoundaryFlowJunction(Junction::sharedPointer junction) {
  return (junction->doesHaveBoundaryFlow());
}

/* deprecated
void Zone::followJunction(Junction::sharedPointer junction) {
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
    
    // flow is positive into the tank (out of the zone), so its sign for demand aggregation purposes should be negative.
    AggregatorTimeSeries::sharedPointer zoneDemand = boost::static_pointer_cast<AggregatorTimeSeries>(this->demand());
    cout << "zone " << this->name() << " : adding tank source : " << flow->name() << endl;
    zoneDemand->addSource(flow, -1.);
  }
  
  
  // for each link connected to the junction, follow it and add its junctions
  BOOST_FOREACH(Link::sharedPointer link, junction->links()) {
    cout << "... examining pipe " << link->name() << endl;
    Pipe::sharedPointer pipe = boost::static_pointer_cast<Pipe>(link);
    
    // get the link direction. into the zone is positive.
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
      AggregatorTimeSeries::sharedPointer zoneDemand = boost::static_pointer_cast<AggregatorTimeSeries>(this->demand());
      if (!zoneDemand) {
        cerr << "zone time series wrong type: " << *(this->demand()) << endl;
      }
      cout << "zone " << this->name() << " : adding source " << pipe->flowMeasure()->name() << endl;
      zoneDemand->addSource(pipe->flowMeasure(), direction);
    }
  }
  
}
*/

Junction::sharedPointer Zone::findJunction(std::string name) {
  Junction::sharedPointer aJunction;
  if (_junctions.find(name) != _junctions.end() ) {
    aJunction = _junctions[name];
  }
  return aJunction;
}

bool Zone::doesHaveJunction(Junction::sharedPointer j) {
  if (_junctions.find(j->name()) != _junctions.end()) {
    return true;
  }
  return false;
}

std::vector<Junction::sharedPointer> Zone::junctions() {
  typedef std::map<std::string, Junction::sharedPointer> JunctionMap;
  std::vector<Junction::sharedPointer> theJunctions;
  BOOST_FOREACH(const JunctionMap::value_type& junctionPair, _junctions) {
    theJunctions.push_back(junctionPair.second);
  }
  return theJunctions;
}

void Zone::setDemand(TimeSeries::sharedPointer demand) {
  if (demand->units().isSameDimensionAs(RTX_CUBIC_METER_PER_SECOND)) {
    _demand = demand;
  }
  else {
    cerr << "could not set demand -- dimension must be volumetric rate" << endl;
  }
}

TimeSeries::sharedPointer Zone::demand() {
  return _demand;
}

void Zone::allocateDemandToJunctions(time_t time) {
  // get each node's base demand for the current time
  // add the base demands together. this is the total base demand.
  // get the input demand value for the current time - from the demand() method
  // compute the global scaling factor
  // apply this to each base demand
  // add each scaled base demand to the appropriate node's demand pattern.
  
  typedef std::map< std::string, Junction::sharedPointer > JunctionMapType;
  double totalBaseDemand = 0;
  double zoneDemand = 0;
  double allocableDemand = 0;
  double meteredDemand = 0;
  
  Units myUnits = demand()->units();
  Units modelUnits = _flowUnits;
  
  
  // if the junction has a boundary flow condition, add it to the "known" demand pool.
  // otherwise, add the nominal base demand to the totalBaseDemand pool.
  BOOST_FOREACH(JunctionMapType::value_type& junctionPair, _junctions) {
    Junction::sharedPointer junction = junctionPair.second;
    
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
  
  // now we have the total (nominal) base demand for the zone.
  // total demand for the zone (includes metered and unmetered) -- already in myUnits.
  Point dPoint = this->demand()->point(time);
  
  zoneDemand = ( dPoint.isValid ? dPoint.value : this->demand()->pointBefore(time).value );
  allocableDemand = zoneDemand - meteredDemand; // the total unmetered demand
  
  cout << "-------------------" << endl;
  cout << "time: " << time << endl;
  cout << "zone demand: " << zoneDemand << endl;
  cout << "metered: " << meteredDemand << endl;
  cout << "allocable: " << allocableDemand << endl;
  
  // set the demand values for unmetered junctions, according to their base demands.
  BOOST_FOREACH(JunctionMapType::value_type& junctionPair, _junctions) {
    Junction::sharedPointer junction = junctionPair.second;
    if (junction->doesHaveBoundaryFlow()) {
      // junction does have boundary flow...
      // just need to copy the boundary flow into the junction's demand time series
      Point dp = junction->boundaryFlow()->point(time);
      double dval = dp.isValid ? dp.value : junction->boundaryFlow()->pointBefore(time).value;
      junction->demand()->insert( Point::convertPoint(Point(dval, time), junction->boundaryFlow()->units(), junction->demand()->units()) );
    }
    else {
      // junction relies on us to set its demand value...
      double baseDemand = Units::convertValue(junction->baseDemand(), modelUnits, myUnits);
      double newDemand = baseDemand * ( allocableDemand / totalBaseDemand );
      Point demandPoint(time, newDemand);
      junction->demand()->insert( Point::convertPoint(demandPoint, myUnits, junction->demand()->units()) );
    }
  }
  
}

