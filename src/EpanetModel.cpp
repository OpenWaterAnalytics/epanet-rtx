//
//  EpanetModel.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#include <iostream>
#include "EpanetModel.h"
#include "rtxMacros.h"
#include "CurveFunction.h"

#include <types.h>

#include <boost/range/adaptors.hpp>

using namespace RTX;
using namespace std;

EpanetModel::EpanetModel() : Model() {
  // nothing to do, right?
  _enOpened = false;
  OW_API_CHECK( OW_newModel(&_enModel), "OW_newModel");
}

EpanetModel::~EpanetModel() {
  this->closeEngine();
}

OW_Project* EpanetModel::epanetModelPointer() {
  return _enModel;
}

#pragma mark - Loading

void EpanetModel::loadModelFromFile(const std::string& filename) throw(std::exception) {
  
  // base class invocation
  Model::loadModelFromFile(filename);
  
  try {
    
    this->useEpanetFile(filename);
    this->createRtxWrappers();
    
  }
  catch(const std::string& errStr) {
    std::cerr << "ERROR: ";
    throw RtxException("File Loading Error: " + errStr);
  }
  
}


void EpanetModel::useEpanetFile(const std::string& filename) {
  
  Model::loadModelFromFile(filename);
  
  Units volumeUnits(0);
  long enTimeStep;
  try {
    OW_API_CHECK( OW_open((char*)filename.c_str(), &_enModel, (char*)"", (char*)""), "OW_open" );
  } catch (const std::string& errStr) {
    cerr << "model not formatted correctly. " << errStr << endl;
    throw "model not formatted correctly. " + errStr;
  }
  
  
  // get units from epanet
  int flowUnitType = 0;
  bool isSI = false;
  
  int qualCode, traceNode;
  char chemName[32],chemUnits[32];
  OW_API_CHECK( OW_getqualinfo(_enModel, &qualCode, chemName, chemUnits, &traceNode), "OW_getqualinfo" );
  Units qualUnits = Units::unitOfType(string(chemUnits));
  this->setQualityUnits(qualUnits);
  
  OW_API_CHECK( OW_getflowunits(_enModel, &flowUnitType), "OW_getflowunits");
  switch (flowUnitType)
  {
    case EN_LPS:
    case EN_LPM:
    case EN_MLD:
    case EN_CMH:
    case EN_CMD:
      isSI = true;
      break;
    default:
      isSI = false;
  }
  switch (flowUnitType) {
    case EN_LPS:
      setFlowUnits(RTX_LITER_PER_SECOND);
      break;
    case EN_MLD:
      setFlowUnits(RTX_MILLION_LITER_PER_DAY);
      break;
    case EN_CMH:
      setFlowUnits(RTX_CUBIC_METER_PER_HOUR);
      break;
    case EN_CMD:
      setFlowUnits(RTX_CUBIC_METER_PER_DAY);
      break;
    case EN_GPM:
      setFlowUnits(RTX_GALLON_PER_MINUTE);
      break;
    case EN_MGD:
      setFlowUnits(RTX_MILLION_GALLON_PER_DAY);
      break;
    case EN_IMGD:
      setFlowUnits(RTX_IMPERIAL_MILLION_GALLON_PER_DAY);
      break;
    case EN_AFD:
      setFlowUnits(RTX_ACRE_FOOT_PER_DAY);
    default:
      break;
  }
  
  if (isSI) {
    setHeadUnits(RTX_METER);
    setPressureUnits(RTX_KILOPASCAL);
    volumeUnits = RTX_LITER;
    
  }
  else {
    setHeadUnits(RTX_FOOT);
    setPressureUnits(RTX_PSI);
    volumeUnits = RTX_CUBIC_FOOT;
  }
  
  this->setVolumeUnits(volumeUnits);
  
  // what units are quality in? who knows!
  this->setQualityUnits(RTX_MICROSIEMENS_PER_CM);
  OW_API_CHECK(OW_setqualtype(_enModel, CHEM, (char*)"rtxConductivity", (char*)"us/cm", (char*)""), "OW_setqualtype");
  
  // get simulation parameters
  OW_API_CHECK(OW_gettimeparam(_enModel, EN_HYDSTEP, &enTimeStep), "OW_gettimeparam EN_HYDSTEP");
  
  this->setHydraulicTimeStep((int)enTimeStep);
  
  int nodeCount, tankCount, linkCount;
  char enName[RTX_MAX_CHAR_STRING];
  
  OW_API_CHECK( OW_getcount(_enModel, EN_NODECOUNT, &nodeCount), "OW_getcount EN_NODECOUNT" );
  OW_API_CHECK( OW_getcount(_enModel, EN_TANKCOUNT, &tankCount), "OW_getcount EN_TANKCOUNT" );
  OW_API_CHECK( OW_getcount(_enModel, EN_LINKCOUNT, &linkCount), "OW_getcount EN_LINKCOUNT" );
  
  // create lookup maps for name->index
  for (int iNode=1; iNode <= nodeCount; iNode++) {
    OW_API_CHECK( OW_getnodeid(_enModel, iNode, enName), "OW_getnodeid" );
    // and keep track of the epanet-toolkit index of this element
    _nodeIndex[string(enName)] = iNode;
  }
  
  for (int iLink = 1; iLink <= linkCount; iLink++) {
    OW_API_CHECK(OW_getlinkid(_enModel, iLink, enName), "OW_getlinkid");
    // keep track of this element index
    _linkIndex[string(enName)] = iLink;
  }
  
  
  // get the valve types
  BOOST_FOREACH(Valve::_sp v, this->valves()) {
    int enIdx = _linkIndex[v->name()];
    EN_LinkType type = EN_PIPE;
    OW_getlinktype(_enModel, enIdx, &type);
    if (type == EN_PIPE) {
      // should not happen
      continue;
    }
    v->valveType = (int)type;
  }
  
  
}

void EpanetModel::initEngine() {
  if (_enOpened) {
    return;
  }
  try {
    OW_API_CHECK(OW_openH(_enModel), "OW_openH");
    OW_API_CHECK(OW_initH(_enModel, 10), "OW_initH");
    OW_API_CHECK(OW_openQ(_enModel), "OW_openQ");
    OW_API_CHECK(OW_initQ(_enModel, EN_NOSAVE), "OW_initQ");
  } catch (...) {
    cerr << "warning: epanet opened improperly" << endl;
  }
  _enOpened = true;
}

void EpanetModel::closeEngine() {
  if (_enOpened) {
    try {
      OW_API_CHECK( OW_closeQ(_enModel), "OW_closeQ");
      OW_API_CHECK( OW_closeH(_enModel), "OW_closeH");
    } catch (...) {
      cerr << "warning: epanet closed improperly" << endl;
    }
    _enOpened = false;
  }
  OW_API_CHECK( OW_close(_enModel), "OW_close");
}


void EpanetModel::createRtxWrappers() {
  
  int nodeCount, tankCount, linkCount;
  
  try {
    OW_API_CHECK( OW_getcount(_enModel, EN_NODECOUNT, &nodeCount), "OW_getcount EN_NODECOUNT" );
    OW_API_CHECK( OW_getcount(_enModel, EN_TANKCOUNT, &tankCount), "OW_getcount EN_TANKCOUNT" );
    OW_API_CHECK( OW_getcount(_enModel, EN_LINKCOUNT, &linkCount), "OW_getcount EN_LINKCOUNT" );
  } catch (...) {
    throw "Could not create wrappers";
  }
  
  
  // create nodes
  for (int iNode=1; iNode <= nodeCount; iNode++) {
    char enName[RTX_MAX_CHAR_STRING];
    double x,y,z;         // rtx coordinates
    EN_NodeType nodeType;         // epanet node type code
    string nodeName;
    Junction::_sp newJunction;
    Reservoir::_sp newReservoir;
    Tank::_sp newTank;
    
    // get relevant info from EPANET toolkit
    OW_API_CHECK( OW_getnodeid(_enModel, iNode, enName), "OW_getnodeid" );
    OW_API_CHECK( OW_getnodevalue(_enModel, iNode, EN_ELEVATION, &z), "OW_getnodevalue EN_ELEVATION");
    OW_API_CHECK( OW_getnodetype(_enModel, iNode, &nodeType), "OW_getnodetype");
    OW_API_CHECK( OW_getcoord(_enModel, iNode, &x, &y), "OW_getcoord");
    //xstd::cout << "coord: " << iNode << " " << x << " " << y << std::endl;
    
    nodeName = string(enName);
    
    //CurveFunction::_sp volumeCurveTs;
    vector< pair<double,double> > curveGeometry;
    double minLevel = 0, maxLevel = 0;
    
    switch (nodeType) {
      case EN_TANK:
        newTank.reset( new Tank(nodeName) );
        // get tank geometry from epanet and pass it along
        // todo -- geometry
        
        addTank(newTank);
        
        OW_API_CHECK(OW_getnodevalue(_enModel, iNode, EN_MAXLEVEL, &maxLevel), "OW_getnodevalue(EN_MAXLEVEL)");
        OW_API_CHECK(OW_getnodevalue(_enModel, iNode, EN_MINLEVEL, &minLevel), "OW_getnodevalue(EN_MINLEVEL)");
        newTank->setMinMaxLevel(minLevel, maxLevel);
        
        newTank->level()->setUnits(headUnits());
        newTank->flowMeasure()->setUnits(flowUnits());
        newTank->volumeMeasure()->setUnits(volumeUnits());
        newTank->flow()->setUnits(flowUnits());
        newTank->volume()->setUnits(volumeUnits());
        //volumeCurveTs = boost::static_pointer_cast<CurveFunction>(newTank->volumeMeasure());
        //volumeCurveTs->setInputUnits(headUnits());
        //volumeCurveTs->setUnits(volumeUnits);
        
        double volumeCurveIndex;
        OW_API_CHECK(OW_getnodevalue(_enModel, iNode, EN_VOLCURVE, &volumeCurveIndex), "OW_getnodevalue EN_VOLCURVE");
        
        
        if (volumeCurveIndex > 0) {
          // curved tank
          double *xVals, *yVals;
          char curveId[256];
          int nVals;
          OW_API_CHECK(OW_getcurve(_enModel, volumeCurveIndex, curveId, &nVals, &xVals, &yVals), "OW_getcurve");
          for (int iPoint = 0; iPoint < nVals; iPoint++) {
            curveGeometry.push_back(make_pair(xVals[iPoint], yVals[iPoint]));
            //volumeCurveTs->addCurveCoordinate(xVals[iPoint], yVals[iPoint]);
            //cout << "(" << xVals[iPoint] << "," << yVals[iPoint] << ")-";
          }
          // client must free x/y values
          free(xVals);
          free(yVals);
          
          newTank->geometryName = string(curveId);
          
          //cout << endl;
        }
        else {
          // it's a cylindrical tank - invent a curve
          double minVolume, maxVolume;
          double minLevel, maxLevel;
          
          OW_API_CHECK(OW_getnodevalue(_enModel, iNode, EN_MAXLEVEL, &maxLevel), "EN_MAXLEVEL");
          OW_API_CHECK(OW_getnodevalue(_enModel, iNode, EN_MINLEVEL, &minLevel), "EN_MINLEVEL");
          OW_API_CHECK(OW_getnodevalue(_enModel, iNode, EN_MINVOLUME, &minVolume), "EN_MINVOLUME");
          OW_API_CHECK(OW_getnodevalue(_enModel, iNode, EN_MAXVOLUME, &maxVolume), "EN_MAXVOLUME");
          
          curveGeometry.push_back(make_pair(minLevel, minVolume));
          curveGeometry.push_back(make_pair(maxLevel, maxVolume));
          
          //volumeCurveTs->addCurveCoordinate(minLevel, minVolume);
          //volumeCurveTs->addCurveCoordinate(maxLevel, maxVolume);
        }
        
        // set tank geometry
        newTank->setGeometry(curveGeometry, headUnits(), this->volumeUnits());
        
        newJunction = newTank;
        break;
      case EN_RESERVOIR:
        newReservoir.reset( new Reservoir(nodeName) );
        addReservoir(newReservoir);
        newJunction = newReservoir;
        break;
      case EN_JUNCTION:
        newJunction.reset( new Junction(nodeName) );
        addJunction(newJunction);
        break;
      default:
        throw "Node Type Unknown";
    } // switch nodeType
    
    // set units for new element
    newJunction->head()->setUnits(headUnits());
    newJunction->pressure()->setUnits(pressureUnits());
    newJunction->demand()->setUnits(flowUnits());
    newJunction->quality()->setUnits(qualityUnits());
    
    // newJunction is the generic (base-class) pointer to the specific object,
    // so we can use base-class methods to set some parameters.
    newJunction->setElevation(z);
    newJunction->setCoordinates(x, y);
    
    // Initial quality specified in input data
    double initQual;
    OW_API_CHECK(OW_getnodevalue(_enModel, iNode, EN_INITQUAL, &initQual), "EN_INITQUAL");
    newJunction->setInitialQuality(initQual);
    
    // Base demand is sum of all demand categories, accounting for patterns
    double demand = 0, categoryDemand = 0, avgPatternValue = 0;
    int numDemands = 0, patternIdx = 0;
    OW_API_CHECK( OW_getnumdemands(_enModel, iNode, &numDemands), "OW_getnumdemands()");
    for (int demandIdx = 1; demandIdx <= numDemands; demandIdx++) {
      OW_API_CHECK( OW_getbasedemand(_enModel, iNode, demandIdx, &categoryDemand), "OW_getbasedemand()" );
      OW_API_CHECK( OW_getdemandpattern(_enModel, iNode, demandIdx, &patternIdx), "OW_getdemandpattern()");
      avgPatternValue = 1.0;
      if (patternIdx > 0) { // Not the default "pattern" = 1
        OW_API_CHECK( OW_getaveragepatternvalue(_enModel, patternIdx, &avgPatternValue), "OW_getaveragepatternvalue()");
      }
      demand += categoryDemand * avgPatternValue;
    }
    newJunction->setBaseDemand(demand);
    
    
    
  } // for iNode
  
  // create links
  for (int iLink = 1; iLink <= linkCount; iLink++) {
    char enLinkName[RTX_MAX_CHAR_STRING], enFromName[RTX_MAX_CHAR_STRING], enToName[RTX_MAX_CHAR_STRING];
    int enFrom, enTo;
    EN_LinkType linkType;
    double length, diameter, status;
    string linkName;
    Node::_sp startNode, endNode;
    Pipe::_sp newPipe;
    Pump::_sp newPump;
    Valve::_sp newValve;
    
    // a bunch of epanet api calls to get properties from the link
    OW_API_CHECK(OW_getlinkid(_enModel, iLink, enLinkName), "OW_getlinkid");
    OW_API_CHECK(OW_getlinktype(_enModel, iLink, &linkType), "OW_getlinktype");
    OW_API_CHECK(OW_getlinknodes(_enModel, iLink, &enFrom, &enTo), "OW_getlinknodes");
    OW_API_CHECK(OW_getnodeid(_enModel, enFrom, enFromName), "OW_getnodeid - enFromName");
    OW_API_CHECK(OW_getnodeid(_enModel, enTo, enToName), "OW_getnodeid - enToName");
    OW_API_CHECK(OW_getlinkvalue(_enModel, iLink, EN_DIAMETER, &diameter), "OW_getlinkvalue EN_DIAMETER");
    OW_API_CHECK(OW_getlinkvalue(_enModel, iLink, EN_LENGTH, &length), "OW_getlinkvalue EN_LENGTH");
    OW_API_CHECK(OW_getlinkvalue(_enModel, iLink, EN_INITSTATUS, &status), "OW_getlinkvalue EN_STATUS");
    
    linkName = string(enLinkName);
    
    // get node pointers
    startNode = nodeWithName(string(enFromName));
    endNode = nodeWithName(string(enToName));
    
    if (! (startNode && endNode) ) {
      std::cerr << "could not find nodes for link " << linkName << std::endl;
      throw "nodes not found";
    }
    
    
    // create the new specific type and add it.
    // newPipe becomes the generic (base-class) pointer in all cases.
    switch (linkType) {
      case EN_PIPE:
        newPipe.reset( new Pipe(linkName, startNode, endNode) );
        addPipe(newPipe);
        break;
      case EN_PUMP:
        newPump.reset( new Pump(linkName, startNode, endNode) );
        newPipe = newPump;
        addPump(newPump);
        break;
      case EN_CVPIPE:
      case EN_PSV:
      case EN_PRV:
      case EN_FCV:
      case EN_PBV:
      case EN_TCV:
      case EN_GPV:
        newValve.reset( new Valve(linkName, startNode, endNode) );
        newValve->valveType = (int)linkType;
        newPipe = newValve;
        addValve(newValve);
        break;
      default:
        std::cerr << "could not find pipe type" << std::endl;
        break;
    } // switch linkType
    
    
    // now that the pipe is created, set some basic properties.
    newPipe->setDiameter(diameter);
    newPipe->setLength(length);
    
    if (status == 0) {
      newPipe->setFixedStatus(Pipe::CLOSED);
    }
    
    newPipe->flow()->setUnits(flowUnits());
    
    
    
  } // for iLink
  
}

void EpanetModel::overrideControls() throw(RTX::RtxException) {
  // set up counting variables for creating model elements.
  int nodeCount, tankCount;
  
  try {
    OW_API_CHECK( OW_getcount(_enModel, EN_NODECOUNT, &nodeCount), "OW_getcount EN_NODECOUNT" );
    OW_API_CHECK( OW_getcount(_enModel, EN_TANKCOUNT, &tankCount), "OW_getcount EN_TANKCOUNT" );
    
    // eliminate all patterns and control rules
    
    int nPatterns;
    double zeroPattern[2];
    zeroPattern[0] = 0;
    
    // clear all patterns, set to zero.  we do this to get rid of extra demand patterns set in [DEMANDS],
    // since there's no other way to get to them using the toolkit.
    OW_API_CHECK( OW_getcount(_enModel, EN_PATCOUNT, &nPatterns), "OW_getcount(EN_PATCOUNT)" );
    for (int i=1; i<=nPatterns; i++) {
      OW_API_CHECK( OW_setpattern(_enModel, i, zeroPattern, 1), "OW_setpattern" );
    }
    for(int iNode = 1; iNode <= nodeCount ; iNode++)  {
      // set pattern to index 0 (unity multiplier). this also rids us of any multiple demand patterns.
      OW_API_CHECK( OW_setnodevalue(_enModel, iNode, EN_PATTERN, 0 ), "OW_setnodevalue(EN_PATTERN)" );  // demand patterns to unity
      OW_API_CHECK( OW_setnodevalue(_enModel, iNode, EN_BASEDEMAND, 0. ), "OW_setnodevalue(EN_BASEDEMAND)" );	// set base demand to zero
    }
    // set the global demand multiplier is unity as well.
    OW_setoption(_enModel, EN_DEMANDMULT, 1.);
    
    // disregard controls and rules.
    this->disableControls();
  }
  catch(string error) {
    std::cerr << "ERROR: " << error;
    throw RtxException();
  }
  // base class implementation
  Model::overrideControls();
}

#pragma mark - Protected Methods:

std::ostream& EpanetModel::toStream(std::ostream &stream) {
  // epanet-specific printing
  stream << "Epanet Model File: " << endl;
  Model::toStream(stream);
  return stream;
}

#pragma mark Setters

/* setting simulation parameters */

void EpanetModel::setReservoirHead(const string& reservoir, double level) {
  setNodeValue(EN_TANKLEVEL, reservoir, level);
}

void EpanetModel::setReservoirQuality(const string& reservoir, double quality) {
  setNodeValue(EN_SOURCEQUAL, reservoir, quality);
}

void EpanetModel::setTankLevel(const string& tank, double level) {
  // just refer to the reservoir method, since in epanet they are the same thing.
  setReservoirHead(tank, level);
}

void EpanetModel::setJunctionDemand(const string& junction, double demand) {
  int nodeIndex = _nodeIndex[junction];
  // Junction demand is total demand - so deal with multiple categories
  int numDemands = 0;
  OW_API_CHECK( OW_getnumdemands(_enModel, nodeIndex, &numDemands), "OW_getnumdemands()");
  for (int demandIdx = 1; demandIdx < numDemands; demandIdx++) {
    OW_API_CHECK( OW_setbasedemand(_enModel, nodeIndex, demandIdx, 0.0), "OW_setbasedemand()" );
  }
  // Last demand category is the one... per EPANET convention
  OW_API_CHECK( OW_setbasedemand(_enModel, nodeIndex, numDemands, demand), "OW_setbasedemand()" );
}

void EpanetModel::setJunctionQuality(const std::string& junction, double quality) {
  // todo - add more source types, depending on time series dimension?
  setNodeValue(EN_SOURCETYPE, junction, SETPOINT);
  setNodeValue(EN_SOURCEQUAL, junction, quality);
}

void EpanetModel::setPipeStatus(const string& pipe, Pipe::status_t status) {
  setLinkValue(EN_STATUS, pipe, status);
}

void EpanetModel::setPumpStatus(const string& pump, Pipe::status_t status) {
  // call the setPipeStatus method, since they are the same in epanet.
  setPipeStatus(pump, status);
}

void EpanetModel::setPumpSetting(const string& pump, double setting) {
  setLinkValue(EN_SETTING, pump, setting);
}

void EpanetModel::setValveSetting(const string& valve, double setting) {
  setLinkValue(EN_SETTING, valve, setting);
}

#pragma mark Getters

double EpanetModel::junctionDemand(const string &junction) {
  return getNodeValue(EN_DEMAND, junction);
}

double EpanetModel::junctionHead(const string &junction) {
  return getNodeValue(EN_HEAD, junction);
}
double EpanetModel::junctionPressure(const string &junction) {
  return getNodeValue(EN_PRESSURE, junction);
}

double EpanetModel::junctionQuality(const string &junction) {
  return getNodeValue(EN_QUALITY, junction);
}

double EpanetModel::reservoirLevel(const string &reservoir) {
  return getNodeValue(EN_TANKLEVEL, reservoir);
}

double EpanetModel::tankLevel(const string &tank) {
  return junctionHead(tank) - nodeWithName(tank)->elevation(); // node elevation & head in same Epanet units
}

double EpanetModel::tankVolume(const string& tank) {
  return getNodeValue(EN_TANKVOLUME, tank);
}

double EpanetModel::tankFlow(const string& tank) {
  return getNodeValue(EN_DEMAND, tank);
}

double EpanetModel::pipeFlow(const string &pipe) {
  return getLinkValue(EN_FLOW, pipe);
}

double EpanetModel::pumpEnergy(const string &pump) {
  return getLinkValue(EN_ENERGY, pump);
}

#pragma mark - Sim options
void EpanetModel::enableControls() {
  int nC;
  OW_getcount(_enModel, EN_CONTROLCOUNT, &nC);
  
  for (int i = 1; i <= nC; ++i) {
    OW_setControlEnabled(_enModel, i, EN_ENABLE);
  }
  OW_getcount(_enModel, EN_RULECOUNT, &nC);
  for (int i = 1; i <= nC; ++i) {
    OW_setRuleEnabled(_enModel, i, EN_ENABLE);
  }
}

void EpanetModel::disableControls() {
  int nC;
  OW_getcount(_enModel, EN_CONTROLCOUNT, &nC);
  
  for (int i = 1; i <= nC; ++i) {
    OW_setControlEnabled(_enModel, i, EN_DISABLE);
  }
  
  OW_getcount(_enModel, EN_RULECOUNT, &nC);
  for (int i = 1; i <= nC; ++i) {
    OW_setRuleEnabled(_enModel, i, EN_DISABLE);
  }
}



#pragma mark Simulation Methods

bool EpanetModel::solveSimulation(time_t time) {
  // TODO -- 
  /*
      determine how to perform this function. maybe keep track of relative error for each simulation period
   so we know what simulation periods have been solved - as in a master simulation clock?
   
   */
  bool success = true;
  long timestep;
  int errorCode;
  
  // set the current epanet-time to zero, since we override epanet-time.
  setCurrentSimulationTime( time );
  OW_API_CHECK(OW_settimeparam(_enModel, EN_HTIME, 0), "OW_settimeparam(EN_HTIME)");
  OW_API_CHECK(OW_settimeparam(_enModel, EN_QTIME, 0), "OW_settimeparam(EN_QTIME)");
  // solve the hydraulics
  OW_API_CHECK(errorCode = OW_runH(_enModel, &timestep), "OW_runH");
  // check for success
  success = this->_didConverge(time, errorCode);
  
  // how to deal with lack of hydraulic convergence here - reset boundary/initial conditions?
  if (this->shouldRunWaterQuality()) {
    OW_API_CHECK(OW_runQ(_enModel, &timestep), "OW_runQ");
  }
  
  return success;
}

time_t EpanetModel::nextHydraulicStep(time_t time) {
  if ( time != currentSimulationTime() ) {
    // todo - throw something?
    cerr << "time not synchronized!" << endl;
  }
  // get the time of the next hydraulic event (according to the simulation)
  long stepLength = 0;
  time_t nextTime = time;
  // re-set the epanet engine's hydstep parameter to the original value,
  // so that the step length figurer-outerer works.
  int actualTimeStep = hydraulicTimeStep();
  OW_API_CHECK( OW_settimeparam(_enModel, EN_REPORTSTEP, (long)actualTimeStep), "OW_settimeparam(EN_REPORTSTEP)");
  OW_API_CHECK( OW_settimeparam(_enModel, EN_HYDSTEP, (long)actualTimeStep), "OW_settimeparam(EN_HYDSTEP)");
  OW_API_CHECK( OW_gettimeparam(_enModel, EN_NEXTEVENT, &stepLength), "OW_gettimeparam(EN_NEXTEVENT)" );
  nextTime += stepLength;
  
  // todo
  //std::cout << "epanet step length: " << stepLength << std::endl;
  //std::cout << "*** next sim time: " << nextTime << std::endl;
  
  return nextTime;
}

// evolve tank levels
void EpanetModel::stepSimulation(time_t time) {
  long step = (long)(time - currentSimulationTime());
  long qstep = step;
  
  //std::cout << "set step to: " << step << std::endl;
  
  long timeLeftInStep = 0;
  
  OW_API_CHECK( OW_settimeparam(_enModel, EN_HYDSTEP, step), "OW_settimeparam(EN_HYDSTEP)" );
  OW_API_CHECK( OW_settimeparam(_enModel, EN_DURATION, step), "OW_settimeparam(EN_DURATION)");
  OW_API_CHECK( OW_nextH(_enModel, &timeLeftInStep), "OW_nextH()" );
  
  if (this->shouldRunWaterQuality()) {
    OW_API_CHECK(OW_nextQ(_enModel, &qstep), "OW_nextQ");
  }
  
  long supposedStep = time - currentSimulationTime();
  if (step != supposedStep) {
    // it's an intermediate step
    cerr << "model returned step: " << step << ", expecting " << supposedStep << endl;
  }
  setCurrentSimulationTime( currentSimulationTime() + step );
}

int EpanetModel::iterations(time_t time) {
  double iterations;
  OW_API_CHECK( OW_getstatistic(_enModel, EN_ITERATIONS, &iterations), "OW_getstatistic(EN_ITERATIONS)");
  return iterations;
}

double EpanetModel::relativeError(time_t time) {
  double relativeError;
  OW_API_CHECK( OW_getstatistic(_enModel, EN_RELATIVEERROR, &relativeError), "OW_getstatistic(EN_RELATIVEERROR)");
  return relativeError;
}

bool EpanetModel::_didConverge(time_t time, int errorCode) {
  // return true if the simulation did not converge
  EN_API_FLOAT_TYPE accuracy;
  
  OW_API_CHECK( OW_getoption(_enModel, EN_ACCURACY, &accuracy), "OW_getoption");
  bool illcondition = errorCode == 101 || errorCode == 110; // 101 is memory issue, 110 is illconditioning
  bool unbalanced = relativeError(time) > accuracy;
  if (illcondition || unbalanced) {
    return false;
  }
  else {
    return true;
  }
}


void EpanetModel::setHydraulicTimeStep(int seconds) {
  
  OW_API_CHECK( OW_settimeparam(_enModel, EN_REPORTSTEP, (long)seconds), "OW_settimeparam(EN_REPORTSTEP)" );
  OW_API_CHECK( OW_settimeparam(_enModel, EN_HYDSTEP, (long)seconds), "OW_settimeparam(EN_HYDSTEP)" );
  // base class method
  Model::setHydraulicTimeStep(seconds);
  
}

void EpanetModel::setQualityTimeStep(int seconds) {
  OW_API_CHECK( OW_settimeparam(_enModel, EN_QUALSTEP, (long)seconds), "OW_settimeparam(EN_QUALSTEP)");
  // call base class
  Model::setQualityTimeStep(seconds);
}

void EpanetModel::setInitialModelQuality() {
  OW_API_CHECK(OW_closeQ(_enModel), "OW_closeQ");
  OW_API_CHECK(OW_openQ(_enModel), "OW_openQ");

  // Junctions
  BOOST_FOREACH(Junction::_sp junc, this->junctions()) {
    double qual = junc->initialQuality();
    int iNode = _nodeIndex[junc->name()];
    OW_API_CHECK(OW_setnodevalue(_enModel, iNode, EN_INITQUAL, qual), "OW_setnodevalue - EN_INITQUAL");
  }
  
  // Tanks
  BOOST_FOREACH(Tank::_sp tank, this->tanks()) {
    double qual = tank->initialQuality();
    int iNode = _nodeIndex[tank->name()];
    OW_API_CHECK(OW_setnodevalue(_enModel, iNode, EN_INITQUAL, qual), "OW_setnodevalue - EN_INITQUAL");
  }
  
  OW_API_CHECK(OW_initQ(_enModel, EN_NOSAVE), "ENinitQ");
}

#pragma mark -
#pragma mark Internal Private Methods

double EpanetModel::getNodeValue(int epanetCode, const string& node) {
  int nodeIndex = _nodeIndex[node];
  double value;
  OW_API_CHECK(OW_getnodevalue(_enModel, nodeIndex, epanetCode, &value), "OW_getnodevalue");
  return value;
}
void EpanetModel::setNodeValue(int epanetCode, const string& node, double value) {
  int nodeIndex = _nodeIndex[node];
  OW_API_CHECK(OW_setnodevalue(_enModel, nodeIndex, epanetCode, value), "OW_setnodevalue");
}

double EpanetModel::getLinkValue(int epanetCode, const string& link) {
  int linkIndex = _linkIndex[link];
  double value;
  OW_API_CHECK(OW_getlinkvalue(_enModel, linkIndex, epanetCode, &value), "OW_getlinkvalue");
  return value;
}
void EpanetModel::setLinkValue(int epanetCode, const string& link, double value) {
  int linkIndex = _linkIndex[link];
  OW_API_CHECK(OW_setlinkvalue(_enModel, linkIndex, epanetCode, value), "OW_setlinkvalue");
}

void EpanetModel::OW_API_CHECK(int errorCode, string externalFunction) throw(string) {
  if (errorCode > 10) {
    char errorMsg[256];
    OW_geterror(errorCode, errorMsg, 255);
    throw externalFunction + "::" + std::string(errorMsg);
  }
}


