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
#include <unordered_map>
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
  class Element : public RTX_object {
  public:
    
    using MetadataValueType = std::variant<double, std::string, std::shared_ptr<TimeSeries>>;
    
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
    
    RTX_BASE_PROPS(Element);

    virtual std::ostream& toStream(std::ostream &stream);
    element_t type();
    void setName(const std::string& newName);
    std::string name();
    virtual void setRecord(PointRecord::_sp record);
    
    template <typename T>
    void addMetadata(const std::string& name, const T& value);
    void removeMetadata(const std::string& name);
    const MetadataValueType& getMetadata(const std::string& name) const;
    MetadataValueType& getMetadata(const std::string& name);
      
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
    
    std::unordered_map<std::string, MetadataValueType> metadata;
  };
  
  std::ostream& operator<< (std::ostream &out, Element &e);
}

#endif
