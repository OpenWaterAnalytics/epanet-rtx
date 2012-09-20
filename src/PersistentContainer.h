//
//  PersistentContainer.h
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See Readme.txt and license.txt for more information
//  https://sourceforge.net/projects/epanet-rtx/

#ifndef epanet_rtx_PersistentContainer_h
#define epanet_rtx_PersistentContainer_h

#include "PointContainer.h"

namespace RTX {

  /*! \class PersistentContainer 
   \brief Derived class of PointContainer, An STL Container - like class for storing Point objects, ordered in time. 
   
   Extends PointContainer for data persistence. Works together with PointRecord derived classes for Point retrieval / storage. Also caches a range of points in its base class implementation.
   
   */
  
  class PersistentContainer : public PointContainer {
  public:
    // ctor/dtor
    PersistentContainer(const std::string &name, PointRecord::sharedPointer pointRecord);
    virtual ~PersistentContainer();
    
    // new public methods for derived class
    void setPointRecord(PointRecord::sharedPointer pointRecord);
    PointRecord::sharedPointer pointRecord();
    
    // extra value-added methods beyond STL-like behavior
    virtual void hintAtRange(time_t start, time_t end);
    virtual void hintAtBulkInsertion(time_t start, time_t end);
    //virtual std::vector< Point::sharedPointer > pointsInRange(time_t start, time_t end);
    virtual bool isPointAvailable(time_t time);
    virtual Point::sharedPointer findPoint(time_t time);
    virtual Point::sharedPointer pointAfter(time_t time);
    virtual Point::sharedPointer pointBefore(time_t time);
    virtual void insertPoint(Point::sharedPointer point);
    virtual void insertPoints(std::vector< Point::sharedPointer > points);
    
  protected:
    virtual std::ostream& toStream(std::ostream &stream);
    
  private:
    PointRecord::sharedPointer _pointRecord;
    std::string _name;  // this guy's record name
    std::string _id;  // identifier for passing to PointRecord (my nametag)
    
  };
  
  
  
  
  
  
  
}



#endif
