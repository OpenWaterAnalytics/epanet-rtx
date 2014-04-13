//
//  Link.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#include "Link.h"

using namespace RTX;
using namespace std;

Link::Link(const std::string& name, Node::sharedPointer startNode, Node::sharedPointer endNode) : Element(name) {
  _from = startNode;
  _to = endNode;
}

Link::~Link() {
  
}

std::pair<Node::sharedPointer, Node::sharedPointer> Link::nodes() {
  return std::make_pair<Node::sharedPointer, Node::sharedPointer>(from(), to());
}

Node::sharedPointer Link::from() {
  return _from;
}

Node::sharedPointer Link::to() {
  return _to;
}



Link::direction_t Link::directionRelativeToNode(Node::sharedPointer node) {
  direction_t dir;
  if (from() == node) {
    dir = outDirection;
  }
  else if (to() == node) {
    dir = inDirection;
  }
  else {
    // should not happen
    std::cerr << "direction could not be found for pipe: " << name() << std::endl;
  }
  return dir;
}

