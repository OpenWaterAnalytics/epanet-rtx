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

#include "rtxMacros.h"
#include "TimeSeries.h"
#include "PointRecord.h"
#include "Junction.h"
#include "Tank.h"
#include "Reservoir.h"
#include "Link.h"
#include "Pipe.h"
#include "Pump.h"
#include "Valve.h"
#include "Model.h"

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
  
  // TODO - split this into separate factory classes (for pointrecords, timeseries, model, etc?)
  
  class ConfigFactory {
  public:
    RTX_SHARED_POINTER(ConfigFactory);
    ConfigFactory();
    ~ConfigFactory();
    
    void loadConfigFile(const string& path);
    void saveConfigFile(const string& path);
    void clear();
    
    map<string, TimeSeries::sharedPointer> timeSeries();
    map<string, PointRecord::sharedPointer> pointRecords();
    map<string, Clock::sharedPointer> clocks();
    PointRecord::sharedPointer defaultRecord();
    Model::sharedPointer model();
    
    vector<string> elementList();
    void configureElements(vector<Element::sharedPointer> elements);
    void configureElement(Element::sharedPointer element);
    
  protected:
    void createSimulationDefaults(Setting& setting);
    
    PointRecord::sharedPointer createPointRecordOfType(Setting& setting);
    PointRecord::sharedPointer createScadaPointRecord(Setting& setting);
    PointRecord::sharedPointer createMySqlPointRecord(Setting& setting);
    Clock::sharedPointer createRegularClock(Setting& setting);
    TimeSeries::sharedPointer createTimeSeriesOfType(Setting& setting);
    void setGenericTimeSeriesProperties(TimeSeries::sharedPointer timeSeries, Setting& setting);
    TimeSeries::sharedPointer createTimeSeries(Setting& setting);
    TimeSeries::sharedPointer createMovingAverage(Setting& setting);
    TimeSeries::sharedPointer createAggregator(Setting& setting);
    TimeSeries::sharedPointer createResampler(Setting &setting);
    TimeSeries::sharedPointer createDerivative(Setting &setting);
    TimeSeries::sharedPointer createOffset(Setting &setting);
    
    void configureQualitySource(Setting &setting, Element::sharedPointer junction);
    void configureBoundaryFlow(Setting &setting, Element::sharedPointer junction);
    void configureHeadMeasure(Setting &setting, Element::sharedPointer junction);
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
    void createZones(Setting& zoneGroup);
    
    bool _doesHaveStateRecord;
    
    Config _configuration;
    typedef PointRecord::sharedPointer (ConfigFactory::*PointRecordFunctionPointer)(Setting&);
    typedef TimeSeries::sharedPointer (ConfigFactory::*TimeSeriesFunctionPointer)(Setting&);
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
  };
  
}

#endif
