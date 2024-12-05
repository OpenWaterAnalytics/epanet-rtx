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
#include <map>
#include <time.h>
#include <variant>
#include <vector>
#include <TimeSeries.h>
#include <PointRecord.h>

#include "RtxObject.h"

using TSF::TimeSeries;
using TSF::Units;
using TSF::Clock;
using TSF::PointRecord;

namespace RTX {

#pragma mark -
#pragma mark Abstract Element
  //!   Element Base Class
  /*!
        Properties common to all elements.
  */
  class Element : public RTX_object {
  public:
    
    using MetadataValueType = std::variant<double, std::string, TimeSeries::_sp>;
    
    enum element_t : int {
      JUNCTION  = 0,
      TANK      = 1,
      RESERVOIR = 2,
      PIPE      = 3,
      PUMP      = 4,
      VALVE     = 5,
      DMA       = 6
    };
    
    // please don't misuse this type enumeration!!!
    
    TSF_BASE_PROPS(Element);

    virtual std::ostream& toStream(std::ostream &stream);
    element_t type();
    void setName(const std::string& newName);
    std::string name();
    virtual void setRecord(PointRecord::_sp record);
    
    template <typename T>
    void setMetadataValue(const std::string& name, const T& value);
    
    void removeMetadataValue(const std::string& name);
    const MetadataValueType& getMetadataValue(const std::string& name) const;
    std::vector<std::string> getMetadataKeys();
      
    std::string userDescription();
    void setUserDescription(const std::string& description);
    
  protected:
    Element(const std::string& name);
    virtual ~Element();
    void setType(element_t type);
  
  private:
    std::string _name;
    element_t _type;
    std::string _userDescription;
    
    std::map<std::string, MetadataValueType> _metadata;
  };
  
  std::ostream& operator<< (std::ostream &out, Element &e);
}

#endif
