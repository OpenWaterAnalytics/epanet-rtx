//
//  Node.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See Readme.txt and license.txt for more information
//  https://sourceforge.net/projects/epanet-rtx/

#include "node.h"
#include "AggregatorTimeSeries.h"
#include "ConstantSeries.h"

using namespace RTX;

Node::Node(const std::string& name) : Element(name) {
  _x = 0;
  _y = 0;
  _z = 0;
}
Node::~Node() {
  
}

void Node::addLink(boost::shared_ptr<Link> link) {
  _links.push_back(link);
}

std::vector< boost::shared_ptr<Link> > Node::links() {
  return _links;
}

std::pair<double,double> Node::coordinates() {
  return std::pair<double,double>(_x,_y);
}
void Node::setCoordinates(double x, double y) {
  _x = x;
  _y = y;
}
double Node::elevation() {
  return _z;
}
void Node::setElevation(double elevation) {
  _z = elevation;
}
