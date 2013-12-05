//
//  EpanetModel.h
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#ifndef epanet_rtx_EpanetModel_h
#define epanet_rtx_EpanetModel_h

#include "Model.h"
#include "rtxMacros.h"

extern "C" {
  #define EN_API_FLOAT_TYPE double
  #include <epanet2.h>
}

namespace RTX {
  
  /*!
   \class EpanetModel
   \brief An Epanet-based model object
   
   Provides a means of loading WDS models from *.inp files, and wraps the EPANET hydraulic engine (via a modified C toolkit embedded in this library)
   
   */
    
  class EpanetModel : public Model {
    
  public:
    RTX_SHARED_POINTER(EpanetModel);
    EpanetModel();
    ~EpanetModel();
    void loadModelFromFile(const std::string& filename) throw(std::exception);
    void useEpanetFile(const std::string& filename);
    virtual void overrideControls() throw(RtxException);
    virtual std::ostream& toStream(std::ostream &stream);

  protected:
    // overridden accessors
    // node elements
    double reservoirLevel(const std::string& reservoir);
    double tankLevel(const std::string& tank);
    double junctionDemand(const std::string& junction);
    double junctionHead(const std::string& junction);
    double junctionQuality(const std::string& junction);
    // link elements
    double pipeFlow(const std::string& pipe);
    double pumpEnergy(const std::string& pump);
    
    // hydraulic
    void setReservoirHead(const std::string& reservoir, double level);
    void setTankLevel(const std::string& tank, double level);
    void setJunctionDemand(const std::string& junction, double demand);
    void setPipeStatus(const std::string& pipe, Pipe::status_t status);
    void setPumpStatus(const std::string& pump, Pipe::status_t status);
    void setPumpSetting(const std::string& pump, double setting);
    void setValveSetting(const std::string& valve, double setting);
    
    // quality
    void setJunctionQuality(const std::string& junction, double quality);
    
    // simulation methods
    virtual void solveSimulation(time_t time);
    virtual time_t nextHydraulicStep(time_t time);
    virtual void stepSimulation(time_t time);
    virtual int iterations(time_t time);
    virtual int relativeError(time_t time);
    virtual void setHydraulicTimeStep(int seconds);
    virtual void setQualityTimeStep(int seconds);
    virtual void setInitialQuality(double qual);
    void ENcheck(int errorCode, std::string externalFunction) throw(std::string);
    
    // protected accessors
    double getNodeValue(int epanetCode, const std::string& node);
    void setNodeValue(int epanetCode, const std::string& node, double value);
    double getLinkValue(int epanetCode, const std::string& link);
    void setLinkValue(int epanetCode, const std::string& link, double value);
    
  private:
    std::map<std::string, int> _nodeIndex;
    std::map<std::string, int> _linkIndex;
    // TODO - use boost filesystem instead of std::string path
//    std::string _modelFile;
    
    void createRtxWrappers();
    
  };
  
}

#endif
