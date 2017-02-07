#ifndef EpanetModelExporter_hpp
#define EpanetModelExporter_hpp

#include <stdio.h>
#include <iostream>
#include "EpanetModel.h"

namespace RTX {
  class EpanetModelExporter {
  public:
    EpanetModelExporter(EpanetModel::_sp model, TimeRange range);
    std::ostream& to_stream(std::ostream &stream);
    
    // calibration files
    static void exportModel(EpanetModel::_sp model, TimeRange range, const std::string& dir, bool exportCalibration);
    
  private:
    TimeRange _range;
    EpanetModel::_sp _model;
  };
  std::ostream& operator<<(std::ostream& stream, EpanetModelExporter& exp);
}

#endif /* EpanetModelExporter_hpp */
