//
//  link_family.h
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#ifndef epanet_rtx_link_h
#define epanet_rtx_link_h

#include "Element.h"
#include "Node.h"

namespace RTX {
  
#pragma mark Link Classes
  
  //!   Link Class
  /*!
   A Link is a purely topological construct. It has start and end Node references.
   */
  class Link : public Element {
  public:
    TSF_BASE_PROPS(Link);
    typedef enum {outDirection, inDirection} direction_t;
    std::pair<Node::_sp, Node::_sp> nodes();
    Node::_sp from();
    Node::_sp to();
    Link::direction_t directionRelativeToNode(Node::_sp node);
    void setNodes(Node::_sp startNode, Node::_sp endNode);
    
  protected:
    Link(const std::string& name);
    virtual ~Link();
    
  private:
    Node::_sp _from, _to;
    std::vector< std::pair<double, double> > _coordinates; // list of shape coordinates (ordered from-to)  
  };
  
  
}

#endif
