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
  #include <epanet2_2.h>
}

namespace RTX {
  
  /*!
   \class EpanetModel
   \brief An Epanet-based model object
   
   Provides a means of loading WDS models from *.inp files, and wraps the EPANET hydraulic engine (via a modified C toolkit embedded in this library)
   
   */
    
  class EpanetModel : public Model {
    
  public:
    RTX_BASE_PROPS(EpanetModel);
    EpanetModel();
    EpanetModel(const std::string& filename);
    EpanetModel(const EpanetModel& o); // copy constructor
    ~EpanetModel();
//    void loadModelFromFile(const std::string& filename) throw(std::exception);
    virtual void initEngine();
    virtual void closeEngine();
    void useEpanetModel(EN_Project model, string path);
    void useEpanetFile(const std::string& filename);
    virtual void overrideControls();
    virtual std::ostream& toStream(std::ostream &stream);
    EN_Project epanetModelPointer();
    
    int enIndexForJunction(Junction::_sp j);
    int enIndexForPipe(Pipe::_sp p);
    void useModelFromPath(const std::string& path) {this->useEpanetFile(path);};
    
    
    // overridden accessors
    // node elements
    double reservoirLevel(const std::string& reservoir);
    double tankLevel(const std::string& tank);
    double tankVolume(const std::string& tank);
    double tankFlow(const std::string& tank);
    double junctionDemand(const std::string& junction);
    double junctionHead(const std::string& junction);
    double junctionPressure(const std::string& junction);
    double junctionQuality(const std::string& junction);
    double tankInletQuality(const std::string& tank);
    // link elements
    double pipeFlow(const std::string& pipe);
    double pipeSetting(const string& pipe);
    double pipeStatus(const string& pipe);
    double pipeEnergy(const std::string& pump);
    
    // hydraulic
    void setReservoirHead(const std::string& reservoir, double level);
    void setReservoirQuality(const string& reservoir, double quality);
    void setTankLevel(const std::string& tank, double level);
    void setJunctionDemand(const std::string& junction, double demand);
    void setPipeStatus(const std::string& pipe, Pipe::status_t status);
    void setPipeStatusControl(const std::string& pipe, Pipe::status_t status, enableControl_t);
    void setPumpStatus(const std::string& pump, Pipe::status_t status);
    void setPumpStatusControl(const std::string& pump, Pipe::status_t status, enableControl_t);
    void setPumpSetting(const std::string& pump, double setting);
    void setPumpSettingControl(const std::string& pump, double setting, enableControl_t);
    void setValveSetting(const std::string& valve, double setting);
    void setValveSettingControl(const std::string& valve, double setting, enableControl_t);

    // quality
    void setJunctionQuality(const std::string& junction, double quality);
    
    void setQualityOptions(QualityType qt, const std::string& traceNode = "");
    QualityType qualityType();
    std::string qualityTraceNode();
    
    virtual void disableControls();
    virtual void enableControls();
    
  protected:
    
    // simulation methods
    virtual bool solveSimulation(time_t time);
    virtual time_t nextHydraulicStep(time_t time);
    virtual void stepSimulation(time_t time);
    virtual int iterations(time_t time);
    virtual double relativeError(time_t time);
    virtual void setHydraulicTimeStep(int seconds);
    virtual void setQualityTimeStep(int seconds);
    virtual void applyInitialQuality();
    virtual void applyInitialTankLevels();
    void EN_API_CHECK(int errorCode, std::string externalFunction);
    void updateEngineWithElementProperties(Element::_sp e);
    virtual void cleanupModelAfterSimulation();
    
    // protected accessors
    double getNodeValue(int epanetCode, const std::string& node);
    void setNodeValue(int epanetCode, const std::string& node, double value);
    double getLinkValue(int epanetCode, const std::string& link);
    void setLinkValue(int epanetCode, const std::string& link, double value);
    void setComment(Element::_sp element, const std::string& comment);
    
    EN_Project _enModel; // protected scope so subclasses can use epanet api
    
  private:
    std::map<std::string, int> _nodeIndex;
    std::map<std::string, int> _linkIndex;
    std::map<std::string, int> _statusControlIndex;
    std::map<std::string, int> _settingControlIndex;
    // TODO - use boost filesystem instead of std::string path
//    std::string _modelFile;
    
    void createRtxWrappers();
    bool _didConverge(time_t time, int errorCode);
    bool _enOpened;
    int _controlCount;
    std::string projectionString;
    
  };
  
}

#endif
