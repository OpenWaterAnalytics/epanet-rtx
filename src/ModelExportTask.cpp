#include "ModelExportTask.h"


using namespace std;
using namespace RTX;


ModelExportTask::ModelExportTask(EpanetModel::_sp m) : _model(m) {
  
  
  
}


std::ostream& RTX::operator<< (std::ostream &out, const ModelExportTask& task) {
  
  
  
  
  return out;
}