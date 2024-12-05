//
//  Node.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//


#include "Node.h"
#include "Link.h"

using namespace std;
using namespace RTX;
using namespace TSF;

Node::Node(const std::string& name) : Element(name), _lonLat(0,0) {
  _z = 0;
}
Node::~Node() {
  
}

void Node::addLink(std::shared_ptr<Link> link) {
  Link *rawLink = link.get();
  
  if (_links.count(rawLink) == 0) {
    _links.insert(rawLink);
  }
}

void Node::removeLink(std::shared_ptr<Link> link) {
  Link *rawLink = link.get();
  _links.erase(rawLink);
}

std::vector< std::shared_ptr<Link> > Node::links() {
  std::vector<std::shared_ptr<Link> > linkvector;
  // get shared pointers for these weakly pointed-to links
  for(Link *l : _links) {
    Link::_sp sharedLink = dynamic_pointer_cast<Link>(l->sp());
    linkvector.push_back(sharedLink);
  }
  return linkvector;
}

Node::location_t Node::coordinates() {
  return _lonLat;
}
void Node::setCoordinates(location_t coords) {
  _lonLat = coords;
}
double Node::elevation() {
  return _z;
}
void Node::setElevation(double elevation) {
  _z = elevation;
}
