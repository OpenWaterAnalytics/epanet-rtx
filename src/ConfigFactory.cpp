//
//  ConfigFactory.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#include <iostream>
#include <boost/foreach.hpp>
#include <boost/filesystem.hpp>

#include "ConfigFactory.h"
#include "AggregatorTimeSeries.h"
#include "MovingAverage.h"
#include "Resampler.h"
#include "ConstantSeries.h"
#include "FirstDerivative.h"
#include "PointRecord.h"
#include "ScadaPointRecord.h"
#include "MysqlPointRecord.h"
#include "Zone.h"
#include "EpanetModel.h"
#include "EpanetSyntheticModel.h"

using namespace RTX;
using namespace libconfig;

using std::vector;
using std::string;

#pragma mark Constructor/Destructor

ConfigFactory::ConfigFactory() {
  // register point record and time series types to their proper creators
  _pointRecordPointerMap.insert(std::make_pair("SCADA", &ConfigFactory::createScadaPointRecord));
  _pointRecordPointerMap.insert(std::make_pair("MySQL", &ConfigFactory::createMySqlPointRecord));
  
  //_clockPointerMap.insert(std::make_pair("regular", &ConfigFactory::createRegularClock));
  //_clockPointerMap.insert(std::make_pair("irregular", &ConfigFactory::createIrregularClock));
  
  _timeSeriesPointerMap.insert(std::make_pair("TimeSeries", &ConfigFactory::createTimeSeries));
  _timeSeriesPointerMap.insert(std::make_pair("MovingAverage", &ConfigFactory::createMovingAverage));
  _timeSeriesPointerMap.insert(std::make_pair("Aggregator", &ConfigFactory::createAggregator));
  _timeSeriesPointerMap.insert(std::make_pair("Resample", &ConfigFactory::createResampler));
  _timeSeriesPointerMap.insert(std::make_pair("Constant", &ConfigFactory::createConstant));
  _timeSeriesPointerMap.insert(std::make_pair("Derivative", &ConfigFactory::createDerivative));
  
  // node-type configuration functions
  // Junctions
  _parameterSetter.insert(std::make_pair("qualitysource", &ConfigFactory::configureQualitySource));
  _parameterSetter.insert(std::make_pair("quality", &ConfigFactory::configureQualityMeasure));
  _parameterSetter.insert(std::make_pair("boundaryflow", &ConfigFactory::configureBoundaryFlow));
  _parameterSetter.insert(std::make_pair("headmeasure", &ConfigFactory::configureHeadMeasure));
  // Tanks, Reservoirs
  _parameterSetter.insert(std::make_pair("boundaryhead", &ConfigFactory::configureBoundaryHead));
  
  // link-type configuration functions
  // Pipes
  _parameterSetter.insert(std::make_pair("status", &ConfigFactory::configurePipeStatus));
  _parameterSetter.insert(std::make_pair("flow", &ConfigFactory::configureFlowMeasure));
  // Pumps
  _parameterSetter.insert(std::make_pair("curve", &ConfigFactory::configurePumpCurve));
  _parameterSetter.insert(std::make_pair("energy", &ConfigFactory::configurePumpEnergyMeasure));
  // Valves
  _parameterSetter.insert(std::make_pair("setting", &ConfigFactory::configureValveSetting));
  
}

ConfigFactory::~ConfigFactory() {
  _pointRecordPointerMap.clear();
  _timeSeriesPointerMap.clear();
  _timeSeriesList.clear();
  _clockList.clear();
  _pointRecordList.clear();
}

#pragma mark - Loading File

void ConfigFactory::loadConfigFile(const std::string& path) {
  
  _configPath = boost::filesystem::path(path);
  
  // use libconfig api to open config file
  try
  {
    _configuration.readFile(_configPath.c_str());
  }
  catch(const FileIOException &fioex)
  {
    std::cerr << "I/O error while reading file." << std::endl;
    return;
  }
  catch(const ParseException &pex)
  {
    std::cerr << "Parse error at " << pex.getFile() << ":" << pex.getLine() << " - " << pex.getError() << std::endl;
    return;
  }
  
  // get the root setting node from the configuration
  Setting& root = _configuration.getRoot();
  
  // get the version number
  std::string version = _configuration.lookup("version");
  // todo -- check version number against CONFIGVERSION
  
  
  // access the settings.
  // open the config root, and get the length of the pointrecord group.
  // create the path if it doesn't exist yet.
  
  if ( !root.exists("configuration") ) {
    root.add("configuration", Setting::TypeGroup);
  }
  Setting& config = root["configuration"];
  
  
  if ( !config.exists("records") ) {
    config.add("records", Setting::TypeList);
  }
  Setting& records = config["records"];
  
  
  if ( !config.exists("clocks") ) {
    config.add("clocks", Setting::TypeList);
  }
  Setting& clockGroup = config["clocks"];
  
  
  if ( !config.exists("timeseries") ) {
    config.add("timeseries", Setting::TypeList);
  }
  Setting& timeSeriesGroup = config["timeseries"];
  
  
  if ( !config.exists("simulation") ) {
    config.add("simulation", Setting::TypeList);
  }
  Setting& simulationGroup = config["simulation"];
  
  
  if ( !config.exists("model") ) {
    config.add("model", Setting::TypeList);
  }
  Setting& modelGroup = config["model"];
  
  
  if ( !config.exists("zones") ) {
    config.add("zones", Setting::TypeList);
  }
  Setting& zoneGroup = config["zones"];
  
  // get the first set - point records.
  createPointRecords(records);
  
  // get clocks
  createClocks(clockGroup);
  
  // get timeseries
  createTimeSeriesList(timeSeriesGroup);
  
  // make a new model
  createModel(modelGroup);
  
  // make zones
  createZones(zoneGroup);
  
  // set defaults
  createSimulationDefaults(simulationGroup);
  
}


std::map<std::string, TimeSeries::sharedPointer> ConfigFactory::timeSeries() {
  return _timeSeriesList;
}

std::map<std::string, PointRecord::sharedPointer> ConfigFactory::pointRecords() {
  return _pointRecordList;
}

PointRecord::sharedPointer ConfigFactory::defaultRecord() {
  return _defaultRecord;
}

std::map<std::string, Clock::sharedPointer> ConfigFactory::clocks() {
  return _clockList;
}


#pragma mark - PointRecord

void ConfigFactory::createPointRecords(Setting& records) {  
  
  int recordCount = records.getLength();
  
  // loop through the records and create them.
  for (int iRecord = 0; iRecord < recordCount; ++iRecord) {
    Setting& record = records[iRecord];
    string recordName = record["name"];
    PointRecord::sharedPointer pointRecord = createPointRecordOfType(record);
    if (pointRecord) {
      _pointRecordList[recordName] = pointRecord;
    }
    else {
      std::cerr << "could not load point record\n";
    }
    
  }

  
  
  return;
}

// simple layer of indirection to make function pointer execution cleaner in the calling code.
// this just executes a function pointer stored in a map, which is keyed with the string name of the type of pointrecord to create.
// so access the "type" field of the passed setting, and execute the proper function to create the pointrecord.
PointRecord::sharedPointer ConfigFactory::createPointRecordOfType(libconfig::Setting &setting) {
  // TODO - check if the map item exists first
  PointRecordFunctionPointer fp = _pointRecordPointerMap[setting["type"]];
  return (this->*fp)(setting);
}

PointRecord::sharedPointer ConfigFactory::createScadaPointRecord(libconfig::Setting &setting) {
  
  libconfig::Setting& connection = setting["connection"];
  // create the initialization string for the scada point record.
  // as you can see, this is a DSN-less connection, which only requires that the driver be registered in odbcinst.ini
  string driver     = connection["DRIVER"];
  string server     = connection["SERVER"];
  string uid        = connection["UID"];
  string pwd        = connection["PWD"];
  string database   = connection["DATABASE"];
  string tdsVersion = connection["TDS_Version"];
  string port       = connection["Port"];
  
  libconfig::Setting& syntax = setting["syntax"];
  string table    = syntax["Table"];
  string dateCol  = syntax["DateColumn"];
  string tagCol   = syntax["TagColumn"];
  string valueCol = syntax["ValueColumn"];
  string qualCol  = syntax["QualityColumn"];
  
  string initString = "DRIVER=" + driver + ";SERVER=" + server + ";UID=" + uid + ";PWD=" + pwd + ";DATABASE=" + database + ";TDS_Version=" + tdsVersion + ";Port=" + port + ";";
  
  ScadaPointRecord::sharedPointer pointRecord( new ScadaPointRecord() );
  pointRecord->setSyntax(table, dateCol, tagCol, valueCol, qualCol);
  pointRecord->connect(initString);
  
  return pointRecord;
}

PointRecord::sharedPointer ConfigFactory::createMySqlPointRecord(libconfig::Setting &setting) {
  string name = setting["name"];
  libconfig::Setting& connection = setting["connection"];
  string host = connection["HOST"];
  string user = connection["UID"];
  string password = connection["PWD"];
  string database = connection["DB"];
  MysqlPointRecord::sharedPointer record( new MysqlPointRecord() );
  record->connect(host, user, password, database);
  bool isGood = false;
  isGood = record->isConnected();
  return record;
}





#pragma mark - Clocks

// keeping the clock creation simple for now - e.g., no function pointers.
void ConfigFactory::createClocks(Setting& clockGroup) {
  int clockCount = clockGroup.getLength();
  for (int iClock = 0; iClock < clockCount; ++iClock) {
    Setting& clock = clockGroup[iClock];
    string clockName = clock["name"];
    int period = clock["period"];
    Clock::sharedPointer aClock( new Clock(period) );
    _clockList[clockName] = aClock;
  }
  
  return;
}



#pragma mark - TimeSeries

void ConfigFactory::createTimeSeriesList(Setting& timeSeriesGroup) {  
  int tsCount = timeSeriesGroup.getLength();
  // loop through the time series and create them.
  for (int iSeries = 0; iSeries < tsCount; ++iSeries) {
    Setting& series = timeSeriesGroup[iSeries];
    string seriesName = series["name"];
    TimeSeries::sharedPointer theTimeSeries = createTimeSeriesOfType(series);
    if (theTimeSeries != NULL) {
      _timeSeriesList[seriesName] = theTimeSeries;
    }
  }
  
  return;
}

TimeSeries::sharedPointer ConfigFactory::createTimeSeriesOfType(libconfig::Setting &setting) {
  std::string type = setting["type"];
  if (_timeSeriesPointerMap.find(type) == _timeSeriesPointerMap.end()) {
    // not found
    std::cerr << "time series type " << type << " not implemented or not recognized" << std::endl;
    TimeSeries::sharedPointer empty;
    return empty;
  }
  TimeSeriesFunctionPointer fp = _timeSeriesPointerMap[type];
  return (this->*fp)(setting);
}

void ConfigFactory::setGenericTimeSeriesProperties(TimeSeries::sharedPointer timeSeries, libconfig::Setting &setting) {
  timeSeries->setName(setting["name"]);
  
  // if a pointRecord is specified, then re-set the timeseries' cache.
  // this means that the storage for the time series is probably external (scada / mysql).
  if (setting.exists("pointRecord")) {
    // TODO - test for existence of the actual point record.
    std::string pointRecordName = setting["pointRecord"];
    PointRecord::sharedPointer pointRecord = _pointRecordList[setting["pointRecord"]];
    timeSeries->newCacheWithPointRecord(pointRecord);
  }
  
  if (setting.exists("clock")) {
    Clock::sharedPointer clock = _clockList[setting["clock"]];
    timeSeries->setClock(clock);
  }
  
  // set any upstream sources. they must already exist. no forward declarations allowed in the config file.
  if (setting.exists("source")) {
    // this timeseries has an upstream source. connect it.
    string sourceName = setting["source"];
    if (_timeSeriesList.find(sourceName) == _timeSeriesList.end()) {
      std::cerr << "cannot locate Timeseries source for " << timeSeries->name() << std::endl;
    }
    else {
      TimeSeries::sharedPointer sourceTimeSeries = _timeSeriesList[sourceName];
      // cast to derived type so we can access extra methods
      ModularTimeSeries::sharedPointer thisTimeSeries = boost::static_pointer_cast<ModularTimeSeries>(timeSeries);
      thisTimeSeries->setSource(sourceTimeSeries);
    }
  }
  
  // TODO -- units, description field
  
  Units theUnits(RTX_DIMENSIONLESS);
  if (setting.exists("units")) {
    string unitName = setting["units"];
    theUnits = Units::unitOfType(unitName);
  }
  timeSeries->setUnits(theUnits);
  
}

TimeSeries::sharedPointer ConfigFactory::createTimeSeries(libconfig::Setting &setting) {
  TimeSeries::sharedPointer timeSeries( new TimeSeries() );
  setGenericTimeSeriesProperties(timeSeries, setting);
  return timeSeries;
}

TimeSeries::sharedPointer ConfigFactory::createAggregator(libconfig::Setting &setting) {
  AggregatorTimeSeries::sharedPointer timeSeries( new AggregatorTimeSeries() );
  // set generic properties
  setGenericTimeSeriesProperties(timeSeries, setting);
  // additional setters for this class...
  // a list of sources with multipliers.
  Setting& sources = setting["sources"];
  int sourceCount = sources.getLength();
  for (int iSource = 0; iSource < sourceCount; ++iSource) {
    Setting& thisSource = sources[iSource];
    TimeSeries::sharedPointer sourceTS;
    string sourceName = thisSource["source"];
    double multiplier;
    // set the multiplier if it's specified - otherwise, set to 1.
    if ( !thisSource.lookupValue("multiplier", multiplier) ) {
      multiplier = 1.;
    }
    if (_timeSeriesList.find(sourceName) == _timeSeriesList.end()) {
      std::cerr << "could not find source timeseries (" << sourceName << ") for " << timeSeries->name() << std::endl;
    }
    else {
      sourceTS = _timeSeriesList[sourceName];
      timeSeries->addSource(sourceTS, multiplier);
    }
  }
  return timeSeries;
}

TimeSeries::sharedPointer ConfigFactory::createMovingAverage(libconfig::Setting &setting) {
  MovingAverage::sharedPointer timeSeries( new MovingAverage() );
  // set generic properties
  setGenericTimeSeriesProperties(timeSeries, setting);
  
  // class-specific settings
  int window = setting["window"];
  timeSeries->setWindowSize(window);
  
  TimeSeries::sharedPointer returnTS = timeSeries;
  return returnTS;
}

TimeSeries::sharedPointer ConfigFactory::createResampler(libconfig::Setting &setting) {
  Resampler::sharedPointer resampler( new Resampler() );
  setGenericTimeSeriesProperties(resampler, setting);
  return resampler;
}

TimeSeries::sharedPointer ConfigFactory::createConstant(libconfig::Setting &setting) {
  ConstantSeries::sharedPointer constant( new ConstantSeries() );
  setGenericTimeSeriesProperties(constant, setting);
  double value = setting["value"];
  constant->setValue(value);
  return constant;
}

TimeSeries::sharedPointer ConfigFactory::createDerivative(Setting &setting) {
  FirstDerivative::sharedPointer derivative( new FirstDerivative() );
  setGenericTimeSeriesProperties(derivative, setting);
  return derivative;
}


#pragma mark - Model

void ConfigFactory::createModel(Setting& setting) {
  
  std::string modelType = setting["type"];
  string modelFileName = setting["file"];
  boost::filesystem::path modelPath = _configPath.parent_path();
  modelPath /= modelFileName;
  
  if ( RTX_STRINGS_ARE_EQUAL(modelType, "epanet") ){
    _model.reset( new EpanetModel() );    
    // load the model
    _model->loadModelFromFile(modelPath.string());
    // hook up the model's elements to timeseries objects
    _model->overrideControls();
    configureElements(_model->elements());
  }
  
  if ( RTX_STRINGS_ARE_EQUAL(modelType, "synthetic_epanet") ) {
    _model.reset( new EpanetSyntheticModel() );
    _model->loadModelFromFile(modelPath.string());
    //configureElements(_model->elements());
  }
  

}

Model::sharedPointer ConfigFactory::model() {
  return _model;
}

#pragma mark - Simulation Settings

void ConfigFactory::createSimulationDefaults(Setting& setting) {
  std::string defaultRecordName = setting["staterecord"];
  if (_pointRecordList.find(defaultRecordName) == _pointRecordList.end()) {
    std::cerr << "could not retrieve point record by name: " << defaultRecordName << std::endl;
  }
  _defaultRecord = _pointRecordList[defaultRecordName];
  // todo - error checking for all these lookups

  // get other simulation settings
  Setting& timeSetting = setting["time"];
  const int hydStep = timeSetting["hydraulic"];
  const int qualStep = timeSetting["quality"];
  
  // provide the model object with this record
  _model->setStorage(_defaultRecord);
  // set other sim properties...
  _model->setHydraulicTimeStep(hydStep);
  _model->setQualityTimeStep(qualStep);
}

#pragma mark - Zone Settings

void ConfigFactory::createZones(Setting& zoneGroup) {
  // get the zone information from the config,
  // then create each zone and add it to the model.
  if ( zoneGroup.exists("demand_areas") ) {
    Setting& zoneList = zoneGroup["demand_areas"];
    const int zoneCount = zoneList.getLength();
    
    for (int iZone = 0; iZone < zoneCount; ++iZone) {
      // for each zone listed, create a new Zone object and populate it with the proper nodes.
      Setting& zoneSetting = zoneList[iZone];
      string name = zoneSetting["id"];
      string junctionID = zoneSetting["junction"];
      Zone::sharedPointer zone( new Zone(name) );
      Junction::sharedPointer junction = boost::dynamic_pointer_cast<Junction>(_model->nodeWithName(junctionID));
      zone->addJunctionTree(junction);
      TimeSeries::sharedPointer zoneDemand;
      std::string zoneDemandName = zoneSetting["demand"];
      if (_timeSeriesList.find(zoneDemandName) == _timeSeriesList.end()) {
        std::cerr << "could not find the zone demand time series \"" << zoneDemandName << "\"" << std::endl;
      }
      zoneDemand = _timeSeriesList[zoneDemandName];
      zone->setDemand(zoneDemand);
      _model->addZone(zone);
    }
  }
  
}



#pragma mark - Element Configuration


void ConfigFactory::configureElements(std::vector<Element::sharedPointer> elements) {
  BOOST_FOREACH(Element::sharedPointer element, elements) {
    //std::cout << "configuring " << element->name() << std::endl;
    configureElement(element);
  }
}

void ConfigFactory::configureElement(Element::sharedPointer element) {
  // make sure that the element is specified in the config file.
  std::string name = element->name();
  
  // find the "elements" section in the configuration
  Setting& elements = _configuration.lookup("configuration.elements");
  
  // see if the elements list includes this element
  const int elementCount = elements.getLength();
  for (int iElement = 0; iElement < elementCount; ++iElement) {
    Setting& elementSetting = elements[iElement];
    std::string modelID = elementSetting["modelID"];
    if ( RTX_STRINGS_ARE_EQUAL(modelID, name) ) {
      // great, a match.
      // configure the element with the proper states/parameters.
      // todo - check element type (link or node)... names may not be unique.
      // todo - check if the type is in the pointer map
      
      // get the type of parameter
      std::string parameterType = elementSetting["parameter"];
      ParameterFunction fp = _parameterSetter[parameterType];
      const string tsName = elementSetting["timeseries"];
      TimeSeries::sharedPointer series = _timeSeriesList[tsName];
      if (!series) {
        std::cerr << "could not find time series \"" << tsName << "\"." << std::endl;
        return;
      }
      (this->*fp)(elementSetting, element);
    }
  }
}


#pragma mark Specific element configuration

void ConfigFactory::configureQualitySource(Setting &setting, Element::sharedPointer junction) {
  Junction::sharedPointer thisJunction = boost::dynamic_pointer_cast<Junction>(junction);
  if (thisJunction) {
    TimeSeries::sharedPointer quality = _timeSeriesList[setting["timeseries"]];
    thisJunction->setQualitySource(quality);
  }
}

void ConfigFactory::configureBoundaryFlow(Setting &setting, Element::sharedPointer junction) {
  Junction::sharedPointer thisJunction = boost::dynamic_pointer_cast<Junction>(junction);
  if (thisJunction) {
    TimeSeries::sharedPointer flow = _timeSeriesList[setting["timeseries"]];
    thisJunction->setBoundaryFlow(flow);
  }
}

void ConfigFactory::configureHeadMeasure(Setting &setting, Element::sharedPointer junction) {
  Junction::sharedPointer thisJunction = boost::dynamic_pointer_cast<Junction>(junction);
  if (thisJunction) {
    TimeSeries::sharedPointer head = _timeSeriesList[setting["timeseries"]];
    thisJunction->setHeadMeasure(head);
  }
}

void ConfigFactory::configureQualityMeasure(Setting &setting, Element::sharedPointer junction) {
  Junction::sharedPointer thisJunction = boost::dynamic_pointer_cast<Junction>(junction);
  if (thisJunction) {
    TimeSeries::sharedPointer quality = _timeSeriesList[setting["timeseries"]];
    thisJunction->setQualityMeasure(quality);
  }
}

void ConfigFactory::configureBoundaryHead(Setting &setting, Element::sharedPointer reservoir) {
  Reservoir::sharedPointer thisReservoir = boost::dynamic_pointer_cast<Reservoir>(reservoir);
  if (thisReservoir) {
    TimeSeries::sharedPointer head = _timeSeriesList[setting["timeseries"]];
    thisReservoir->setBoundaryHead(head);
  }
}

void ConfigFactory::configurePipeStatus(Setting &setting, Element::sharedPointer pipe) {
  Pipe::sharedPointer thisPipe = boost::dynamic_pointer_cast<Pipe>(pipe);
  if (thisPipe) {
    TimeSeries::sharedPointer status = _timeSeriesList[setting["timeseries"]];
    thisPipe->setStatusParameter(status);
  }
}

void ConfigFactory::configureFlowMeasure(Setting &setting, Element::sharedPointer pipe) {
  Pipe::sharedPointer thisPipe = boost::dynamic_pointer_cast<Pipe>(pipe);
  if (thisPipe) {
    TimeSeries::sharedPointer flow = _timeSeriesList[setting["timeseries"]];
    thisPipe->setFlowMeasure(flow);
  }
}

void ConfigFactory::configurePumpCurve(Setting &setting, Element::sharedPointer pump) {
  Pump::sharedPointer thisPump = boost::dynamic_pointer_cast<Pump>(pump);
  if (thisPump) {
    TimeSeries::sharedPointer curve = _timeSeriesList[setting["timeseries"]];
    thisPump->setCurveParameter(curve);
  }
}

void ConfigFactory::configurePumpEnergyMeasure(Setting &setting, Element::sharedPointer pump) {
  Pump::sharedPointer thisPump = boost::dynamic_pointer_cast<Pump>(pump);
  if (thisPump) {
    TimeSeries::sharedPointer energy = _timeSeriesList[setting["timeseries"]];
    thisPump->setEnergyMeasure(energy);
  }
}
void ConfigFactory::configureValveSetting(Setting &setting, Element::sharedPointer valve) {
  Valve::sharedPointer thisValve = boost::dynamic_pointer_cast<Valve>(valve);
  if (thisValve) {
    TimeSeries::sharedPointer valveSetting = _timeSeriesList[setting["timeseries"]];
    thisValve->setSettingParameter(valveSetting);
  }
}








