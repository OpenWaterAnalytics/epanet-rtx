//
//  Link.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See Readme.txt and license.txt for more information
//  http://tinyurl.com/epanet-rtx

#include "Link.h"

using namespace RTX;

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




