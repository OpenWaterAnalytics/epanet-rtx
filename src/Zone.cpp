//
//  Zone.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See Readme.txt and license.txt for more information
//  https://sourceforge.net/projects/epanet-rtx/

#include "Zone.h"
#include <boost/foreach.hpp>

using namespace RTX;

Zone::Zone(const std::string& name) : Element(name), _flowUnits(1) {
  this->setType(ZONE);
  _flowUnits = RTX_LITER_PER_SECOND;
  
  _demand.reset(new TimeSeries() );
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
  _junctions[junction->name()] = junction;
}

void Zone::addJunctionTree(Junction::sharedPointer junction) {
  // don't let us add the same junction twice.
  if (!junction || find(junction->name())) {
    return;
  }
  // std::cout << "adding junction " << junction->name() << std::endl;
  // perform dfs
  // add the junction to my list
  addJunction(junction);
  // for each link connected to the junction, follow it and add its junctions
  BOOST_FOREACH(Link::sharedPointer link, junction->links()) {
    Pipe::sharedPointer pipe = boost::dynamic_pointer_cast<Pipe>(link);
    // make sure we can follow this link,
    // then get this link's nodes
    if ( !(pipe->doesHaveFlowMeasure()) ) {
      // follow each of the link's nodes.
      addJunctionTree(boost::static_pointer_cast<Junction>( pipe->from() ) );
      addJunctionTree(boost::static_pointer_cast<Junction>( pipe->to() ) );
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
  // TODO - account for tanks (maybe leave this to the client?)
  BOOST_FOREACH(JunctionMapType::value_type& junctionPair, _junctions) {
    Junction::sharedPointer junction = junctionPair.second;
    
    if ( junction->doesHaveBoundaryFlow() ) {
      double demand = Units::convertValue(junction->boundaryFlow()->value(time), junction->boundaryFlow()->units(), myUnits);
      meteredDemand += demand;
    }
    else {
      double demand = Units::convertValue(junction->baseDemand(), modelUnits, myUnits);
      totalBaseDemand += demand;
    }
    
  }
  
  // now we have the total (nominal) base demand for the zone.
  // total demand for the zone (includes metered and unmetered) -- already in myUnits.
  zoneDemand = this->demand()->value(time);
  allocableDemand = zoneDemand - meteredDemand; // the total unmetered demand
  
  // set the demand values for unmetered junctions, according to their base demands.
  BOOST_FOREACH(JunctionMapType::value_type& junctionPair, _junctions) {
    Junction::sharedPointer junction = junctionPair.second;
    if (junction->doesHaveBoundaryFlow()) {
      // junction does have boundary flow...
      // just need to copy the boundary flow into the junction's demand time series
      Point::sharedPointer demandPoint = junction->boundaryFlow()->point(time);
      junction->demand()->insert( Point::convertPoint(*demandPoint, junction->boundaryFlow()->units(), junction->demand()->units()) );
    }
    else {
      // junction relies on us to set its demand value...
      double baseDemand = Units::convertValue(junction->baseDemand(), modelUnits, myUnits);
      double newDemand = baseDemand * ( allocableDemand / totalBaseDemand );
      Point::sharedPointer demandPoint( new Point(time, newDemand) );
      junction->demand()->insert( Point::convertPoint(*demandPoint, myUnits, junction->demand()->units()) );
    }
  }
  
}

