//
//  ConfigProject.h
//  epanet-rtx
//
//  Open Water Analytics [wateranalytics.org]
//  See README.md and license.txt for more information
//  

#ifndef epanet_rtx_ConfigProject_h
#define epanet_rtx_ConfigProject_h

#define CONFIGVERSION "1.0"

#include "ProjectFile.h"

namespace RTX {
  
  /*! \class ConfigProject 
   \brief A factory for various elements, read from (or written to) a libconfig file.
 
   \fn void ConfigProject::loadConfigFile(const std::string& path)
   \brief Create elements described in the libconfig file "path"
   \param path The path to the libconfig text file.
   
   \fn void ConfigProject::saveConfigFile(const std::string& path)
   \brief Write the stored configuration to a text file
   \param path The path to the libconfig text file.
   
   \fn void ConfigProject::clear()
   \brief Clear all configuration settings.
   
   \fn std::map<std::string, TimeSeries::_sp> ConfigProject::timeSeries()
   \brief Get a list of TimeSeries pointers held by the configuration object.
   \return The list of pointers.
   
   \fn std::map<std::string, PointRecord::_sp> ConfigProject::pointRecords()
   \brief Get a list of PointRecord pointers held by the configuration object.
   \return The list of pointers.
   
   \fn std::map<std::string, Clock::_sp> ConfigProject::clocks()
   \brief Get a list of Clock pointers held by the configuration object.
   \return The list of pointers.
   
   \fn void ConfigProject::addTimeSeries(TimeSeries::_sp timeSeries)
   \brief add a TimeSeries pointer to the configuration
   \param timeSeries A TimeSeries shared pointer
   

   */
  
  using libconfig::Setting;
  using libconfig::Config;
  using std::string;
  using std::map;
  using std::vector;
  
  class PointRecordFactory;
  
  // TODO - split this into separate factory classes (for pointrecords, timeseries, model, etc?)
  
  class ConfigProject : public ProjectFile {
  public:
    RTX_SHARED_POINTER(ConfigProject);
    ConfigProject();
    ~ConfigProject();
    
    void loadProjectFile(const string& path);
    void saveProjectFile(const string& path);
    void clear();
    
    RTX_LIST<TimeSeries::_sp> timeSeries();
    RTX_LIST<Clock::_sp> clocks();
    RTX_LIST<PointRecord::_sp> records();
    
    PointRecord::_sp defaultRecord();
    Model::_sp model();
    
    void configureElements(Model::_sp model);
    
  protected:
    void createSimulationDefaults(Setting& setting);
    
    PointRecord::_sp createPointRecordOfType(Setting& setting);
    Clock::_sp createRegularClock(Setting& setting);
    TimeSeries::_sp createTimeSeriesOfType(Setting& setting);
    void setGenericTimeSeriesProperties(TimeSeries::_sp timeSeries, Setting& setting);
    TimeSeries::_sp createTimeSeries(Setting& setting);
    TimeSeries::_sp createMovingAverage(Setting& setting);
    TimeSeries::_sp createAggregator(Setting& setting);
    TimeSeries::_sp createResampler(Setting &setting);
    TimeSeries::_sp createDerivative(Setting &setting);
    TimeSeries::_sp createOffset(Setting &setting);
    TimeSeries::_sp createThreshold(Setting &setting);
    TimeSeries::_sp createCurveFunction(Setting &setting);
    TimeSeries::_sp createConstant(Setting &setting);
    TimeSeries::_sp createValidRange(Setting &setting);
    TimeSeries::_sp createMultiplier(Setting &setting);
    TimeSeries::_sp createRuntimeStatus(Setting &setting);
    TimeSeries::_sp createGain(Setting &setting);


    void configureQualitySource(Setting &setting, Element::_sp junction);
    void configureBoundaryFlow(Setting &setting, Element::_sp junction);
    void configureHeadMeasure(Setting &setting, Element::_sp junction);
    void configurePressureMeasure(Setting &setting, Element::_sp junction);
    void configureLevelMeasure(Setting &setting, Element::_sp tank);
    void configureQualityMeasure(Setting &setting, Element::_sp junction);
    void configureBoundaryHead(Setting &setting, Element::_sp junction);
    void configurePipeStatus(Setting &setting, Element::_sp pipe);
    void configurePipeSetting(Setting &setting, Element::_sp pipe);
    void configureFlowMeasure(Setting &setting, Element::_sp pipe);
    void configurePumpCurve(Setting &setting, Element::_sp pump);
    void configurePumpEnergyMeasure(Setting &setting, Element::_sp pump);
    
    void createModel(Setting& setting);
    
  private:
    void createPointRecords(Setting& records);
    void createClocks(Setting& clockGroup);
    void createTimeSeriesList(Setting& timeSeriesGroup);
    void createDmaObjs(Setting& dmaGroup);
    void createSaveOptions(Setting& saveGroup);
    double getConfigDouble(Setting& config, const std::string& name);
    bool _doesHaveStateRecord;
    
    Config _configuration;
    typedef TimeSeries::_sp (ConfigProject::*TimeSeriesFunctionPointer)(Setting&);
    typedef PointRecord::_sp (*PointRecordFunctionPointer)(Setting&);
    typedef void (ConfigProject::*ParameterFunction)(Setting&, Element::_sp);
    map<string, PointRecordFunctionPointer> _pointRecordPointerMap;
    map<string, TimeSeriesFunctionPointer> _timeSeriesPointerMap;
    map<string, ParameterFunction> _parameterSetter;
    map<string, TimeSeries::_sp> _timeSeriesList;
    map<string, PointRecord::_sp> _pointRecordList;
    map<string, Clock::_sp> _clockList;
    PointRecord::_sp _defaultRecord;
    Model::_sp _model;
    std::string _configPath;
    
    map<string, string> _timeSeriesSourceList;
    map<string, std::vector< std::pair<string, double> > > _timeSeriesAggregationSourceList;
    map<TimeSeries::_sp,std::string> _multiplierBasisList;
  };
  
}

#endif
