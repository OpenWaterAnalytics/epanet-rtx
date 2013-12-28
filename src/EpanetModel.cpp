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
  
}

EpanetModel::~EpanetModel() {
  try {
    ENcheck( ENcloseH(), "ENcloseH");
    ENcheck( ENclose(), "ENclose");
  } catch (...) {
    cerr << "warning: epanet closed improperly" << endl;
  }
  
}

#pragma mark - Loading

void EpanetModel::loadModelFromFile(const std::string& filename) throw(std::exception) {
  
  // base class invocation
  Model::loadModelFromFile(filename);
  
  try {
    
    this->useEpanetFile(filename);
    this->createRtxWrappers();
    
  }
  catch(string error) {
    std::cerr << "ERROR: " << error;
    throw RtxException(error);
  }
  
}


void EpanetModel::useEpanetFile(const std::string& filename) {
  
  Model::loadModelFromFile(filename);
  
  Units volumeUnits(0);
  long enTimeStep;
  
  ENcheck( ENopen((char*)filename.c_str(), (char*)"", (char*)""), "ENopen" );
  
  // get units from epanet
  int flowUnitType = 0;
  bool isSI = false;
  
  int qualCode, traceNode;
  char chemName[32],chemUnits[32];
  ENcheck( ENgetqualinfo(&qualCode, chemName, chemUnits, &traceNode), "ENgetqualinfo" );
  Units qualUnits = Units::unitOfType(string(chemUnits));
  this->setQualityUnits(qualUnits);
  
  ENcheck( ENgetflowunits(&flowUnitType), "ENgetflowunits");
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
    volumeUnits = RTX_LITER;
  }
  else {
    setHeadUnits(RTX_FOOT);
    volumeUnits = RTX_CUBIC_FOOT;
  }
  
  this->setVolumeUnits(volumeUnits);
  
  
  // what units are quality in? who knows!
  this->setQualityUnits(RTX_MICROSIEMENS_PER_CM);
  ENcheck(ENsetqualtype(CHEM, (char*)"rtxConductivity", (char*)"us/cm", (char*)""), "ENsetqualtype");
  
  // get simulation parameters
  ENcheck(ENgettimeparam(EN_HYDSTEP, &enTimeStep), "ENgettimeparam EN_HYDSTEP");
  
  this->setHydraulicTimeStep((int)enTimeStep);
  
  ENcheck(ENopenH(), "ENopenH");
  ENcheck(ENinitH(10), "ENinitH");
  ENcheck(ENopenQ(), "ENopenQ");
  ENcheck(ENinitQ(EN_NOSAVE), "ENinitQ");
  
  
  int nodeCount, tankCount, linkCount;
  char enName[RTX_MAX_CHAR_STRING];
  
  ENcheck( ENgetcount(EN_NODECOUNT, &nodeCount), "ENgetcount EN_NODECOUNT" );
  ENcheck( ENgetcount(EN_TANKCOUNT, &tankCount), "ENgetcount EN_TANKCOUNT" );
  ENcheck( ENgetcount(EN_LINKCOUNT, &linkCount), "ENgetcount EN_LINKCOUNT" );
  
  // create lookup maps for name->index
  for (int iNode=1; iNode <= nodeCount; iNode++) {
    ENcheck( ENgetnodeid(iNode, enName), "ENgetnodeid" );
    // and keep track of the epanet-toolkit index of this element
    _nodeIndex[string(enName)] = iNode;
  }
  
  for (int iLink = 1; iLink <= linkCount; iLink++) {
    ENcheck(ENgetlinkid(iLink, enName), "ENgetlinkid");
    // keep track of this element index
    _linkIndex[string(enName)] = iLink;
  }
  
  
  
}


void EpanetModel::createRtxWrappers() {
  
  int nodeCount, tankCount, linkCount;
  
  ENcheck( ENgetcount(EN_NODECOUNT, &nodeCount), "ENgetcount EN_NODECOUNT" );
  ENcheck( ENgetcount(EN_TANKCOUNT, &tankCount), "ENgetcount EN_TANKCOUNT" );
  ENcheck( ENgetcount(EN_LINKCOUNT, &linkCount), "ENgetcount EN_LINKCOUNT" );
  
  // create nodes
  for (int iNode=1; iNode <= nodeCount; iNode++) {
    char enName[RTX_MAX_CHAR_STRING];
    double x,y,z;         // rtx coordinates
    int nodeType;         // epanet node type code
    string nodeName;
    Junction::sharedPointer newJunction;
    Reservoir::sharedPointer newReservoir;
    Tank::sharedPointer newTank;
    
    // get relevant info from EPANET toolkit
    ENcheck( ENgetnodeid(iNode, enName), "ENgetnodeid" );
    ENcheck( ENgetnodevalue(iNode, EN_ELEVATION, &z), "ENgetnodevalue EN_ELEVATION");
    ENcheck( ENgetnodetype(iNode, &nodeType), "ENgetnodetype");
    ENcheck( ENgetcoord(iNode, &x, &y), "ENgetcoord");
    //xstd::cout << "coord: " << iNode << " " << x << " " << y << std::endl;
    
    nodeName = string(enName);
    
    //CurveFunction::sharedPointer volumeCurveTs;
    vector< pair<double,double> > curveGeometry;
    double minLevel = 0, maxLevel = 0;
    
    switch (nodeType) {
      case EN_TANK:
        newTank.reset( new Tank(nodeName) );
        // get tank geometry from epanet and pass it along
        // todo -- geometry
        
        addTank(newTank);
        
        ENcheck(ENgetnodevalue(iNode, EN_MAXLEVEL, &maxLevel), "ENgetnodevalue(EN_MAXLEVEL)");
        ENcheck(ENgetnodevalue(iNode, EN_MINLEVEL, &minLevel), "ENgetnodevalue(EN_MINLEVEL)");
        newTank->setMinMaxLevel(minLevel, maxLevel);
        
        newTank->level()->setUnits(headUnits());
        newTank->flowMeasure()->setUnits(flowUnits());
        //volumeCurveTs = boost::static_pointer_cast<CurveFunction>(newTank->volumeMeasure());
        //volumeCurveTs->setInputUnits(headUnits());
        //volumeCurveTs->setUnits(volumeUnits);
        
        double volumeCurveIndex;
        ENcheck(ENgetnodevalue(iNode, EN_VOLCURVE, &volumeCurveIndex), "ENgetnodevalue EN_VOLCURVE");
        
        
        if (volumeCurveIndex > 0) {
          // curved tank
          double *xVals, *yVals;
          char curveId[256];
          int nVals;
          ENcheck(ENgetcurve(volumeCurveIndex, curveId, &nVals, &xVals, &yVals), "ENgetcurve");
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
          
          ENcheck(ENgetnodevalue(iNode, EN_MAXLEVEL, &maxLevel), "EN_MAXLEVEL");
          ENcheck(ENgetnodevalue(iNode, EN_MINLEVEL, &minLevel), "EN_MINLEVEL");
          ENcheck(ENgetnodevalue(iNode, EN_MINVOLUME, &minVolume), "EN_MINVOLUME");
          ENcheck(ENgetnodevalue(iNode, EN_MAXVOLUME, &maxVolume), "EN_MAXVOLUME");
          
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
    newJunction->demand()->setUnits(flowUnits());
    newJunction->quality()->setUnits(qualityUnits());
    
    // newJunction is the generic (base-class) pointer to the specific object,
    // so we can use base-class methods to set some parameters.
    newJunction->setElevation(z);
    newJunction->setCoordinates(x, y);
    
    // Base demand is sum of all demand categories, accounting for patterns
    double demand = 0, categoryDemand = 0, avgPatternValue = 0;
    int numDemands = 0, patternIdx = 0;
    ENcheck( ENgetnumdemands(iNode, &numDemands), "ENgetnumdemands()");
    for (int demandIdx = 1; demandIdx <= numDemands; demandIdx++) {
      ENcheck( ENgetbasedemand(iNode, demandIdx, &categoryDemand), "ENgetbasedemand()" );
      ENcheck( ENgetdemandpattern(iNode, demandIdx, &patternIdx), "ENgetdemandpattern()");
      avgPatternValue = 1.0;
      if (patternIdx > 0) { // Not the default "pattern" = 1
        ENcheck( ENgetaveragepatternvalue(patternIdx, &avgPatternValue), "ENgetaveragepatternvalue()");
      }
      demand += categoryDemand * avgPatternValue;
    }
    newJunction->setBaseDemand(demand);
    
    
    
  } // for iNode
  
  // create links
  for (int iLink = 1; iLink <= linkCount; iLink++) {
    char enLinkName[RTX_MAX_CHAR_STRING], enFromName[RTX_MAX_CHAR_STRING], enToName[RTX_MAX_CHAR_STRING];
    int linkType, enFrom, enTo;
    double length, diameter, status;
    string linkName;
    Node::sharedPointer startNode, endNode;
    Pipe::sharedPointer newPipe;
    Pump::sharedPointer newPump;
    Valve::sharedPointer newValve;
    
    // a bunch of epanet api calls to get properties from the link
    ENcheck(ENgetlinkid(iLink, enLinkName), "ENgetlinkid");
    ENcheck(ENgetlinktype(iLink, &linkType), "ENgetlinktype");
    ENcheck(ENgetlinknodes(iLink, &enFrom, &enTo), "ENgetlinknodes");
    ENcheck(ENgetnodeid(enFrom, enFromName), "ENgetnodeid - enFromName");
    ENcheck(ENgetnodeid(enTo, enToName), "ENgetnodeid - enToName");
    ENcheck(ENgetlinkvalue(iLink, EN_DIAMETER, &diameter), "ENgetlinkvalue EN_DIAMETER");
    ENcheck(ENgetlinkvalue(iLink, EN_LENGTH, &length), "ENgetlinkvalue EN_LENGTH");
    ENcheck(ENgetlinkvalue(iLink, EN_INITSTATUS, &status), "ENgetlinkvalue EN_STATUS");
    
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
    ENcheck( ENgetcount(EN_NODECOUNT, &nodeCount), "ENgetcount EN_NODECOUNT" );
    ENcheck( ENgetcount(EN_TANKCOUNT, &tankCount), "ENgetcount EN_TANKCOUNT" );
    
    // eliminate all patterns and control rules
    
    int nPatterns;
    double zeroPattern[2];
    zeroPattern[0] = 0;
    
    // clear all patterns, set to zero.  we do this to get rid of extra demand patterns set in [DEMANDS],
    // since there's no other way to get to them using the toolkit.
    ENcheck( ENgetcount(EN_PATCOUNT, &nPatterns), "ENgetcount(EN_PATCOUNT)" );
    for (int i=1; i<=nPatterns; i++) {
      ENcheck( ENsetpattern(i, zeroPattern, 1), "ENsetpattern" );
    }
    for(int iNode = 1; iNode <= nodeCount ; iNode++)  {
      // set pattern to index 0 (unity multiplier). this also rids us of any multiple demand patterns.
      ENcheck( ENsetnodevalue( iNode, EN_PATTERN, 0 ), "ENsetnodevalue(EN_PATTERN)" );  // demand patterns to unity
      ENcheck( ENsetnodevalue( iNode, EN_BASEDEMAND, 0. ), "ENsetnodevalue(EN_BASEDEMAND)" );	// set base demand to zero
    }
    // set the global demand multiplier is unity as well.
    ENsetoption(EN_DEMANDMULT, 1.);
    
    // disregard controls.
    int controlCount;
    ENcheck( ENgetcount(EN_CONTROLCOUNT, &controlCount), "ENgetcount(EN_CONTROLCOUNT)" );
    for( int iControl = 1; iControl <= controlCount; iControl++ ) {
      ENcheck( ENsetcontrol( iControl, 0, 0, 0, 0, 0 ), "ENsetcontrol" );
    }
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

void EpanetModel::setTankLevel(const string& tank, double level) {
  // just refer to the reservoir method, since in epanet they are the same thing.
  setReservoirHead(tank, level);
}

void EpanetModel::setJunctionDemand(const string& junction, double demand) {
  int nodeIndex = _nodeIndex[junction];
  // Junction demand is total demand - so deal with multiple categories
  int numDemands = 0;
  ENcheck( ENgetnumdemands(nodeIndex, &numDemands), "ENgetnumdemands()");
  for (int demandIdx = 1; demandIdx < numDemands; demandIdx++) {
    ENcheck( ENsetbasedemand(nodeIndex, demandIdx, 0.0), "ENsetbasedemand()" );
  }
  // Last demand category is the one... per EPANET convention
  ENcheck( ENsetbasedemand(nodeIndex, numDemands, demand), "ENsetbasedemand()" );
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

double EpanetModel::junctionQuality(const string &junction) {
  return getNodeValue(EN_QUALITY, junction);
}

double EpanetModel::reservoirLevel(const string &reservoir) {
  return getNodeValue(EN_TANKLEVEL, reservoir);
}

double EpanetModel::tankLevel(const string &tank) {
  // no difference in epanet
  return reservoirLevel(tank);
}

double EpanetModel::pipeFlow(const string &pipe) {
  return getLinkValue(EN_FLOW, pipe);
}

double EpanetModel::pumpEnergy(const string &pump) {
  return getLinkValue(EN_ENERGY, pump);
}


#pragma mark Simulation Methods

void EpanetModel::solveSimulation(time_t time) {
  // TODO -- 
  /*
      determine how to perform this function. maybe keep track of relative error for each simulation period
   so we know what simulation periods have been solved - as in a master simulation clock?
   
   */
  long timestep;
  // set the current epanet-time to zero, since we override epanet-time.
  setCurrentSimulationTime( time );
  ENcheck(ENsettimeparam(EN_HTIME, 0), "ENsettimeparam(EN_HTIME)");
  ENcheck(ENsettimeparam(EN_QTIME, 0), "ENsettimeparam(EN_QTIME)");
  // solve the hydraulics
  ENcheck(ENrunH(&timestep), "ENrunH");
  if (this->shouldRunWaterQuality()) {
    ENcheck(ENrunQ(&timestep), "ENrunQ");
  }
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
  ENcheck( ENsettimeparam(EN_REPORTSTEP, (long)actualTimeStep), "ENsettimeparam(EN_REPORTSTEP)");
  ENcheck( ENsettimeparam(EN_HYDSTEP, (long)actualTimeStep), "ENsettimeparam(EN_HYDSTEP)");
  ENcheck( ENgettimeparam(EN_NEXTEVENT, &stepLength), "ENgettimeparam(EN_NEXTEVENT)" );
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
  
  ENcheck( ENsettimeparam(EN_HYDSTEP, step), "ENsettimeparam(EN_HYDSTEP)" );
  ENcheck( ENnextH(&step), "ENnexH()" );
  
  if (this->shouldRunWaterQuality()) {
    ENcheck(ENnextQ(&qstep), "ENnextQ");
  }
  
  long supposedStep = time - currentSimulationTime();
  if (step != supposedStep) {
    // it's an intermediate step
    //cerr << "model returned step: " << step << ", expecting " << supposedStep << endl;
  }
  setCurrentSimulationTime( currentSimulationTime() + step );
}

int EpanetModel::iterations(time_t time) {
  double iterations;
  ENcheck( ENgetstatistic(EN_ITERATIONS, &iterations), "ENgetstatistic(EN_ITERATIONS)");
  return iterations;
}

int EpanetModel::relativeError(time_t time) {
  double relativeError;
  ENcheck( ENgetstatistic(EN_RELATIVEERROR, &relativeError), "ENgetstatistic(EN_RELATIVEERROR)");
  return relativeError;
}


void EpanetModel::setHydraulicTimeStep(int seconds) {
  
  ENcheck( ENsettimeparam(EN_REPORTSTEP, (long)seconds), "ENsettimeparam(EN_REPORTSTEP)" );
  ENcheck( ENsettimeparam(EN_HYDSTEP, (long)seconds), "ENsettimeparam(EN_HYDSTEP)" );
  // base class method
  Model::setHydraulicTimeStep(seconds);
  
}

void EpanetModel::setQualityTimeStep(int seconds) {
  ENcheck( ENsettimeparam(EN_QUALSTEP, (long)seconds), "ENsettimeparam(EN_QUALSTEP)");
  // call base class
  Model::setQualityTimeStep(seconds);
}

void EpanetModel::setInitialQuality(double qual) {
  ENcheck(ENcloseQ(), "ENcloseQ");
  BOOST_FOREACH(int iNode, _nodeIndex | boost::adaptors::map_values) {
    ENcheck(ENsetnodevalue(iNode, EN_INITQUAL, qual), "ENsetnodevalue - EN_INITQUAL");
  }
  ENcheck(ENopenQ(), "ENopenQ");
  ENcheck(ENinitQ(EN_NOSAVE), "ENinitQ");
}

#pragma mark -
#pragma mark Internal Private Methods

double EpanetModel::getNodeValue(int epanetCode, const string& node) {
  int nodeIndex = _nodeIndex[node];
  double value;
  ENcheck(ENgetnodevalue(nodeIndex, epanetCode, &value), "ENgetnodevalue");
  return value;
}
void EpanetModel::setNodeValue(int epanetCode, const string& node, double value) {
  int nodeIndex = _nodeIndex[node];
  ENcheck(ENsetnodevalue(nodeIndex, epanetCode, value), "ENsetnodevalue");
}

double EpanetModel::getLinkValue(int epanetCode, const string& link) {
  int linkIndex = _linkIndex[link];
  double value;
  ENcheck(ENgetlinkvalue(linkIndex, epanetCode, &value), "ENgetlinkvalue");
  return value;
}
void EpanetModel::setLinkValue(int epanetCode, const string& link, double value) {
  int linkIndex = _linkIndex[link];
  ENcheck(ENsetlinkvalue(linkIndex, epanetCode, value), "ENsetlinkvalue");
}

void EpanetModel::ENcheck(int errorCode, string externalFunction) throw(string) {
  if (errorCode > 10) {
    char errorMsg[256];
    ENgeterror(errorCode, errorMsg, 255);
    throw externalFunction + "::" + std::string(errorMsg);
  }
}


