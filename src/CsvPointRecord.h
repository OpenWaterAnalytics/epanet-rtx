//
//  CsvPointRecord.h
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//

#ifndef __epanet_rtx__CsvPointRecord__
#define __epanet_rtx__CsvPointRecord__

#include <iostream>
#include "DbPointRecord.h"

#include <boost/filesystem.hpp>

namespace RTX {
  class CsvPointRecord : public DbPointRecord {
  public:
    RTX_SHARED_POINTER(CsvPointRecord);
    CsvPointRecord();
    
    virtual void connect() throw(RtxException);
    virtual bool isConnected();
    virtual std::string registerAndGetIdentifier(std::string recordName);
    //virtual std::vector<std::string> identifiers();
    //virtual time_pair_t range(const std::string& id);
    virtual std::ostream& toStream(std::ostream &stream);
    
  protected:
    virtual std::vector<Point> selectRange(const std::string& id, time_t startTime, time_t endTime);
    virtual Point selectNext(const std::string& id, time_t time);
    virtual Point selectPrevious(const std::string& id, time_t time);
    
    // insertions or alterations may choose to ignore / deny
    virtual void insertSingle(const std::string& id, Point point);
    virtual void insertRange(const std::string& id, std::vector<Point> points);
    virtual void removeRecord(const std::string& id);
    virtual void truncate();
    
  private:
    bool _isReadonly;
    bool _isConnected;
    boost::filesystem::path _path;
    void loadDataFromCsvDir(boost::filesystem::path dirPath);
  };
}




#endif /* defined(__epanet_rtx__CsvPointRecord__) */
