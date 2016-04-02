#ifndef ModelExportTask_h
#define ModelExportTask_h

#include <stdio.h>
#include <iostream>

#include "EpanetModel.h"

namespace RTX {
  class ModelExportTask {
    
  public:
    ModelExportTask(EpanetModel::_sp model);
  private:
    EpanetModel::_sp _model;
    
  };
  std::ostream& operator<<(std::ostream& ost, const ModelExportTask& task);
}




#endif /* ModelExportTask_hpp */
