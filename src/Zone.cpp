//
//  Zone.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#include "Zone.h"
#include <boost/foreach.hpp>

using namespace RTX;
using namespace std;

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

void Zone::setRecord(PointRecord::sharedPointer record) {
  if (_demand) {
    _demand->newCacheWithPointRecord(record);
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
    _junctions[junction->name()] = junction;
  }
}

void Zone::enumerateJunctionsWithRootNode(Junction::sharedPointer junction) {
  
  cout << "==========" << endl;
  cout << "Zone " << name() << " : enumerating junctions" << endl;
  this->followJunction(junction);
  
}

void Zone::followJunction(Junction::sharedPointer junction) {
  // don't let us add the same junction twice.
  if (!junction || find(junction->name())) {
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

Junction::sharedPointer Zone::find(std::string name) {
  Junction::sharedPointer aJunction;
  if (_junctions.find(name) != _junctions.end() ) {
    aJunction = _junctions[name];
  }
  return aJunction;
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
  _demand = demand;
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
      double demand = Units::convertValue(junction->boundaryFlow()->point(time).value(), junction->boundaryFlow()->units(), myUnits);
      meteredDemand += demand;
    }
    else {
      double demand = Units::convertValue(junction->baseDemand(), modelUnits, myUnits);
      totalBaseDemand += demand;
    }
    
  }
  
  // now we have the total (nominal) base demand for the zone.
  // total demand for the zone (includes metered and unmetered) -- already in myUnits.
  zoneDemand = this->demand()->point(time).value();
  allocableDemand = zoneDemand - meteredDemand; // the total unmetered demand
  
  cout << "-------------------" << endl;
  cout << "time: " << time << endl;
  cout << "zone demand: " << zoneDemand << endl;
  cout << "metered demand: " << meteredDemand << endl;
  cout << "allocable demand: " << allocableDemand << endl;
  
  // set the demand values for unmetered junctions, according to their base demands.
  BOOST_FOREACH(JunctionMapType::value_type& junctionPair, _junctions) {
    Junction::sharedPointer junction = junctionPair.second;
    if (junction->doesHaveBoundaryFlow()) {
      // junction does have boundary flow...
      // just need to copy the boundary flow into the junction's demand time series
      Point demandPoint = junction->boundaryFlow()->point(time);
      junction->demand()->insert( Point::convertPoint(demandPoint, junction->boundaryFlow()->units(), junction->demand()->units()) );
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

