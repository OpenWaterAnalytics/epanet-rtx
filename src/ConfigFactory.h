//
//  ConfigFactory.h
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#ifndef epanet_rtx_ConfigFactory_h
#define epanet_rtx_ConfigFactory_h

#define CONFIGVERSION "1.0"

#include <libconfig.h++>

#include "ProjectFile.h"

namespace RTX {
  
  /*! \class ConfigFactory 
   \brief A factory for various elements, read from (or written to) a libconfig file.
 
   \fn void ConfigFactory::loadConfigFile(const std::string& path)
   \brief Create elements described in the libconfig file "path"
   \param path The path to the libconfig text file.
   
   \fn void ConfigFactory::saveConfigFile(const std::string& path)
   \brief Write the stored configuration to a text file
   \param path The path to the libconfig text file.
   
   \fn void ConfigFactory::clear()
   \brief Clear all configuration settings.
   
   \fn std::map<std::string, TimeSeries::sharedPointer> ConfigFactory::timeSeries()
   \brief Get a list of TimeSeries pointers held by the configuration object.
   \return The list of pointers.
   
   \fn std::map<std::string, PointRecord::sharedPointer> ConfigFactory::pointRecords()
   \brief Get a list of PointRecord pointers held by the configuration object.
   \return The list of pointers.
   
   \fn std::map<std::string, Clock::sharedPointer> ConfigFactory::clocks()
   \brief Get a list of Clock pointers held by the configuration object.
   \return The list of pointers.
   
   \fn void ConfigFactory::addTimeSeries(TimeSeries::sharedPointer timeSeries)
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
  
  class ConfigFactory : public ProjectFile {
  public:
    RTX_SHARED_POINTER(ConfigFactory);
    ConfigFactory();
    ~ConfigFactory();
    
    void loadProjectFile(const string& path);
    void saveProjectFile(const string& path);
    void clear();
    
    map<string, TimeSeries::sharedPointer> timeSeries();
    map<string, PointRecord::sharedPointer> pointRecords();
    map<string, Clock::sharedPointer> clocks();
    PointRecord::sharedPointer defaultRecord();
    Model::sharedPointer model();
    
    void configureElements(Model::sharedPointer model);
    
  protected:
    void createSimulationDefaults(Setting& setting);
    
    PointRecord::sharedPointer createPointRecordOfType(Setting& setting);
    Clock::sharedPointer createRegularClock(Setting& setting);
    TimeSeries::sharedPointer createTimeSeriesOfType(Setting& setting);
    void setGenericTimeSeriesProperties(TimeSeries::sharedPointer timeSeries, Setting& setting);
    TimeSeries::sharedPointer createTimeSeries(Setting& setting);
    TimeSeries::sharedPointer createMovingAverage(Setting& setting);
    TimeSeries::sharedPointer createAggregator(Setting& setting);
    TimeSeries::sharedPointer createResampler(Setting &setting);
    TimeSeries::sharedPointer createDerivative(Setting &setting);
    TimeSeries::sharedPointer createOffset(Setting &setting);
    TimeSeries::sharedPointer createThreshold(Setting &setting);
    TimeSeries::sharedPointer createCurveFunction(Setting &setting);
    TimeSeries::sharedPointer createConstant(Setting &setting);
    TimeSeries::sharedPointer createValidRange(Setting &setting);
    TimeSeries::sharedPointer createMultiplier(Setting &setting);
    TimeSeries::sharedPointer createRuntimeStatus(Setting &setting);

    
    void configureQualitySource(Setting &setting, Element::sharedPointer junction);
    void configureBoundaryFlow(Setting &setting, Element::sharedPointer junction);
    void configureHeadMeasure(Setting &setting, Element::sharedPointer junction);
    void configurePressureMeasure(Setting &setting, Element::sharedPointer junction);
    void configureLevelMeasure(Setting &setting, Element::sharedPointer tank);
    void configureQualityMeasure(Setting &setting, Element::sharedPointer junction);
    void configureBoundaryHead(Setting &setting, Element::sharedPointer junction);
    void configurePipeStatus(Setting &setting, Element::sharedPointer pipe);
    void configureFlowMeasure(Setting &setting, Element::sharedPointer pipe);
    void configurePumpCurve(Setting &setting, Element::sharedPointer pump);
    void configurePumpEnergyMeasure(Setting &setting, Element::sharedPointer pump);
    void configureValveSetting(Setting &setting, Element::sharedPointer valve);
    
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
    typedef TimeSeries::sharedPointer (ConfigFactory::*TimeSeriesFunctionPointer)(Setting&);
    typedef PointRecord::sharedPointer (*PointRecordFunctionPointer)(Setting&);
    typedef void (ConfigFactory::*ParameterFunction)(Setting&, Element::sharedPointer);
    map<string, PointRecordFunctionPointer> _pointRecordPointerMap;
    map<string, TimeSeriesFunctionPointer> _timeSeriesPointerMap;
    map<string, ParameterFunction> _parameterSetter;
    map<string, TimeSeries::sharedPointer> _timeSeriesList;
    map<string, PointRecord::sharedPointer> _pointRecordList;
    map<string, Clock::sharedPointer> _clockList;
    PointRecord::sharedPointer _defaultRecord;
    Model::sharedPointer _model;
    std::string _configPath;
    
    map<string, string> _timeSeriesSourceList;
    map<string, std::vector< std::pair<string, double> > > _timeSeriesAggregationSourceList;
    map<TimeSeries::sharedPointer,std::string> _multiplierBasisList;
  };
  
}

#endif
