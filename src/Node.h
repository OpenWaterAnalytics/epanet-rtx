//
//  node_family.h
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#ifndef epanet_rtx_node_family_h
#define epanet_rtx_node_family_h

#include "Element.h"
#include "AggregatorTimeSeries.h"

namespace RTX {
  
  // forward declaration
  class Link;
  class Model;
  
#pragma mark Node Classes
  
  //!   Node Class
  /*!
   A Node is a purely topological construct. It has x,y,z coordinates and references to Links.
   */
  class Node : public Element {
  public:
    friend class Model;
    RTX_SHARED_POINTER(Node);
    // properties - get/set
    std::pair<double,double> coordinates();
    virtual void setCoordinates(double x, double y);
    double elevation();
    virtual void setElevation(double elevation);
    std::vector< boost::shared_ptr<Link> > links();
    
  protected:
    Node(const std::string& name);
    virtual ~Node();
    void addLink(boost::shared_ptr<Link> link);
    
  private:
    double _x, _y, _z;
    std::vector< boost::shared_ptr<Link> > _links;
    
  };
  
}

#endif
