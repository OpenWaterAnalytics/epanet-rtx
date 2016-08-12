//
//  Node.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//

#include <boost/foreach.hpp>

#include "Node.h"
#include "AggregatorTimeSeries.h"

using namespace RTX;

Node::Node(const std::string& name) : Element(name) {
  _lonLat = {0,0};
  _z = 0;
}
Node::~Node() {
  
}

void Node::addLink(std::shared_ptr<Link> link) {
  std::weak_ptr<Link> weaklink(link);
  _links.push_back(weaklink);
}

std::vector< std::shared_ptr<Link> > Node::links() {
  std::vector<std::shared_ptr<Link> > linkvector;
  // get shared pointers for these weakly pointed-to links
  BOOST_FOREACH(std::weak_ptr<Link> l, _links) {
    std::shared_ptr<Link> sharedLink(l);
    linkvector.push_back(sharedLink);
  }
  return linkvector;
}

std::pair<double,double> Node::coordinates() {
  return _lonLat;
}
void Node::setCoordinates(double x, double y) {
  _lonLat = {x,y};
}
double Node::elevation() {
  return _z;
}
void Node::setElevation(double elevation) {
  _z = elevation;
}
