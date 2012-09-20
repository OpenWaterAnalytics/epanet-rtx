//
//  PointContainer.h
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See Readme.txt and license.txt for more information
//  http://tinyurl.com/epanet-rtx

#ifndef epanet_rtx_PointContainer_h
#define epanet_rtx_PointContainer_h

#define POINTCONTAINER_CACHESIZE 10000;

#include <map>
#include <time.h>
#include <vector>
#include "Point.h"
#include "PointRecord.h"
#include "rtxMacros.h"

namespace RTX {
    
  //class PointContainerIterator;
  
  
  /*! \class PointContainer 
      \brief A Container class for storing Points, ordered in time. 
  
      Derived classes may choose to extend the storage functionality for data persistence or acquisition.
  */
  
  class PointContainer {
    //friend class PointContainerIterator;
  public:
    RTX_SHARED_POINTER(PointContainer);
    PointContainer();
    virtual ~PointContainer();
     
    bool isEmpty();
    size_t size();
    //Point::sharedPointer operator[](const time_t& time);
    friend std::ostream& operator<< (std::ostream &out, PointContainer &pointContainer);
    
    virtual void hintAtRange(time_t start, time_t end) {};
    virtual void hintAtBulkInsertion(time_t start, time_t end) {};
    //virtual std::vector< Point::sharedPointer > pointsInRange(time_t start, time_t end);
    virtual bool isPointAvailable(time_t time);
    virtual Point::sharedPointer findPoint(time_t time);
    virtual Point::sharedPointer pointAfter(time_t time);
    virtual Point::sharedPointer pointBefore(time_t time);
    virtual Point::sharedPointer firstPoint();
    virtual Point::sharedPointer lastPoint();
    virtual long int numberOfPoints();
    virtual void insertPoint(Point::sharedPointer point);
    virtual void insertPoints(std::vector< Point::sharedPointer > points);
    
    virtual void setCacheSize(int size);
    int cacheSize();
    
    virtual void reset();
    
  protected:
    virtual std::ostream& toStream(std::ostream &stream);
    
  private:
    //typedef std::map<time_t,Point::sharedPointer> PointMap_t;
    //PointMap_t _points;
    typedef std::map<time_t, std::pair<double,double> > PairMap_t;
    PairMap_t _pairMap;
    int _cacheSize;
    
    Point::sharedPointer makePoint(PairMap_t::iterator iterator);
  };
  
  
}

#endif
