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
    RTX_SHARED_POINTER(Link);
    std::pair<Node::sharedPointer, Node::sharedPointer> nodes();
    Node::sharedPointer from();
    Node::sharedPointer to();
    
  protected:
    Link(const std::string& name, Node::sharedPointer startNode, Node::sharedPointer endNode);
    virtual ~Link();
    
  private:
    Node::sharedPointer _from, _to;
    std::vector< std::pair<double, double> > _coordinates; // list of shape coordinates (ordered from-to)  
  };
  
  
}

#endif
