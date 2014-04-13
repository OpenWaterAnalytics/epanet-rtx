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
#include "MapPointRecord.h"

#include <boost/filesystem.hpp>

#define RTX_CSVPR_SUPER MapPointRecord

namespace RTX {
  class CsvPointRecord : public RTX_CSVPR_SUPER {
  public:
    RTX_SHARED_POINTER(CsvPointRecord);
    CsvPointRecord();
    virtual ~CsvPointRecord();
    
    void setPath(const std::string& path); // set csv directory and load data
    void setReadOnly(bool readOnly);
    bool isReadOnly();
    
    virtual std::ostream& toStream(std::ostream &stream);
    
    
  private:
    bool _isReadonly;
    boost::filesystem::path _path;
    void loadDataFromCsvDir(boost::filesystem::path dirPath);
    void saveDataToCsvDir(boost::filesystem::path dirPath);
  };
}




#endif /* defined(__epanet_rtx__CsvPointRecord__) */
