//
//  node_family.h
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#ifndef epanet_rtx_node_family_h
#define epanet_rtx_node_family_h

// #include <boost/weak_ptr.hpp>

#include "Element.h"

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
    struct location_t {
      double longitude;
      double latitude;
      location_t(double lon, double lat) {longitude = lon; latitude = lat;};
    };
    friend class Model;
    TSF_BASE_PROPS(Node);
    // properties - get/set
    location_t coordinates();
    virtual void setCoordinates(location_t location);
    double elevation();
    virtual void setElevation(double elevation);
    std::vector< std::shared_ptr<Link> > links();
    
  protected:
    Node(const std::string& name);
    virtual ~Node();
    void addLink(std::shared_ptr<Link> link);
    void removeLink(std::shared_ptr<Link> link);
    
  private:
    location_t _lonLat;
    double _z;
    std::set<Link*> _links;
    
  };
  
}

#endif
