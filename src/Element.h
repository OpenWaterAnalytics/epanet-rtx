//
//  element.h
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#ifndef epanet_rtx_element_h
#define epanet_rtx_element_h

#include <string.h>
#include <time.h>
#include "rtxMacros.h"
#include "TimeSeries.h"
#include "PointRecord.h"


namespace RTX {

#pragma mark -
#pragma mark Abstract Element
  //!   Element Base Class
  /*!
        Properties common to all elements.
  */
  class Element {
  public:
    typedef enum { JUNCTION, TANK, RESERVOIR, PIPE, PUMP, VALVE, DMA } element_t;
      // please don't misuse this type enumeration!!!
    RTX_SHARED_POINTER(Element);
    virtual std::ostream& toStream(std::ostream &stream);
    element_t type();
    void setName(const std::string& newName);
    std::string name();
    virtual void setRecord(PointRecord::sharedPointer record);

  protected:
    Element(const std::string& name);
    virtual ~Element();
    void setType(element_t type);
  
  private:
    std::string _name;
    element_t _type;
  };
  
  std::ostream& operator<< (std::ostream &out, Element &e);
}

#endif
