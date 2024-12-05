#ifndef EpanetModelExporter_hpp
#define EpanetModelExporter_hpp

#include <stdio.h>
#include <iostream>
#include "EpanetModel.h"

namespace RTX {
  class EpanetModelExporter {
  public:
    
    enum ExportType {
      Snapshot,
      Forecast
    };
    
    EpanetModelExporter(EpanetModel::_sp model, TSF::TimeRange range, ExportType exportType);
    std::ostream& to_stream(std::ostream &stream);
    
    // calibration files
    static void exportModel(EpanetModel::_sp model, TSF::TimeRange range, const std::string& dir, bool exportCalibration, ExportType exportType);
    
  private:
    void replaceControlsInStream(std::ifstream &original, std::ostream &stream);

    TSF::TimeRange _range;
    EpanetModel::_sp _model;
    ExportType _exportType;
  };
  
  std::ostream& operator<<(std::ostream& stream, EpanetModelExporter& exp);
  
}

#endif /* EpanetModelExporter_hpp */
