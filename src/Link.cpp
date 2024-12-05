//
//  Link.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#include "Link.h"

using namespace RTX;
using namespace TSF;
using namespace std;

Link::Link(const std::string& name) : Element(name) {
  
}

void Link::setNodes(Node::_sp startNode, Node::_sp endNode) {
  _from = startNode;
  _to = endNode;
}

Link::~Link() {
  
}

std::pair<Node::_sp, Node::_sp> Link::nodes() {
  return std::make_pair<Node::_sp, Node::_sp>(from(), to());
}

Node::_sp Link::from() {
  return _from;
}

Node::_sp Link::to() {
  return _to;
}



Link::direction_t Link::directionRelativeToNode(Node::_sp node) {
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
    dir = inDirection;
  }
  return dir;
}

